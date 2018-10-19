#include "PingThread.h"
#include "XMDLoggerWrapper.h"
#include "XMDPacket.h"
#include <sstream>


PingThread::PingThread(PacketDispatcher* dispatcher, XMDCommonData* commonData) {
    dispatcher_ = dispatcher;
    commonData_ = commonData;
    stopFlag_ = false;
}

PingThread::~PingThread() {}

void* PingThread::process() {
    uint64_t pingTime = current_ms();
    bool isPing = false;
    while(!stopFlag_) {
        uint64_t currentTime = current_ms();
        if (currentTime - pingTime >= PING_INTERVAL) {
            isPing = true;
        }
        std::vector<uint64_t> connVec = commonData_->getConnVec();
        for (size_t i = 0; i < connVec.size(); i++) {
            uint64_t connId = connVec[i];
            ConnInfo connInfo;
            if(!commonData_->getConnInfo(connId, connInfo)){
                commonData_->deleteFromConnVec(connId);
                continue;
            } 
            
            if (!connInfo.connState == CONNECTED) {
                continue;
            }

            if (isPing) {
                sendPing(connId, connInfo);
            }
            checkTimeout(connId, connInfo);
        }

        if (isPing) {
            pingTime = currentTime;
        }
        isPing = false;
        
        usleep(1000000);
    }

    return NULL;
}

int PingThread::sendPing(uint64_t connId, ConnInfo connInfo) {
    XMDPacketManager packetMan;
    uint64_t packetId = commonData_->getPakcetId(connId);
    packetMan.buildXMDPing(connId, true, connInfo.sessionKey, packetId);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return -1;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);

    PingPacket ping;
    ping.connId = connId;
    ping.packetId = packetId;
    ping.sendTime = current_ms();
    
    commonData_->insertPing(ping);
    return 0;
}


int PingThread::checkTimeout(uint64_t conn_id, ConnInfo connInfo) {
    uint64_t currentTime = current_ms();
    std::stringstream ss_conn;
    ss_conn << conn_id;
    std::string connKey = ss_conn.str();
    uint64_t connLastPacketTime = commonData_->getLastPacketTime(connKey);

    if ((connLastPacketTime != 0) &&((int)(currentTime - connLastPacketTime) >= connInfo.timeout * 1000)) {
        commonData_->deleteConn(conn_id);
        dispatcher_->handleCloseConn(conn_id, CLOSE_TIMEOUT);
        XMDLoggerWrapper::instance()->debug("connection(%ld) timeout", conn_id);
    } else {
        std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.begin();
        for (; it != connInfo.streamMap.end(); it++) {
            std::stringstream ss_stream;
            ss_stream << conn_id << it->first;
            std::string streamKey = ss_stream.str();
            uint64_t streamLastPacketTime = commonData_->getLastPacketTime(streamKey);
            if ((streamLastPacketTime != 0) && ((int)(currentTime - streamLastPacketTime) >= it->second.timeout * 1000)) {
                commonData_->deleteStream(conn_id, it->first);
                XMDLoggerWrapper::instance()->debug("connection(%ld) stream(%d) timeout", conn_id, it->first);
            }
        }
    }
    return 0;
}


void PingThread::stop() {
    stopFlag_ = true;
}


