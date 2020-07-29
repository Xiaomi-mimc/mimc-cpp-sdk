#ifdef _WIN32
#include <ws2tcpip.h>
#else

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif // _WIN32
#include <sstream>
#include <string.h>

#include "XMDTransceiver.h"
#include "common.h"
#include "sleep_define.h"


int XMDTransceiver::start() {
    commonData_ = new XMDCommonData(workerThreadSize_, timerThreadSize_);
    packetDispatcher_ = new PacketDispatcher();
    recvThread_ = new XMDRecvThread(port_, commonData_, packetDispatcher_, this);
    sendThread_ = new XMDSendThread(commonData_, packetDispatcher_, this);
    workerThreadPool_ = new XMDWorkerThreadPool(workerThreadSize_, commonData_, packetDispatcher_);
    timerThreadPool_ = new XMDTimerThreadPool(timerThreadSize_, commonData_);

    bufferMaxSize_ = DEFAULT_RESEND_QUEUE_SIZE + SOCKET_SENDQUEUE_MAX_SIZE + WORKER_QUEUE_MAX_SIZE;
        
    int ret = recvThread_->InitSocket();
    if (ret != 0) {
        return ret;
    }
    sendThread_->SetListenFd(recvThread_->listenfd());

    return 0;
}

pthread_mutex_t XMDTransceiver::reset_socket_mutex_ = PTHREAD_MUTEX_INITIALIZER;

int XMDTransceiver::resetSocket() {
    /*pthread_mutex_lock(&reset_socket_mutex_);
    XMDLoggerWrapper::instance()->error("XMD reset socket");
    int ret = recvThread_->InitSocket();
    if (ret != 0) {
        packetDispatcher_->handleSocketError(ret, "reset socket err");
        XMDLoggerWrapper::instance()->error("XMD reset socket failed");
        pthread_mutex_unlock(&reset_socket_mutex_);
        return ret;
    }
    sendThread_->SetListenFd(recvThread_->listenfd());
    pthread_mutex_unlock(&reset_socket_mutex_);*/
    return 0;
}

int XMDTransceiver::buildAndSendDatagram(char* ip, uint16_t port, char* data, unsigned int len, uint64_t delay_ms, unsigned char packetType) {
    if (len > MAX_PACKET_LEN) {
        XMDLoggerWrapper::instance()->warn("packet too large,len=%d.", len);
        return -1;
    }
    
    if (NULL == data || NULL == ip) {
        XMDLoggerWrapper::instance()->warn("input invalid, ip or data is null.");
        return -1;
    }

    if (commonData_->isTimerQueueFull(0, len)) {
        XMDLoggerWrapper::instance()->warn("timer queue full drop datagram packet");
        return -1;
    }
    XMDPacketManager packetMan;
    packetMan.buildDatagram((unsigned char*)data, len, packetType);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return 0;
    }

    SendQueueData* queueData = new SendQueueData((in_addr_t)inet_addr(ip), port, (unsigned char*)xmddata, packetLen);

    WorkerThreadData* workerThreadData = new WorkerThreadData(WORKER_DATAGREAM_TIMEOUT, (void*)queueData, sizeof(SendQueueData) + packetLen);

    TimerThreadData* timerThreadData = new TimerThreadData();
    timerThreadData->connId = 0;
    if (0 == delay_ms) {
        timerThreadData->time = 0;
    } else {
        timerThreadData->time = delay_ms + current_ms();
    }
    timerThreadData->data = (void*)workerThreadData;
    timerThreadData->len = sizeof(WorkerThreadData) + sizeof(SendQueueData) + packetLen;
    
    commonData_->timerQueuePush(timerThreadData, 0);


    return 0;
}


int XMDTransceiver::sendDatagram(char* ip, uint16_t port, char* data, unsigned int len, uint64_t delay_ms) {
    unsigned char tmp = 0;
    return buildAndSendDatagram(ip, port, data, len, delay_ms, tmp);
}

int XMDTransceiver::sendTestRttPacket(uint64_t connId, unsigned int delayMs, unsigned int packetCount) {
    if (commonData_->isTimerQueueFull(0, sizeof(TestRttPacket) + sizeof(TimerThreadData) + sizeof(WorkerThreadData))) {
        XMDLoggerWrapper::instance()->warn("timer queue full drop test rtt packet");
        return -1;
    }
    TestRttPacket* testRtt = new TestRttPacket();
    testRtt->connId = connId;
    testRtt->delayMs = delayMs;
    testRtt->packetCount = packetCount;

    WorkerThreadData* workerThreadData = new WorkerThreadData(WORKER_TEST_RTT, (void*)testRtt, sizeof(TestRttPacket));

    TimerThreadData* timerThreadData = new TimerThreadData();
    timerThreadData->connId = 0;
    timerThreadData->time = delayMs + current_ms();
    timerThreadData->data = (void*)workerThreadData;
    timerThreadData->len = sizeof(WorkerThreadData) + sizeof(TestRttPacket);
    commonData_->timerQueuePush(timerThreadData, connId);

    return 0;
}



