#include "PingThread.h"
#include "XMDLoggerWrapper.h"
#include "XMDPacket.h"
#include <sstream>
#include <chrono>
#include <thread>

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
        if (currentTime - pingTime >= (commonData_->GetPingTimeInterval() * 1000)) {
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
		std::this_thread::sleep_for(std::chrono::seconds(1));
        //usleep(1000000);
    }

    return NULL;
}

int PingThread::sendPing(uint64_t connId, ConnInfo connInfo) {
    XMDPacketManager packetMan;
    uint64_t packetId = commonData_->getPakcetId(connId);
    packetMan.buildXMDPing(connId, false, connInfo.sessionKey, packetId);
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
    } 
    
    return 0;
}


void PingThread::stop() {
    stopFlag_ = true;
}


