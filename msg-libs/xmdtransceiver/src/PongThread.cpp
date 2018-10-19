#include "PongThread.h"
#include "XMDLoggerWrapper.h"
#include "XMDPacket.h"
#include <sstream>
#include <unistd.h>


PongThread::PongThread(XMDCommonData* commonData) {
    commonData_ = commonData;
    stopFlag_ = false;
}

PongThread::~PongThread() {}


void* PongThread::process() {
    while(!stopFlag_) {
        usleep(1000);
        PongThreadData data;
        if (commonData_->pongThreadQueuePop(data)) {
            PacketLossInfo packetLossInfo;
            if (commonData_->getPacketLossInfo(data.connId, packetLossInfo)) {
                XMDPong pong;
                pong.connId = data.connId;
                pong.packetId = commonData_->getPakcetId(data.connId);
                pong.ackedPacketId = data.packetId;
                pong.timeStamp1 = data.ts;
                pong.totalPackets = packetLossInfo.caculatePacketId - packetLossInfo.minPacketId;
                pong.recvPackets = packetLossInfo.oldPakcetCount;
                XMDPacketManager packetMan;

                ConnInfo connInfo;
                if(commonData_->getConnInfo(data.connId, connInfo)) {
                    packetMan.buildXMDPong(pong, true, connInfo.sessionKey);
                    XMDPacket *xmddata = NULL;
                    int packetLen = 0;
                    if (packetMan.encode(xmddata, packetLen) != 0) {
                        continue;
                    }
                    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
                    commonData_->socketSendQueuePush(sendData);
                    XMDLoggerWrapper::instance()->debug("connection(%ld) send pong,total packets:%d,recv packets:%d", 
                                                         data.connId, pong.totalPackets, pong.recvPackets);
                }
            }
        }
    }

    return NULL;
}

void PongThread::stop() {
    stopFlag_ = true;
}



