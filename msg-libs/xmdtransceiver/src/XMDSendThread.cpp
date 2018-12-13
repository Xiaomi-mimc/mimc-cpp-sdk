#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sstream>
#include <memory>
#include <cstring>

#include "XMDSendThread.h"
#include "common.h"
#include "PacketBuilder.h"
#include "XMDLoggerWrapper.h"

XMDSendThread::XMDSendThread(int fd, int port, XMDCommonData* commonData, PacketDispatcher* dispactcher) {
    stopFlag_ = false;
    listenfd_ = fd;
    port_ = port;
    commonData_ = commonData;
    dispatcher_ = dispactcher;
    testPacketLoss_ = 0;
}

XMDSendThread::XMDSendThread() {
    commonData_ = NULL;
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
        SendQueueData* datagram = commonData_->datagramQueuePop();
        if (datagram != NULL) {
            isSleep = false;
            send(datagram->ip, datagram->port, (char*)datagram->data, datagram->len);
            delete datagram;
        }
 
        if (!commonData_->resendQueueEmpty()) {
            ResendData* resendData = commonData_->resendQueuePop();
            if (resendData != NULL) {
                isSleep = false;
                if (resendData->reSendCount > 0 || resendData->reSendCount == -1) {
                    XMDLoggerWrapper::instance()->debug("resend,connid=%ld, packet id=%ld", 
                                                         resendData->connId, resendData->packetId);
                    send(resendData->ip, resendData->port, (char*)resendData->data, resendData->len);
                    uint64_t current_time = current_ms();
                    resendData->reSendTime = current_time + getResendTimeInterval(1);
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
                         std::stringstream ss;
                         ss << packetInfo.connId << packetInfo.streamId << packetInfo.groupId;
                         std::string key = ss.str();
                         if (commonData_->SendCallbackMapRecordExist(key)) {
                             commonData_->deleteSendCallbackMap(key);
                             dispatcher_->streamDataSendFail(packetInfo.connId, packetInfo.streamId, 
                                                             packetInfo.groupId, packetInfo.ctx);
                         }
                     }

                     if (resendData->packetId == 0) {
                         XMDLoggerWrapper::instance()->warn("conn create fail, didn't recv resp,connid=%ld", resendData->connId);
                         ConnInfo connInfo;
                         if(commonData_->getConnInfo(resendData->connId, connInfo)){
                             dispatcher_->handleCreateConnFail(resendData->connId, connInfo.ctx);
                             commonData_->deleteConn(resendData->connId);
                         }
                     }

                     //commonData_->deleteConn(ackPacket.connId);
                     //dispatcher_->handleCloseConn(ackPacket.connId, CLOSE_SEND_FAIL);
    
                     
                     XMDLoggerWrapper::instance()->debug("packet resend fail, drop it,connid=%ld, packet id=%ld", 
                                                          resendData->connId, resendData->packetId);
                     delete resendData;
                }
            }
        }

        if (isSleep) {
            usleep(1000);
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

    int ret = sendto(listenfd_, data, len, MSG_DONTWAIT, (struct sockaddr*)&addr, addrLen);
    if (ret < 0) {
        XMDLoggerWrapper::instance()->warn("XMDSendThread send fail, ip:%u,port:%u,errmsg:%s,len:%d.", ip, port, strerror(errno), len);
        return;
    }

    //XMDLoggerWrapper::instance()->debug("XMDSendThread send data, len:%d, ip:%u,port:%u.", len, ip, port);
}


