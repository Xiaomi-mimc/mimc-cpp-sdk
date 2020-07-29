#include "XMDWorkerThread.h"
#include "XMDLoggerWrapper.h"
#ifdef _WIN32
#include <stdlib.h>
#else
#include <unistd.h>
#endif // _WIN32
#include <sstream>


WorkerThread::WorkerThread(int id, XMDCommonData* commonData, PacketDispatcher* dispatcher) {
    commonData_ = commonData;
    dispatcher_ = dispatcher;
    workerCommonData_ = new WorkerCommonData(commonData_);
    packetCallback_ = new PacketCallback(dispatcher_, commonData, workerCommonData_);
    packetRecover_ = new PacketRecover(commonData, workerCommonData_, packetCallback_);
    packetDecoder_ = new PacketDecoder(dispatcher, commonData, workerCommonData_, packetRecover_);
    stopFlag_ = false;
    thread_id_ = id;
}

WorkerThread::~WorkerThread() {
    if (packetDecoder_) {
        delete packetDecoder_;
    }

    if (packetRecover_) {
        delete packetRecover_;
    }

    if (packetCallback_) {
        delete packetCallback_;
    }
    
    if (workerCommonData_) {
        delete workerCommonData_;
    }    
}

void* WorkerThread::process() {
    char tname[16];
    memset(tname, 0, 16);
    strcpy(tname, "xmd worker");
#ifdef _WIN32
#else
#ifdef _IOS_MIMC_USE_ 
	pthread_setname_np(tname);
#else           
	prctl(PR_SET_NAME, tname);
#endif
#endif // _WIN32
    
    while(!stopFlag_) {
        WorkerThreadData* workerThreadData = commonData_->workerQueuePop(thread_id_);
        if (workerThreadData == NULL) {
            usleep(1000);
            XMDLoggerWrapper::instance()->debug("workerThreadData is null");
            continue;
        }
        HandleWorkerThreadData(workerThreadData);
        delete workerThreadData;
    }

    return NULL;
}

void WorkerThread::HandleWorkerThreadData(WorkerThreadData* workerThreadData) {
    switch (workerThreadData->dataType)
    {
        case WORKER_CREATE_NEW_CONN: {
            CreateConnection(workerThreadData->data);
            break;
        }
        case WORKER_CLOSE_CONN: {
            CloseConnection(workerThreadData->data);
            break;
        }
        case WORKER_CREATE_STREAM: {
            CreateStream(workerThreadData->data);
            break;
        }
        case WORKER_CLOSE_STREAM: {
            CloseStream(workerThreadData->data);
            break;
        }
        case WORKER_SEND_RTDATA: {
            SendRTData(workerThreadData->data);
            break;
        }
        case WORKER_RECV_DATA: {
            RecvData(workerThreadData->data);
            break;
        }
        case WORKER_RESEND_TIMEOUT: {
            HandleResendTimeout(workerThreadData->data);
            break;
        }
        case WORKER_DATAGREAM_TIMEOUT: {
            HandleDatagramTimeout(workerThreadData->data);
            break;
        }
        case WORKER_DELETE_ACKGROUP_TIMEOUT: {
            HandleDeleteAckGroupTimeout(workerThreadData->data);
            break;
        }
        case WORKER_DELETE_FECGROUP_TIMEOUT: {
            HandleDeleteFECGroupTimeout(workerThreadData->data);
            break;
        }
        case WORKER_CHECK_CALLBACK_BUFFER: {
            HandleCheckCallbackBuffer(workerThreadData->data);
            break;
        }
        case WORKER_PING_TIMEOUT: {
            HandlePingTimeout(workerThreadData->data);
            break;
        }
        case WORKER_PONG_TIMEOUT: {
            HandlePongTimeout(workerThreadData->data);
            break;
        }
        case WORKER_CHECK_CONN_AVAILABLE: {
            HandleCheckConn(workerThreadData->data);
            break;
        }
        case WORKER_TEST_RTT: {
            HandleTestRtt(workerThreadData->data);
            break;
        }
        case WORKER_BATCH_ACK: {
            HandleSendBatchAck(workerThreadData->data);
            break;
        }
        default: {
            XMDLoggerWrapper::instance()->warn("unknow wroker thread packet type:%d", workerThreadData->dataType);
            break;
        }
    }
}

