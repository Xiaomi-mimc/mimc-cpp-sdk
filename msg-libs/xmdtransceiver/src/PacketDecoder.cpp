#include "PacketDecoder.h"
#include "XMDLoggerWrapper.h"
#include <sstream>


PacketDecoder::PacketDecoder(PacketDispatcher* dispatcher, XMDCommonData* commonData, WorkerCommonData* workerCommonData, PacketRecover* packetRecover) {
    dispatcher_ = dispatcher;
    commonData_ = commonData;
    workerCommonData_ = workerCommonData;
    packetRecover_ = packetRecover;
}

PacketDecoder::~PacketDecoder() {
    dispatcher_ = NULL;
    commonData_ = NULL;
}


void PacketDecoder::decode(uint32_t ip, int port, char* data, int len) {
    XMDPacketManager packetMan;
    XMDPacket* xmdPacket = packetMan.decode(data, len);
    if (NULL == xmdPacket) {
        return;
    }
    XMDType xmdType = (XMDType)xmdPacket->getType();
    int packetLen = len -  sizeof(XMDPacket) - XMD_CRC_LEN;
    if (xmdType == DATAGRAM) {
        const int IP_STR_LEN = 32;
        char ipStr[IP_STR_LEN];
        memset(ipStr, 0, IP_STR_LEN);
        inet_ntop(AF_INET, &ip, ipStr, IP_STR_LEN);
        dispatcher_->handleRecvDatagram(ipStr, port, (char*)xmdPacket->data, packetLen, xmdPacket->getPacketType());
    } else if (xmdType == STREAM) {
        PacketType packetType = (PacketType)xmdPacket->getPacketType();
        uint64_t connId = 0;
        if (packetType == CONN_BEGIN) {
            //uint64_t* tmpId = (uint64_t*)(xmdPacket->data + 2);  //VERSION len
            uint64_t tmpId = 0;
            trans_uint64_t(tmpId, (char*)(xmdPacket->data + 2));
            connId = xmd_ntohll(tmpId);
        } else {
            //uint64_t* tmpId = (uint64_t*)xmdPacket->data;
            uint64_t tmpId = 0;
            trans_uint64_t(tmpId, (char*)xmdPacket->data);
            connId = xmd_ntohll(tmpId);
        }

        XMDLoggerWrapper::instance()->debug("packe decode packet type=%d", packetType);
        
    
        ConnInfo connInfo;
        if(workerCommonData_->getConnInfo(connId, connInfo)){
            if ((packetType != CONN_BEGIN) && (ip != connInfo.ip || port != connInfo.port)) {
                XMDLoggerWrapper::instance()->warn("conn(%ld) ip/port changed, origin ip=%d, port=%d, current ip=%d, port=%d",
                                             connId, connInfo.ip, connInfo.port, ip, port);
                
                workerCommonData_->updateConnIpInfo(connId, ip, port);

                const int IP_STR_LEN = 32;
                char tmpip[IP_STR_LEN];
                memset(tmpip, 0, IP_STR_LEN);
                inet_ntop(AF_INET, &ip, tmpip, IP_STR_LEN);
                int ipLen = 0;
                for (; ipLen < 16; ipLen++) {
                    if (tmpip[ipLen] == '\0') {
                        break;
                    }
                }
                std::string ipStr(tmpip, ipLen);
                dispatcher_->handleConnIpChange(connId, ipStr, port);
            } else {
                 if ((packetType == CONN_BEGIN) && (ip != connInfo.ip || port != connInfo.port)) {
                     XMDLoggerWrapper::instance()->info("conn id conflict, drop create conn packet, id=%ld", connId);
                     return;
                 }
            }
        } else {
             if (packetType == FEC_STREAM_DATA || packetType == PING || packetType == PONG || packetType == BATCH_ACK
                 || packetType == ACK_STREAM_DATA || packetType == BATCH_ACK_STREAM_DATA || packetType == ACK || packetType == STREAM_END) {
                 XMDLoggerWrapper::instance()->warn("conn reset type=%d", packetType);
                 sendConnReset(ip, port, connId, CONN_NOT_EXIST);
                 return;
             }
        }
        
        switch (packetType)
        {
            case CONN_BEGIN: {
                handleNewConn(ip, port, xmdPacket->data, packetLen);
                break;
            }
            case CONN_RESP_SUPPORT: {
                handleConnResp(ip, port, xmdPacket->data, packetLen);
                break;
            }
            case CONN_CLOSE: {
                handleConnClose(xmdPacket->data, packetLen);
                break;
            }
            case CONN_RESET: {
                handleConnReset(xmdPacket->data, packetLen);
                break;
            }
            case STREAM_END: {
                handleCloseStream(connInfo, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case FEC_STREAM_DATA: {
                handleFECStreamData(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case ACK: {
                handleStreamDataAck(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case BATCH_ACK: {
                handleBatchAck(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case PING: {
                handlePing(connId, connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case PONG: {
                handlePong(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case ACK_STREAM_DATA: {
                handleAckStreamData(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign(), packetType);
                break;
            }
            case TEST_RTT_PACKET: {
                handleTestRttData(connId, connInfo, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case BATCH_ACK_STREAM_DATA: {
                handleAckStreamData(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign(), packetType);
                break;
            }
            
            default: {
                XMDLoggerWrapper::instance()->warn("unknow packet type:%d", packetType);
                break;
            }
        }
    } else {
        XMDLoggerWrapper::instance()->warn("unknow xmdType:%d", xmdType);
    }
}

void PacketDecoder::handleNewConn(uint32_t ip, int port, unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDConnection* conn = packetMan.decodeNewConn(data, len);
    if (NULL == conn) {
        return;
    }

    bool isConnExist = false;
    if (workerCommonData_->isConnExist(conn->GetConnId())) {
        ConnInfo conninfo;
        workerCommonData_->getConnInfo(conn->GetConnId(), conninfo);
        if (!(conninfo.port == port && conninfo.ip == ip)) {
            sendConnReset(ip, port, conn->GetConnId(), CONN_ID_CONFLICT);
            return;
        } else {
            isConnExist = true;
        }
    }


    ConnInfo connInfo;
    connInfo.ip = ip;
    connInfo.port = port;
    connInfo.timeout = conn->GetTimeout();
    connInfo.max_stream_id = 0;
    connInfo.connState = CONNECTED;
    connInfo.sessionKey = "";
    int32_t pingInterval = (unsigned int)(conn->GetTimeout() / 2.5) * 1000;
    connInfo.pingInterval = pingInterval < PING_INTERVAL ? PING_INTERVAL : pingInterval;
    
    if (!isConnExist) {
        workerCommonData_->insertConn(conn->GetConnId(), connInfo);
        commonData_->addConnStreamId(conn->GetConnId(), -1);
    }

    
    commonData_->SetPingIntervalMs(conn->GetConnId(), pingInterval);
    
    
    packetMan.buildConnResp(conn->GetConnId(), NULL, 0);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);
    XMDLoggerWrapper::instance()->info("recv new connetion id=%ld, %lu", conn->GetConnId(), conn->GetConnId());
    if (!isConnExist) {
        dispatcher_->handleNewConn(conn->GetConnId(), (char*)conn->GetData(), 
                                   len -  sizeof(XMDConnection) - conn->GetNLen() - conn->GetELen());

        WorkerData* workerData = new WorkerData();
        workerData->connId = conn->GetConnId();
        workerCommonData_->startTimer(conn->GetConnId(), WORKER_PING_TIMEOUT, commonData_->GetPingIntervalMs(conn->GetConnId()), (void*)workerData, sizeof(WorkerData));

        WorkerData* checkConnData = new WorkerData();
        checkConnData->connId = conn->GetConnId();
        workerCommonData_->startTimer(conn->GetConnId(), WORKER_CHECK_CONN_AVAILABLE, connInfo.timeout * 1000 + 1000, (void*)checkConnData, sizeof(WorkerData));
    }

    workerCommonData_->insertPacketLossInfoMap(conn->GetConnId());

    std::stringstream ss_conn;
    ss_conn << conn->GetConnId();
    std::string connKey = ss_conn.str();
    workerCommonData_->updateLastPacketTime(connKey, current_ms());
    return;
}

void PacketDecoder::handleConnResp(uint32_t ip, int port, unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDConnResp* connResp = packetMan.decodeConnResp(data, len);
    if (NULL == connResp) {
        return;
    }
    XMDLoggerWrapper::instance()->info("recv conn reaponse conn(%ld)", connResp->GetConnId());

    ConnInfo connInfo;
    if(!workerCommonData_->getConnInfo(connResp->GetConnId(), connInfo)){
        XMDLoggerWrapper::instance()->info("conn reaponse conn(%ld) not exist", connResp->GetConnId());
        sendConnReset(ip, port, connResp->GetConnId(), CONN_NOT_EXIST);
        return;
    }
    bool isconnected = false;
    if (connInfo.connState == CONNECTED) {
        XMDLoggerWrapper::instance()->debug("conn(%ld) already connected, repeated conn resp", connResp->GetConnId());
        isconnected = true;
    }
    std::stringstream ss;
    ss << connResp->GetConnId() << 0;
    std::string connKey = ss.str();

    if (workerCommonData_->deletePacketCallbackInfo(connKey)  && (!isconnected)) {
        connInfo.connState = CONNECTED;
        connInfo.sessionKey = "";
        workerCommonData_->updateConn(connResp->GetConnId(), connInfo);
        dispatcher_->handleConnCreateSucc(connResp->GetConnId(), connInfo.ctx);

        WorkerData* workerData = new WorkerData();
        workerData->connId = connResp->GetConnId();
        workerCommonData_->startTimer(connResp->GetConnId(), WORKER_PING_TIMEOUT, commonData_->GetPingIntervalMs(connResp->GetConnId()), (void*)workerData, sizeof(WorkerData));

        WorkerData* checkConnData = new WorkerData();
        checkConnData->connId = connResp->GetConnId();
        workerCommonData_->startTimer(connResp->GetConnId(), WORKER_CHECK_CONN_AVAILABLE, connInfo.timeout * 1000 + 1000, (void*)checkConnData, sizeof(WorkerData));
    }

    
    workerCommonData_->updateIsPacketRecvAckMap(connKey, true);

    workerCommonData_->insertPacketLossInfoMap(connResp->GetConnId());

    std::stringstream ss_conn;
    ss_conn << connResp->GetConnId();
    std::string connTmpKey = ss_conn.str();
    workerCommonData_->updateLastPacketTime(connTmpKey, current_ms());
}

void PacketDecoder::handleConnClose(unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDConnClose* connClose = packetMan.decodeConnClose(data, len);
    if (NULL == connClose) {
        return;
    }

    if (workerCommonData_->deleteConn(connClose->GetConnId()) != 0) {
        XMDLoggerWrapper::instance()->warn("Close conn invalid conn id:%ld", connClose->GetConnId());
        return;
    }
    dispatcher_->handleCloseConn(connClose->GetConnId(), CLOSE_NORMAL);
}

void PacketDecoder::handleFECStreamData(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    XMDPacketManager packetMan;
    XMDFECStreamData* streamData = packetMan.decodeFECStreamData(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == streamData) {
        return;
    }

    XMDLoggerWrapper::instance()->debug("recv fec stream packet,connid=%ld,packetid=%ld,streamid=%d,groupid=%d,pid=%d,sliceid=%d", 
                                         streamData->GetConnId(), streamData->GetPacketId(), streamData->GetStreamId(),
                                         streamData->GetGroupId(), streamData->PId, streamData->GetSliceId());

    uint64_t timestamp = current_ms();
    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamData->GetStreamId());
    
    if (it == connInfo.streamMap.end()) {
        dispatcher_->handleNewStream(streamData->GetConnId(), streamData->GetStreamId());
        StreamInfo streamInfo;
        streamInfo.sType =  FEC_STREAM;
        streamInfo.callbackWaitTimeout = 0;
        streamInfo.isEncrypt = isEncrypt;
        workerCommonData_->insertStream(streamData->GetConnId(), streamData->GetStreamId(), streamInfo);
        XMDLoggerWrapper::instance()->debug("stream timeout=%d", streamData->GetTimeout());
    } 

    

    workerCommonData_->updatePacketLossInfoMap(streamData->GetConnId(), streamData->GetPacketId());
    
    std::stringstream ss_conn;
    ss_conn << streamData->GetConnId();
    std::string connKey = ss_conn.str();
    workerCommonData_->updateLastPacketTime(connKey, timestamp);

    packetRecover_->insertFecStreamPacket(streamData, len);

    return;
}


void PacketDecoder::handleCloseStream(ConnInfo connInfo, unsigned char* data, int len, bool isEncrypt) {    
    XMDPacketManager packetMan;
    XMDStreamClose* streamClose = packetMan.decodeStreamClose(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == streamClose) {
        return;
    }

    if (!workerCommonData_->isStreamExist(streamClose->GetConnId(), streamClose->GetStreamId())) {
        XMDLoggerWrapper::instance()->warn("close stream packet invalid,stream does not exist." 
                                        "conn id:%ld, stream id:%d", 
                                    streamClose->GetConnId(), streamClose->GetStreamId());
        return;
    }
    dispatcher_->handleCloseStream(streamClose->GetConnId(), streamClose->GetStreamId());
}


void PacketDecoder::handleConnReset(unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDConnReset* connReset = packetMan.decodeConnReset(data, len);
    if (NULL == connReset) {
        return;
    }
    if (workerCommonData_->deleteConn(connReset->GetConnId()) != 0) {
        XMDLoggerWrapper::instance()->warn("reset conn invalid conn id:%ld", connReset->GetConnId());
        return;
    }
    
    dispatcher_->handleCloseConn(connReset->GetConnId(), CLOSE_CONN_RESET);

    std::stringstream ss;
    ss << connReset->GetConnId() << 0;
    std::string connKey = ss.str();
    workerCommonData_->updateIsPacketRecvAckMap(connKey, true);
}

void PacketDecoder::handleStreamDataAck(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    XMDPacketManager packetMan;
    XMDStreamDataAck* ack = packetMan.decodeStreamDataAck(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == ack) {
        return;
    }
    XMDLoggerWrapper::instance()->debug("recv ack packet,packetid=%ld, acked packet id=%ld,", ack->GetPacketId(), ack->GetAckedPacketId());

    workerCommonData_->updatePacketLossInfoMap(ack->GetConnId(), ack->GetPacketId());

    std::stringstream ss_ack;
    ss_ack << ack->GetConnId() << ack->GetAckedPacketId();
    std::string ackpacketKey = ss_ack.str();
    
    workerCommonData_->updateIsPacketRecvAckMap(ackpacketKey, true);

    PacketCallbackInfo packetCallbackInfo;
    if (workerCommonData_->getDeletePacketCallbackInfo(ackpacketKey, packetCallbackInfo)) {
        std::stringstream ss_callback;
        ss_callback << packetCallbackInfo.connId << packetCallbackInfo.streamId << packetCallbackInfo.groupId;
        std::string callbackKey = ss_callback.str();
        if (workerCommonData_->updateSendCallbackMap(callbackKey, packetCallbackInfo.sliceId) == 1) {
            dispatcher_->streamDataSendSucc(packetCallbackInfo.connId, packetCallbackInfo.streamId, packetCallbackInfo.groupId, packetCallbackInfo.ctx);
        }
    }
}

void PacketDecoder::handleBatchAck(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    if (data == NULL) {
        return;
    }

    if (len < (int)sizeof(XMDBatchAck)) {
        return;
    }

    XMDBatchAck* batchAck = (XMDBatchAck*)data;
    workerCommonData_->updatePacketLossInfoMap(batchAck->GetConnId(), batchAck->GetPacketId());

    char* ackedPacketId = (char*)(data + sizeof(XMDBatchAck));
    int ackedPacketNum = (len - sizeof(XMDBatchAck)) / sizeof(uint64_t);
    XMDLoggerWrapper::instance()->debug("batch ack packet num =%d", ackedPacketNum);
    for (int i = 0; i < ackedPacketNum; i++) {
        std::stringstream ss_ack;
        uint64_t tmp_acked_id = 0;
        trans_uint64_t(tmp_acked_id, ackedPacketId);
        ss_ack << batchAck->GetConnId() << xmd_ntohll(tmp_acked_id);
        std::string ackpacketKey = ss_ack.str();
        
        workerCommonData_->updateIsPacketRecvAckMap(ackpacketKey, true);
    
        PacketCallbackInfo packetCallbackInfo;
        if (workerCommonData_->getDeletePacketCallbackInfo(ackpacketKey, packetCallbackInfo)) {
            std::stringstream ss_callback;
            ss_callback << packetCallbackInfo.connId << packetCallbackInfo.streamId << packetCallbackInfo.groupId;
            std::string callbackKey = ss_callback.str();
            if (workerCommonData_->updateSendCallbackMap(callbackKey, packetCallbackInfo.sliceId) == 1) {
                dispatcher_->streamDataSendSucc(packetCallbackInfo.connId, packetCallbackInfo.streamId, packetCallbackInfo.groupId, packetCallbackInfo.ctx);
            }
        }

        ackedPacketId += sizeof(uint64_t);
    }
}


void PacketDecoder::handlePing(uint64_t connId, ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    uint64_t timestamp = current_ms();
    XMDPacketManager packetMan;
    XMDPing* ping = packetMan.decodeXMDPing(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == ping) {
        return;
    }

    PongThreadData* pongthreadData = new PongThreadData();
    pongthreadData->connId = connId;
    pongthreadData->packetId = ping->GetPacketId();
    pongthreadData->ts = timestamp;

    workerCommonData_->startTimer(connId, WORKER_PONG_TIMEOUT, SEND_PONG_WAIT_TIME, (void*)pongthreadData, sizeof(PongThreadData));

    workerCommonData_->updatePacketLossInfoMap(connId, ping->GetPacketId());
    workerCommonData_->startPacketLossCalucate(connId);

    std::stringstream ss_conn;
    ss_conn << connId;
    std::string connKey = ss_conn.str();
    workerCommonData_->updateLastPacketTime(connKey, timestamp);
}

void PacketDecoder::handlePong(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    uint64_t currentTime = current_ms();
    XMDPacketManager packetMan;
    XMDPong* pong = packetMan.decodeXMDPong(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == pong) {
        return;
    }

    workerCommonData_->updatePacketLossInfoMap(pong->GetConnId(), pong->GetPacketId());

    netStatus status = workerCommonData_->getNetStatus(pong->GetConnId());
    if (pong->GetTotalPackets() != 0 && pong->GetRecvPackets() <= pong->GetTotalPackets()) {
        status.totalPackets += pong->GetTotalPackets();
        status.tmpTotalPackets += pong->GetTotalPackets();
        status.tmpRecvPackets += pong->GetRecvPackets();
        if ((int)status.tmpTotalPackets >= NETSTATUS_MIN_PACKETS) {
            float currentPacketLossRate = 1 - float(status.tmpRecvPackets) / float(status.tmpTotalPackets);
            status.packetLossRate = RTT_SMOOTH_FACTOR * currentPacketLossRate + ( 1 - RTT_SMOOTH_FACTOR) * status.packetLossRate;
            status.tmpTotalPackets = 0;
            status.tmpRecvPackets = 0;
        }
    } else {
        //status.packetLossRate = 0;
    }
    PingPacket pingPakcet;
    if (workerCommonData_->getPingPacket(pong->GetConnId(), pingPakcet)) {
        if (pingPakcet.packetId == pong->GetAckedPacketId()) {
            int currentRtt = (currentTime - pingPakcet.sendTime - (pong->GetTimestamp2() - pong->GetTimestamp1())) / 2;
            if (currentRtt < 0) {
                XMDLoggerWrapper::instance()->warn("invalid rtt %d", currentRtt);
            } else {
                status.rtt = RTT_SMOOTH_FACTOR * currentRtt + ( 1 - RTT_SMOOTH_FACTOR) * status.rtt;
                workerCommonData_->updateResendInterval(pong->GetConnId(), status.rtt);
            }
        }
    }

    workerCommonData_->updateNetStatus(pong->GetConnId(), status);

    XMDLoggerWrapper::instance()->debug("connection(%ld) recv pong, packet loss rate:%f, rtt:%d, total:%d,recved:%d,ts0:%ld,ts1:%ld,ts2:%ld,ts3:%ld", 
                                                         pong->GetConnId(), status.packetLossRate, status.rtt,
                                                         pong->GetTotalPackets(),pong->GetRecvPackets(),
                                                         pingPakcet.sendTime,pong->GetTimestamp1(),pong->GetTimestamp2(),currentTime);

    std::stringstream ss_conn;
    ss_conn << pong->GetConnId();
    std::string connKey = ss_conn.str();
    workerCommonData_->updateLastPacketTime(connKey, currentTime);
}

void PacketDecoder::handleAckStreamData(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt, PacketType packetType) {
    XMDPacketManager packetMan;
    XMDACKStreamData* streamData = packetMan.decodeAckStreamData(data, len, isEncrypt, connInfo.sessionKey);
    if (streamData == NULL) {
        return;
    }
    XMDLoggerWrapper::instance()->debug("recv ack stream packet,connid=%ld,packetid=%ld", 
                                         streamData->GetConnId(), streamData->GetPacketId());
                                         
    uint64_t timestamp = current_ms();
    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamData->GetStreamId());
    
    if (it == connInfo.streamMap.end()) {
        dispatcher_->handleNewStream(streamData->GetConnId(), streamData->GetStreamId());
        StreamInfo streamInfo;
        streamInfo.sType = (StreamType)packetType;
        streamInfo.callbackWaitTimeout = streamData->GetWaitTime();
        streamInfo.isEncrypt = isEncrypt;
        workerCommonData_->insertStream(streamData->GetConnId(), streamData->GetStreamId(), streamInfo);
    }

    workerCommonData_->updatePacketLossInfoMap(streamData->GetConnId(), streamData->GetPacketId());

    std::stringstream ss_conn;
    ss_conn << streamData->GetConnId();
    std::string connKey = ss_conn.str();
    workerCommonData_->updateLastPacketTime(connKey, timestamp);

    packetRecover_->insertAckStreamPacket(streamData, len);

    if (packetType == ACK_STREAM_DATA) {
        uint64_t packetId = workerCommonData_->getPakcetId(streamData->GetConnId());
        packetMan.buildStreamDataAck(streamData->GetConnId(), packetId, streamData->GetPacketId(), 
                                     isEncrypt, connInfo.sessionKey);
        XMDPacket *xmddata = NULL;
        int packetLen = 0;
        if (packetMan.encode(xmddata, packetLen) != 0) {
            return;
        }
        SendQueueData* sendData = new SendQueueData(ip, port, (unsigned char*)xmddata, packetLen);
        commonData_->socketSendQueuePush(sendData);
    } else {
        int ret = workerCommonData_->insertBatchAckMap(streamData->GetConnId(), streamData->GetPacketId());
        if (ret == 1) {
            WorkerData* bacthAckData = new WorkerData();
            bacthAckData->connId = streamData->GetConnId();
            workerCommonData_->startTimer(streamData->GetConnId(), WORKER_BATCH_ACK, DELAY_BATCH_ACK_MS, (void*)bacthAckData, sizeof(WorkerData));
        }
    }
}


void PacketDecoder::handleTestRttData(uint64_t connId, ConnInfo connInfo, unsigned char* data, int len, bool isEncrypt) {
    XMDPacketManager packetMan;
    XMDPing* rttTestdata = packetMan.decodeXMDPing(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == rttTestdata) {
        return;
    }

    workerCommonData_->updatePacketLossInfoMap(connId, rttTestdata->GetPacketId());

    std::stringstream ss_conn;
    ss_conn << connId;
    std::string connKey = ss_conn.str();
    uint64_t timestamp = current_ms();
    workerCommonData_->updateLastPacketTime(connKey, timestamp);
}



void PacketDecoder::sendConnReset(uint32_t ip, int port, uint64_t conn_id, ConnResetType type) {
    XMDPacketManager packetMan;
    packetMan.buildConnReset(conn_id, type);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return;
    }
    SendQueueData* sendData = new SendQueueData(ip, port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);
    XMDLoggerWrapper::instance()->warn("send conn(%ld) reset.type=%d", conn_id, type);
}