uint64_t XMDTransceiver::createConnection(char* ip, uint16_t port, char* data, unsigned int len, uint16_t timeout, void* ctx) {
    if (NULL == ip || (NULL == data && len != 0)) {
        XMDLoggerWrapper::instance()->warn("input invalid, ip is null");
        return 0;
    }
    
    uint32_t local_ip = 0;
    uint16_t local_port = 0;
    getLocalInfo(local_ip, local_port);
    uint64_t conn_id = rand64(local_ip, local_port);

    NewConnWorkerData* connData = new NewConnWorkerData((unsigned char*)data, len);
    connData->connInfo.connId = conn_id;
    connData->connInfo.ip = (in_addr_t)inet_addr(ip);
    connData->connInfo.port = port;
    connData->connInfo.timeout = timeout;
    connData->connInfo.connState = CONNECTING;
    connData->connInfo.max_stream_id = 0;
    connData->connInfo.ctx = ctx;

    uint16_t pingInterval = (unsigned int)(timeout / 2.5) * 1000;
    connData->connInfo.pingInterval = pingInterval < PING_INTERVAL ? PING_INTERVAL : pingInterval;
    commonData_->SetPingIntervalMs(conn_id, pingInterval);

    WorkerThreadData* workerData = new WorkerThreadData(WORKER_CREATE_NEW_CONN, (void*)connData, sizeof(NewConnWorkerData) + len);
    commonData_->workerQueuePush(workerData, conn_id);

    commonData_->addConnStreamId(conn_id, 0);

    return conn_id;
}

int XMDTransceiver::closeConnection(uint64_t connId) {
    WorkerData* workerData = new WorkerData();
    workerData->connId = connId;
    workerData->streamId = 0;

    WorkerThreadData* workerThreadData = new WorkerThreadData(WORKER_CLOSE_CONN, (void*)workerData, sizeof(WorkerData));
    commonData_->workerQueuePush(workerThreadData, connId);

    return 0;
}

uint16_t XMDTransceiver::createStream(uint64_t connId, StreamType streamType, uint16_t waitTime, bool isEncrypt) {  
    uint16_t streamId = commonData_->getConnStreamId(connId);
    
    NewStreamWorkerData* workerData = new NewStreamWorkerData();
    workerData->connId = connId;
    workerData->streamId = streamId;
    workerData->callbackWaitTimeout = waitTime;
    workerData->sType = streamType;
    workerData->isEncrypt = isEncrypt;

    WorkerThreadData* workerThreadData = new WorkerThreadData(WORKER_CREATE_STREAM, (void*)workerData, sizeof(NewStreamWorkerData));
    commonData_->workerQueuePush(workerThreadData, connId);
    
    return streamId;
}

int XMDTransceiver::closeStream(uint64_t connId, uint16_t streamId) {
    WorkerData* workerData = new WorkerData();
    workerData->connId = connId;
    workerData->streamId = streamId;

    WorkerThreadData* workerThreadData = new WorkerThreadData(WORKER_CLOSE_STREAM, (void*)workerData, sizeof(WorkerData));
    commonData_->workerQueuePush(workerThreadData, connId);
    
    return 0;
}

int XMDTransceiver::sendRTData(uint64_t connId, uint16_t streamId, char* data, unsigned int len, void* ctx) {
    return sendRTData(connId, streamId, data, len, false, P1, 2, ctx);
}


int XMDTransceiver::sendRTData(uint64_t connId, uint16_t streamId, char* data, unsigned int len, bool canBeDropped, DataPriority priority, int resendCount, void* ctx) {
    if (len > MAX_PACKET_LEN) {
        XMDLoggerWrapper::instance()->warn("packet too large,len=%d.", len);
        return -1;
    }

    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("input invalid, data is null.");
        return -1;
    }

    if (resendCount != -1 && commonData_->isWorkerQueueFull(connId % workerThreadSize_, len)) {
        XMDLoggerWrapper::instance()->warn("worker queue size %d + %d > %d.", 
                                           commonData_->getWorkerQueueUsedSize(connId % workerThreadSize_),  len,
                                           commonData_->getWorkerQueueMaxSize(connId % workerThreadSize_));
        return -1;
    }

    uint32_t groupId = commonData_->getGroupId(connId, streamId);
    RTWorkerData* workerData = new RTWorkerData((unsigned char*)data, len);
    workerData->connId = connId;
    workerData->streamId = streamId;
    workerData->groupId = groupId;
    workerData->canBeDropped = canBeDropped;
    workerData->dataPriority = priority;
    workerData->resendCount = resendCount;
    workerData->ctx = ctx;

    WorkerThreadData* workerThreadData = new WorkerThreadData(WORKER_SEND_RTDATA, workerData, sizeof(RTWorkerData) + len);
    commonData_->workerQueuePush(workerThreadData, connId);

    return groupId;
}