void WorkerThread::CreateConnection(void* data) {
    if (data == NULL) {
        return;
    }
    NewConnWorkerData* connData = (NewConnWorkerData*)data;
    workerCommonData_->insertConn(connData->connInfo.connId, connData->connInfo);
    
    XMDPacketManager packetMan;
    packetMan.buildConn(connData->connInfo.connId, connData->data, connData->len, connData->connInfo.timeout, 0, NULL, 0, NULL, false);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        delete connData;
        return;
    }

    SendQueueData* sendData = new SendQueueData(connData->connInfo.ip, connData->connInfo.port, (unsigned char*)xmddata, 
                                                packetLen, connData->connInfo.connId, 0, CREATE_CONN_RESEND_TIME, 
                                                workerCommonData_->getResendInterval(connData->connInfo.connId));

    commonData_->socketSendQueuePush(sendData);

    std::stringstream ss_ack;
    ss_ack << connData->connInfo.connId << 0;
    std::string ackpacketKey = ss_ack.str();
    workerCommonData_->insertIsPacketRecvAckMap(ackpacketKey, false);

    PacketCallbackInfo packetInfo;
    packetInfo.connId = connData->connInfo.connId;
    packetInfo.streamId = 0;
    packetInfo.packetId = 0;
    packetInfo.groupId = 0;
    packetInfo.sliceId = 0;
    packetInfo.ctx = connData->connInfo.ctx;
    workerCommonData_->insertPacketCallbackInfoMap(ackpacketKey, packetInfo);

    delete connData;
}

void WorkerThread::CreateStream(void* data){
    if (data == NULL) {
        return;
    }
    NewStreamWorkerData* workerData = (NewStreamWorkerData*)data;

    StreamInfo streamInfo;
    streamInfo.sType = workerData->sType;
    streamInfo.isEncrypt = workerData->isEncrypt;
    streamInfo.callbackWaitTimeout = workerData->callbackWaitTimeout;
    workerCommonData_->insertStream(workerData->connId, workerData->streamId, streamInfo);
    
    delete workerData;
}

void WorkerThread::CloseConnection(void* data){
    if (data == NULL) {
        return;
    }
    WorkerData* workerData = (WorkerData*)data;
    
    ConnInfo connInfo;
    if(!workerCommonData_->getConnInfo(workerData->connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("connection(%llu) not exist.", workerData->connId);
        delete workerData;
        return;
    }
    
    if (workerCommonData_->deleteConn(workerData->connId) != 0) {
        delete workerData;
        return;
    }

    XMDPacketManager packetMan;
    packetMan.buildConnClose(workerData->connId);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        delete workerData;
        return;
    }
    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);
    delete workerData;
}

void WorkerThread::CloseStream(void* data){
    if (data == NULL) {
        return;
    }
    WorkerData* workerData = (WorkerData*)data;
    if (workerCommonData_->deleteStream(workerData->connId, workerData->streamId) != 0) {
        delete workerData;
        return;
    }

    ConnInfo connInfo;
    if(!workerCommonData_->getConnInfo(workerData->connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("connection(%llu) not exist.", workerData->connId);
        delete workerData;
        return;
    }
    if (connInfo.connState != CONNECTED) {
        XMDLoggerWrapper::instance()->warn("connection(%ld) has not been established.", workerData->connId);
        delete workerData;
        return;
    }

    XMDPacketManager packetMan;
    packetMan.buildStreamClose(workerData->connId, workerData->streamId, false, connInfo.sessionKey);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        delete workerData;
        return;
    }
    
    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);
    delete workerData;
}

