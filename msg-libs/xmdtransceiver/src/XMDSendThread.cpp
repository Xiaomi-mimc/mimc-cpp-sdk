#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // _WIN32

#include <errno.h>
#include <sstream>
#include <memory>
#include <cstring>
#include <thread>
#include <chrono>

#include "XMDSendThread.h"
#include "common.h"
#include "PacketBuilder.h"
#include "XMDLoggerWrapper.h"
#include "XMDTransceiver.h"

XMDSendThread::XMDSendThread(XMDCommonData* commonData, PacketDispatcher* dispactcher, XMDTransceiver* transceiver) {
    stopFlag_ = false;
    listenfd_ = 0;
    commonData_ = commonData;
    dispatcher_ = dispactcher;
    testPacketLoss_ = 0;
	transceiver_ = transceiver;
    is_reset_socket_ = true;
    send_fail_count_ = 0;
#ifdef _WIN32  // set socket to non-blocking mode.
	u_long iMode = 1;
	long iResult = ioctlsocket(listenfd_, FIONBIO, &iMode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);
	int nSendBuf = 5 * 1024 * 1024;
	setsockopt(listenfd_, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int));
#endif // _WIN32

}

XMDSendThread::XMDSendThread() {
    commonData_ = NULL;
}

void XMDSendThread::SetListenFd(int fd) {
    listenfd_ = fd;
}


void* XMDSendThread::process() {
    while (!stopFlag_) {
        bool isSleep = true;
        SendQueueData* sendData = commonData_->socketSendQueuePop();
        if (sendData != NULL) {
            isSleep = false;
            send(sendData->ip, sendData->port, (char*)sendData->data, sendData->len);
            delete sendData;
        }
        SendQueueData* datagram = commonData_->datagramQueuePriorityPop();
        if (datagram != NULL) {
            isSleep = false;
            send(datagram->ip, datagram->port, (char*)datagram->data, datagram->len);
            delete datagram;
        }
 
        //if (!commonData_->resendQueueEmpty()) {
            ResendData* resendData = commonData_->resendQueuePriorityPop();
            if (resendData != NULL) {
                isSleep = false;
                if (resendData->reSendCount > 0 || resendData->reSendCount == -1) {
                    XMDLoggerWrapper::instance()->debug("resend,connid=%ld, packet id=%ld", 
                                                         resendData->connId, resendData->packetId);
                    send(resendData->ip, resendData->port, (char*)resendData->data, resendData->len);
                    uint64_t current_time = current_ms();
                    resendData->reSendTime = current_time + commonData_->getResendTimeInterval();
                    if (resendData->reSendCount != -1) {
                        resendData->reSendCount--;
                    }
                    commonData_->resendQueuePush(resendData);
                } else {
                     std::stringstream ss_ack;
                     ss_ack << resendData->connId << resendData->packetId;
                     std::string ackpacketKey = ss_ack.str();
                     commonData_->deleteIsPacketRecvAckMap(ackpacketKey);
                     
                     PacketCallbackInfo packetInfo;
                     if (commonData_->getDeletePacketCallbackInfo(ackpacketKey, packetInfo)) {
                         if (resendData->packetId == 0) {
                             XMDLoggerWrapper::instance()->warn("conn create fail, didn't recv resp,connid=%ld", resendData->connId);
                             ConnInfo connInfo;
                             if(commonData_->getConnInfo(resendData->connId, connInfo)){
                                 dispatcher_->handleCreateConnFail(resendData->connId, connInfo.ctx);
                                 commonData_->deleteConn(resendData->connId);
                             }
                         } else {
                             std::stringstream ss;
                             ss << packetInfo.connId << packetInfo.streamId << packetInfo.groupId;
                             std::string key = ss.str();
                             if (commonData_->SendCallbackMapRecordExist(key)) {
                                 commonData_->deleteSendCallbackMap(key);
                                 dispatcher_->streamDataSendFail(packetInfo.connId, packetInfo.streamId, 
                                                                 packetInfo.groupId, packetInfo.ctx);
                             }
                         }
                     }

                     
                     XMDLoggerWrapper::instance()->debug("packet resend fail, drop it,connid=%ld, packet id=%ld", 
                                                          resendData->connId, resendData->packetId);
                     delete resendData;
                }
            }

        if (isSleep) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
            //usleep(1000);
        }

    }

    while(1) {
        bool is_break = true;
        if (!commonData_->socketSendQueueEmpty()) {
            SendQueueData* sendData = commonData_->socketSendQueuePop();
            if (NULL != sendData) {
                delete sendData;
                is_break = false;
            }
        }

        if (!commonData_->datagramQueueEmpty()) {
            SendQueueData* datagram = commonData_->datagramQueuePop();
            if (NULL != datagram) {
                delete datagram;
                is_break = false;
            }
        }

        if (commonData_->resendQueueEmpty()) {
            ResendData* resendData = commonData_->resendQueuePop();
            if (NULL != resendData) {
                delete resendData;
                is_break = false;
            }
        }

        if (is_break) {
            break;
        }

    }
    return NULL;
}

void XMDSendThread::send(uint32_t ip, int port, char* data , int len) {

    if (rand32() % 100 < testPacketLoss_) {
        XMDLoggerWrapper::instance()->info("test drop this packet");
        return;
    }
    
    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    memset(&addr, 0, addrLen);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);

    bool is_fail = false;

#ifdef _WIN32
	int ret = sendto(listenfd_, data, len, 0, (struct sockaddr*)&addr, addrLen);
#else
	int ret = sendto(listenfd_, data, len, MSG_DONTWAIT, (struct sockaddr*)&addr, addrLen);
#endif // _WIN32

#ifdef _WIN32
	if (ret == SOCKET_ERROR){
		XMDLoggerWrapper::instance()->warn("XMDSendThread send fail, ip:%u,port:%u,errmsg:%d,len:%d.", ip, port, WSAGetLastError(), len);
		is_fail = true;
	}
#else
	if (ret < 0) {
        if (errno == EAGAIN) {
            XMDLoggerWrapper::instance()->debug("XMDSendThread first send fail try to resend.");
            usleep(100);
            int resend_ret = sendto(listenfd_, data, len, MSG_DONTWAIT, (struct sockaddr*)&addr, addrLen);
            if (resend_ret < 0) {
                XMDLoggerWrapper::instance()->warn("XMDSendThread resend data fail, ip:%u,port:%u,errmsg:%s,len:%d.", ip, port, strerror(errno), len);
                is_fail = true;
            }
        } else {
            XMDLoggerWrapper::instance()->warn("XMDSendThread send data fail, ip:%u,port:%u,errmsg:%s,len:%d.", ip, port, strerror(errno), len);
            is_fail = true;
        }
    } else {
        is_reset_socket_ = true;
        send_fail_count_ = 0;
        dispatcher_->setIsCallBackSocketErr(true);
    }
#endif

    if (is_fail && is_reset_socket_) {
        send_fail_count_++;
        if (send_fail_count_ >= 2) {
            is_reset_socket_ = false;
            dispatcher_->handleSocketError(errno, "socket send err");
        }
        if (!stopFlag_ && transceiver_->resetSocket() < 0) {
            is_reset_socket_ = false;
        }        
    }


    //XMDLoggerWrapper::instance()->debug("XMDSendThread send data, len:%d, ip:%u,port:%u.", len, ip, port);
}

void XMDSendThread::stop(){ 
	stopFlag_ = true; 
#ifdef _WIN32
	closesocket(listenfd_);
	WSACleanup();
#endif
}