int XMDTransceiver::updatePeerInfo(uint64_t connId, char* ip, uint16_t port) {
    /*if (NULL == ip) {
        XMDLoggerWrapper::instance()->warn("input invalid, ip is null");
        return -1;
    }

    return commonData_->updateConnIpInfo(connId, (in_addr_t)inet_addr(ip), port);*/
    return 0;
}

int XMDTransceiver::getPeerInfo(uint64_t connId, std::string &ip, int32_t& port) {
    /*ConnInfo connInfo;
    if(!commonData_->getConnInfo(connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("connection(%ld) not exist.", connId);
        return -1;
    }

    port = (uint16_t)connInfo.port;

    const int IP_STR_LEN = 32;
    char ipStr[IP_STR_LEN];
    memset(ipStr, 0, IP_STR_LEN);
    inet_ntop(AF_INET, &connInfo.ip, ipStr, IP_STR_LEN);
    int ipLen = 0;
    for (ipLen = 0; ipLen < IP_STR_LEN; ipLen++) {
        if (ipStr[ipLen] == '\0') {
            break;
        }
    }
    char cIP[ipLen];
    memcpy(cIP, ipStr, ipLen);
    std::string tmpIp(cIP, ipLen);
    ip = tmpIp;*/

    return 0;
}



int XMDTransceiver::getLocalInfo(std::string &ip, uint16_t& port) {
    if (local_ip_ == 0) {
        local_ip_ = getLocalIntIp();
        struct in_addr tmp_addr;
        tmp_addr.s_addr = local_ip_;
        local_ip_str_ = inet_ntoa(tmp_addr);
    }

    if (local_port_ == 0) {
        local_port_ = getLocalPort(recvThread_->listenfd());
    }

    ip = local_ip_str_;
    port = local_port_;

    return 0;
}

int XMDTransceiver::getLocalInfo(uint32_t &ip, uint16_t& port) {
    if (local_ip_ == 0) {
        local_ip_ = getLocalIntIp();
        struct in_addr tmp_addr;
        tmp_addr.s_addr = local_ip_;
        local_ip_str_ = inet_ntoa(tmp_addr);
    }

    if (local_port_ == 0) {
        local_port_ = getLocalPort(recvThread_->listenfd());
    }
    
    ip = local_ip_;
    port = local_port_;

    return 0;
}



void XMDTransceiver::setSendBufferSize(int size) {
    bufferMaxSize_ = size;
    int timerQueueSize = size * 0.5;
    int workerQueueSize = size * 0.35;
    int socketQueueSize = bufferMaxSize_ - timerQueueSize - workerQueueSize;
    
    commonData_->setTimerQueueMaxSize(0, timerQueueSize);
    commonData_->setWorkerQueueMaxSize(0, workerQueueSize);
    commonData_->setSocketSendQueueMaxSize(socketQueueSize);
}

int XMDTransceiver::getSendBufferUsedSize() {
    return commonData_->getTimerQueueUsedSize(0) + commonData_->getWorkerQueueUsedSize(0) + getSocketSendQueueUsedSize();
}

int XMDTransceiver::getSendBufferMaxSize() {
    return bufferMaxSize_;
}

float XMDTransceiver::getSendBufferUsageRate() {
    float maxUsageRate = commonData_->getWorkerQueueUsageRate(0);
    float timerQueueUsageRate = commonData_->getTimerQueueUsageRate(0);
    float socketQueueUsageRate = commonData_->getSocketSendQueueUsageRate();
    if (maxUsageRate < timerQueueUsageRate) {
        maxUsageRate = timerQueueUsageRate;
    }
    if (maxUsageRate < socketQueueUsageRate) {
        maxUsageRate = socketQueueUsageRate;
    }

    return maxUsageRate;
}


void XMDTransceiver::clearSendBuffer() {
    //commonData_->clearTimerQueue(0);
}

ConnectionState XMDTransceiver::getConnState(uint64_t connId) {
    /*ConnInfo connInfo;
    if(!commonData_->getConnInfo(connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("connection(%ld) not exist.", connId);
        return CLOSED;
    }

    return connInfo.connState;*/
    return CONNECTED;
}


void XMDTransceiver::run() {
    sendThread_->run();
    recvThread_->run();
    workerThreadPool_->run();
    timerThreadPool_->run();
}

void XMDTransceiver::join() {
    sendThread_->join();
    recvThread_->join();
    workerThreadPool_->join();
    timerThreadPool_->join();
}

void XMDTransceiver::stop() {    
    //CLOSE CONN
    sendThread_->stop();
    recvThread_->stop();
    timerThreadPool_->stop();
    workerThreadPool_->stop();
    
    commonData_->NotifyBlockQueue();
    usleep(XMD_TRANS_STOP_SLEEP_MS);
}