void WorkerThread::SendRTData(void* data){
    if (data == NULL) {
        XMDLoggerWrapper::instance()->warn("worker SendRTData data is null");
        return;
    }
    RTWorkerData* workerData = (RTWorkerData*)data;

    ConnInfo connInfo;
    if(!workerCommonData_->getConnInfo(workerData->connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("connection(%llu) not exist.", workerData->connId);
        dispatcher_->streamDataSendFail(workerData->connId, workerData->streamId, workerData->groupId, workerData->ctx);
        delete workerData;
        return;
    }
    
    StreamInfo sInfo;
    if(!workerCommonData_->getStreamInfo(workerData->connId, workerData->streamId, sInfo)) {
        XMDLoggerWrapper::instance()->warn("stream(%d) not exist.", workerData->streamId);
        dispatcher_->streamDataSendFail(workerData->connId, workerData->streamId, workerData->groupId, workerData->ctx);
        delete workerData;
        return;
    }


    PacketBuilder builder(commonData_, dispatcher_, workerCommonData_, thread_id_);
    builder.build(workerData, connInfo, sInfo);


    delete workerData;
}

void WorkerThread::RecvData(void* data){
    if (data == NULL) {
        XMDLoggerWrapper::instance()->warn("worker RecvData data is null");
        return;
    }
    SocketData* socketData = (SocketData*)data;
    packetDecoder_->decode(socketData->ip, socketData->port, (char*)socketData->data, socketData->len);
    delete socketData;
}


void WorkerThread::HandleResendTimeout(void* data) {
    if (data == NULL) {
        return;
    }
    SendQueueData* resendData = (SendQueueData*)data;
    std::stringstream ss_ack;
    ss_ack << resendData->connId << resendData->packetId;
    std::string ackpacketKey = ss_ack.str();

    ConnInfo connInfo;
    if (!workerCommonData_->getConnInfo(resendData->connId, connInfo)) {
        XMDLoggerWrapper::instance()->debug("connection(%llu) not exist,drop resend packet.", resendData->connId);

        workerCommonData_->deleteIsPacketRecvAckMap(ackpacketKey);
        PacketCallbackInfo packetInfo;
        if (workerCommonData_->getPacketCallbackInfo(ackpacketKey, packetInfo)) {
            if (resendData->packetId == 0) {
                XMDLoggerWrapper::instance()->warn("conn create fail, didn't recv resp,connid=%ld", resendData->connId);
                ConnInfo connInfo;
                if(workerCommonData_->getConnInfo(resendData->connId, connInfo) && workerCommonData_->deletePacketCallbackInfo(ackpacketKey)){
                    dispatcher_->handleCreateConnFail(resendData->connId, connInfo.ctx);
                    workerCommonData_->deleteConn(resendData->connId);
                }
            } else {
                std::stringstream ss;
                ss << packetInfo.connId << packetInfo.streamId << packetInfo.groupId;
                std::string key = ss.str();
                workerCommonData_->deletePacketCallbackInfo(ackpacketKey);
                if (workerCommonData_->getDeleteCallbackMapRecord(key)) {
                     dispatcher_->streamDataSendFail(packetInfo.connId, packetInfo.streamId, 
                                                      packetInfo.groupId, packetInfo.ctx);
                }
            }
        }
   
        delete resendData;
        return;
    }

    if ((connInfo.connState == CONNECTED) && (resendData->packetId == 0)) {
        delete resendData;
        workerCommonData_->deleteIsPacketRecvAckMap(ackpacketKey);
        workerCommonData_->deletePacketCallbackInfo(ackpacketKey);
        return;
    }

    if (workerCommonData_->getIsPacketRecvAckMapValue(ackpacketKey)) {
        delete resendData;
        workerCommonData_->deleteIsPacketRecvAckMap(ackpacketKey);
        workerCommonData_->deletePacketCallbackInfo(ackpacketKey);
        return;
    }

    if (resendData->reSendCount == 0) {
        workerCommonData_->deleteIsPacketRecvAckMap(ackpacketKey);
        PacketCallbackInfo packetInfo;
        if (workerCommonData_->getPacketCallbackInfo(ackpacketKey, packetInfo)) {
            if (resendData->packetId == 0) {
                XMDLoggerWrapper::instance()->warn("conn create fail, didn't recv resp,connid=%ld", resendData->connId);
                ConnInfo connInfo;
                if(workerCommonData_->getConnInfo(resendData->connId, connInfo) && workerCommonData_->deletePacketCallbackInfo(ackpacketKey)){
                    dispatcher_->handleCreateConnFail(resendData->connId, connInfo.ctx);
                    workerCommonData_->deleteConn(resendData->connId);
                }
            } else {
                XMDLoggerWrapper::instance()->info("TimerQueueUsedSize:%u,WorkerQueueUsedSize:%u,SocketSendQueueUsedSize:%u",
                        commonData_->getTimerQueueUsedSize(0), commonData_->getWorkerQueueUsedSize(0), commonData_->getSocketSendQueueUsedSize());
                std::stringstream ss;
                ss << packetInfo.connId << packetInfo.streamId << packetInfo.groupId;
                std::string key = ss.str();
                workerCommonData_->deletePacketCallbackInfo(ackpacketKey);
                if (workerCommonData_->getDeleteCallbackMapRecord(key)) {
                     dispatcher_->streamDataSendFail(packetInfo.connId, packetInfo.streamId, 
                                                      packetInfo.groupId, packetInfo.ctx);
                }
            }
        }
                     
        delete resendData;
        return;
    }

    if (resendData->reSendCount != -1) {
        resendData->reSendCount--;
    }

    if (resendData->reSendInterval < MAX_RESEND_INTERVAL) {
        resendData->reSendInterval = resendData->reSendInterval * 3 / 2;
        resendData->reSendInterval = resendData->reSendInterval > MAX_RESEND_INTERVAL ? MAX_RESEND_INTERVAL : resendData->reSendInterval;
    }


    commonData_->socketSendQueuePush(resendData);
}

void WorkerThread::HandleDatagramTimeout(void* data) {
    if (data == NULL) {
        return;
    }
    SendQueueData* queueData = (SendQueueData*)data;
    if (commonData_->isSocketSendQueueFull(queueData->len)) {
        XMDLoggerWrapper::instance()->warn("socket queue full %d drop datagram packet", commonData_->getSocketSendQueueUsedSize());
        delete queueData;
        return;
    }
    commonData_->socketSendQueuePush(queueData);
}

void WorkerThread::HandleDeleteAckGroupTimeout(void* data) {
    if (data == NULL) {
        return;
    }
    DeleteGroupTimeout* delteData = (DeleteGroupTimeout*)data;
    packetRecover_->deleteFromAckGroupMap(delteData->key);
    delete delteData;
}

void WorkerThread::HandleDeleteFECGroupTimeout(void* data) {
    if (data == NULL) {
        return;
    }
    DeleteGroupTimeout* delteData = (DeleteGroupTimeout*)data;
    packetRecover_->deleteFromFECGroupMap(delteData->key);
    delete delteData;
}

void WorkerThread::HandleCheckCallbackBuffer(void* data) {
    if (data == NULL) {
        return;
    }

    CheckCallbackBufferData* key = (CheckCallbackBufferData*)data;
    packetCallback_->CheckCallbackBuffer(key);
    delete key;
}

void WorkerThread::HandlePingTimeout(void* data) {
    XMDLoggerWrapper::instance()->debug("start to ping ");
    WorkerData* workerData = (WorkerData*)data;
    
    ConnInfo connInfo;
    if(!workerCommonData_->getConnInfo(workerData->connId, connInfo)){
        delete workerData;
        return;
    }
    XMDPacketManager packetMan;
    uint64_t packetId = workerCommonData_->getPakcetId(workerData->connId);
    packetMan.buildXMDPing(workerData->connId, false, connInfo.sessionKey, packetId);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return ;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);

    PingPacket ping;
    ping.connId = workerData->connId;
    ping.packetId = packetId;
    ping.sendTime = current_ms();
    workerCommonData_->insertPing(ping);

    workerCommonData_->startTimer(workerData->connId, WORKER_PING_TIMEOUT, commonData_->GetPingIntervalMs(workerData->connId), (void*)workerData, sizeof(WorkerData));
}

