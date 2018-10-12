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
#include "LoggerWrapper.h"

XMDSendThread::XMDSendThread(int fd, int port, XMDCommonData* commonData, PacketDispatcher* dispactcher) {
    stopFlag_ = false;
    listenfd_ = fd;
    port_ = port;
    commonData_ = commonData;
    dispatcher_ = dispactcher;
    testNetFlag_ = false;
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
                send(resendData->ip, resendData->port, (char*)resendData->data, resendData->len);
                if (resendData->sendCount < MAX_SEND_TIME) {
                    uint64_t current_time = current_ms();
                    resendData->lastSendTime = current_time;
                    resendData->reSendTime = current_time + RESEND_DATA_INTEVAL;
                    resendData->sendCount++;
                    commonData_->resendQueuePush(resendData);
                } else {
                     std::stringstream ss_ack;
                     ss_ack << resendData->connId << resendData->packetId;
                     std::string ackpacketKey = ss_ack.str();
                     ackPacketInfo ackPacket;
                     if (commonData_->getDeleteAckPacketInfo(ackpacketKey, ackPacket)) {
                         std::stringstream ss;
                         ss << ackPacket.connId << ackPacket.streamId << ackPacket.groupId;
                         std::string key = ss.str();
                         commonData_->deleteSendCallbackMap(key);
                         dispatcher_->streamDataSendFail(ackPacket.connId, ackPacket.streamId, 
                                                         ackPacket.groupId, ackPacket.ctx);
                     }

                     commonData_->deleteConn(ackPacket.connId);
                     dispatcher_->handleCloseConn(ackPacket.connId, CLOSE_SEND_FAIL);
    
                    delete resendData;
                    LoggerWrapper::instance()->debug("packet resend %d times, drop it,packet id=%ld", 
                                                     MAX_SEND_TIME, resendData->packetId);
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
    if (testNetFlag_) {
        LoggerWrapper::instance()->warn("test do not send data");
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
        LoggerWrapper::instance()->warn("XMDSendThread send fail, ip:%u,port:%u,errmsg:%s,len:%d.", ip, port, strerror(errno), len);
        return;
    }

    LoggerWrapper::instance()->debug("XMDSendThread send data, len:%d, ip:%u,port:%u.", len, ip, port);
    //std::cout<<"time="<<current_ms()<<std::endl;
}


