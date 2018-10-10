#include "PingThread.h"
#include "LoggerWrapper.h"
#include "XMDPacket.h"
#include <sstream>


PingThread::PingThread(PacketDispatcher* dispatcher, XMDCommonData* commonData) {
    dispatcher_ = dispatcher;
    commonData_ = commonData;
    stopFlag_ = false;
    pingCount_ = 0;
}

PingThread::~PingThread() {}

void* PingThread::process() {
    //int pingCountTimeout = 0;
    while(!stopFlag_) {
        /*pingCount_++;
        pingCountTimeout++;
        std::vector<uint64_t> connVec = commonData_->getConnVec();
        for (size_t i = 0; i < connVec.size(); i++) {
            uint64_t connId = connVec[i];
            ConnInfo connInfo;
            if(!commonData_->getConnInfo(connId, connInfo)){
                continue;
            }
            if (!connInfo.isConnected) {
                continue;
            }
    
            sendPing(connId, connInfo);
            if (pingCount_ >= CALCULATE_PACKET_LOSS_INTERVAL / PING_INTERVAL) {
                double packetLossRate = 0;
                int packetttl = 0;
                if (commonData_->calculatePacketLoss(connId, packetLossRate, packetttl) == 0) {
                    LoggerWrapper::instance()->debug("connection(%ld) packet loss rate:%f, ttl=%dms", 
                                                     connId, packetLossRate, packetttl);
                }
                pingCount_ = 0;

                
                netStatus status;
                status.packetLossRate = packetLossRate;
                status.ttl = packetttl;
                commonData_->updateNetStatus(connId, status);
            }

            if (pingCountTimeout >= CHECKOUT_CONN_TIMEOUT_INTERVAL / PING_INTERVAL) {
                checkTimeout(connId, connInfo);
                pingCountTimeout = 0;
            }
        }*/
        
        
        usleep(PING_INTERVAL * 1000);
    }

    return NULL;
}

int PingThread::sendPing(uint64_t connId, ConnInfo connInfo) {
    XMDPacketManager packetMan;
    uint64_t packetId = commonData_->getPakcetId(connId);
    packetMan.buildXMDPing(connId, connInfo.isEncrypt, connInfo.sessionKey, packetId);
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
    ping.ttl = 0;
    
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
        LoggerWrapper::instance()->debug("connection(%ld) timeout", conn_id);
    } else {
        std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.begin();
        for (; it != connInfo.streamMap.end(); it++) {
            std::stringstream ss_stream;
            ss_stream << conn_id << it->first;
            std::string streamKey = ss_stream.str();
            uint64_t streamLastPacketTime = commonData_->getLastPacketTime(streamKey);
            if ((streamLastPacketTime != 0) && ((int)(currentTime - streamLastPacketTime) >= it->second.timeout * 1000)) {
                commonData_->deleteStream(conn_id, it->first);
                LoggerWrapper::instance()->debug("connection(%ld) stream(%d) timeout", conn_id, it->first);
            }
        }
    }
    return 0;
}


void PingThread::stop() {
    stopFlag_ = true;
}