void WorkerThread::HandlePongTimeout(void* data) {
    PongThreadData* pongthreadData = (PongThreadData*)data;
    PacketLossInfo packetLossInfo;
    if (workerCommonData_->getPacketLossInfo(pongthreadData->connId, packetLossInfo)) {
        XMDPong pong;
        pong.connId = pongthreadData->connId;
        pong.packetId = workerCommonData_->getPakcetId(pongthreadData->connId);
        pong.ackedPacketId = pongthreadData->packetId;
        pong.timeStamp1 = pongthreadData->ts;
        pong.totalPackets = packetLossInfo.caculatePacketId - packetLossInfo.minPacketId;
        pong.recvPackets = packetLossInfo.oldPakcetSet.size();
        XMDPacketManager packetMan;

        ConnInfo connInfo;
        if(workerCommonData_->getConnInfo(pongthreadData->connId, connInfo)) {
            packetMan.buildXMDPong(pong, false, connInfo.sessionKey);
            XMDPacket *xmddata = NULL;
            int packetLen = 0;
            if (packetMan.encode(xmddata, packetLen) != 0) {
                return;
            }
            SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
            commonData_->socketSendQueuePush(sendData);
            XMDLoggerWrapper::instance()->debug("connection(%ld) send pong,total packets:%d,recv packets:%d", 
                                                 pongthreadData->connId, pong.totalPackets, pong.recvPackets);
        }
    }

    delete pongthreadData;
}

void WorkerThread::HandleCheckConn(void* data) {
    XMDLoggerWrapper::instance()->debug("check conn timeout");

    WorkerData* workerData = (WorkerData*)data;
    
    ConnInfo connInfo;
    if(!workerCommonData_->getConnInfo(workerData->connId, connInfo)){
        delete workerData;
        return;
    }

    uint64_t currentTime = current_ms();
    std::stringstream ss_conn;
    ss_conn << workerData->connId;
    std::string connKey = ss_conn.str();
    uint64_t connLastPacketTime = workerCommonData_->getLastPacketTime(connKey);

    if ((connLastPacketTime != 0) &&((int)(currentTime - connLastPacketTime) >= connInfo.timeout * 1000)) {
        workerCommonData_->deleteConn(workerData->connId);
        dispatcher_->handleCloseConn(workerData->connId, CLOSE_TIMEOUT);
        XMDLoggerWrapper::instance()->debug("connection(%ld) timeout", workerData->connId);
        delete workerData;
    } else {
        workerCommonData_->startTimer(workerData->connId, WORKER_CHECK_CONN_AVAILABLE, connInfo.pingInterval, (void*)workerData, sizeof(WorkerData));
    }
}

void WorkerThread::HandleTestRtt(void* data) {
    TestRttPacket* testRtt = (TestRttPacket*)data;
    if (testRtt->packetCount == 0) {
        delete testRtt;
        return;
    }

    ConnInfo connInfo;
    if(!workerCommonData_->getConnInfo(testRtt->connId, connInfo)){
        delete testRtt;
        return;
    }
    if (commonData_->isTimerQueueFull(0, sizeof(TestRttPacket) + sizeof(TimerThreadData) + sizeof(WorkerThreadData))) {
        XMDLoggerWrapper::instance()->warn("timer queue full drop test rtt packet");
        delete testRtt;
        return;
    }
    if (commonData_->isSocketSendQueueFull(sizeof(TestRttPacket))) {
        XMDLoggerWrapper::instance()->warn("socket queue full %d drop test rtt packet", commonData_->getSocketSendQueueUsedSize());
        delete testRtt;
        return;
    }
    XMDPacketManager packetMan;
    uint64_t packetId = workerCommonData_->getPakcetId(testRtt->connId);
    packetMan.buildXMDTestRttPacket(testRtt->connId, false, connInfo.sessionKey, packetId);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return ;
    }

    SendQueueData* queueData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(queueData);

    testRtt->packetCount--;
    WorkerThreadData* workerThreadData = new WorkerThreadData(WORKER_TEST_RTT, (void*)testRtt, sizeof(TestRttPacket));
    TimerThreadData* timerThreadData = new TimerThreadData();
    timerThreadData->connId = 0;
    timerThreadData->time = testRtt->delayMs + current_ms();
    timerThreadData->data = (void*)workerThreadData;
    timerThreadData->len =  sizeof(WorkerThreadData) + sizeof(TestRttPacket);
    commonData_->timerQueuePush(timerThreadData, 0);
}

void WorkerThread::HandleSendBatchAck(void* data) {
    WorkerData* bacthAckData = (WorkerData*)data;
    ConnInfo connInfo;
    if(!workerCommonData_->getConnInfo(bacthAckData->connId, connInfo)){
        delete bacthAckData;
        return;
    }
    std::vector<uint64_t> packetIdVec;
    if (workerCommonData_->getBatchAckVec(bacthAckData->connId, packetIdVec)) {
        int batchAckPacketNum = (packetIdVec.size() % BATCH_ACK_MAX_NUM == 0) ? packetIdVec.size() / BATCH_ACK_MAX_NUM : packetIdVec.size() / BATCH_ACK_MAX_NUM + 1;
        for (int i = 0; i < batchAckPacketNum; i++) {
            int ackedPacketNum = packetIdVec.size() < BATCH_ACK_MAX_NUM ? packetIdVec.size() : BATCH_ACK_MAX_NUM;

            int ackPacketLen = sizeof(XMDPacket) + sizeof(XMDBatchAck) + XMD_CRC_LEN + ackedPacketNum * sizeof(uint64_t);
            XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(ackPacketLen);
            xmdPakcet_t->SetMagic();
            xmdPakcet_t->SetSign(STREAM, BATCH_ACK, false);
            XMDBatchAck* batchAck = (XMDBatchAck*)((char*)xmdPakcet_t + sizeof(XMDPacket));
            batchAck->SetConnId(bacthAckData->connId);
            batchAck->SetPacketId(workerCommonData_->getPakcetId(bacthAckData->connId));
            
            char* ack = (char*)((char*)xmdPakcet_t + sizeof(XMDPacket) + sizeof(XMDBatchAck));
            std::vector<uint64_t>::iterator iter = packetIdVec.begin();
            for (int j = 0; j < ackedPacketNum; j++) {
                if (iter == packetIdVec.end()) {
                    break;
                }
                uint64_t acked_id = xmd_htonll(*iter);
                char* tmp_acked_id = (char*)&acked_id;
                for (unsigned int k = 0; k < sizeof(uint64_t); k++) {
                    ack[k] = tmp_acked_id[k];
                }

                iter = packetIdVec.erase(iter);
                ack+= sizeof(uint64_t);
            }

            char* crc32 = (char*)xmdPakcet_t + ackPacketLen - XMD_CRC_LEN;
            uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, ackPacketLen - XMD_CRC_LEN);
            uint32_t real_crc32 = htonl(crc32_val);
            char* tmp_char_crc = (char*)&real_crc32;
            for (int i = 0; i < XMD_CRC_LEN; i++) {
                crc32[i] = tmp_char_crc[i];
            }

            SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmdPakcet_t, ackPacketLen);
            commonData_->socketSendQueuePush(sendData);
        }
    }

    delete bacthAckData;
}


void WorkerThread::stop() {
    stopFlag_ = true;
}




