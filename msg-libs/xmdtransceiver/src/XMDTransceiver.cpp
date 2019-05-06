#ifdef _WIN32
#include <ws2tcpip.h>

#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif // _WIN32

#include "XMDTransceiver.h"
#include "common.h"
#include <string.h>
#include <thread>
#include <chrono>
#include <sstream>

int XMDTransceiver::start() {
    commonData_ = new XMDCommonData(decodeThreadSize_);
    packetDispatcher_ = new PacketDispatcher();
    recvThread_ = new XMDRecvThread(port_, commonData_, packetDispatcher_, this);
    sendThread_ = new XMDSendThread(commonData_, packetDispatcher_, this);
    packetbuildThreadPool_ = new XMDPacketBuildThreadPool(1, commonData_, packetDispatcher_);
    packetRecoverThreadPool_ = new XMDPacketRecoverThreadPool(decodeThreadSize_, commonData_);
    packetDecodeThreadPool_ = new XMDPacketDecodeThreadPool(1, commonData_, packetDispatcher_);
    callbackThread_ = new XMDCallbackThread(packetDispatcher_, commonData_);
    pingThread_ = new PingThread(packetDispatcher_, commonData_);
    pongThread_ = new PongThread(commonData_);
        
    int ret = recvThread_->InitSocket();
    if (ret != 0) {
        return ret;
    }
    sendThread_->SetListenFd(recvThread_->listenfd());
	
#ifdef _WIN32
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			XMDLoggerWrapper::instance()->error("WSAStartup(MAKEWORD(2, 2), &wsaData) execute failed!");
			return -1;
		}
#endif // _WIN32

    return 0;
}

int XMDTransceiver::resetSocket() {
    /*reset_socket_mutex_.lock();
    XMDLoggerWrapper::instance()->error("XMD reset socket");
    int ret = recvThread_->InitSocket();
    if (ret != 0) {
        packetDispatcher_->handleSocketError(ret, "reset socket err");
        XMDLoggerWrapper::instance()->error("XMD reset socket failed");
        reset_socket_mutex_.unlock();
        return ret;
    }
    sendThread_->SetListenFd(recvThread_->listenfd());
    reset_socket_mutex_.unlock();*/
    return 0;
}

int XMDTransceiver::sendDatagram(char* ip, uint16_t port, char* data, int len, uint64_t delay_ms) {
    if (len > MAX_PACKET_LEN) {
        XMDLoggerWrapper::instance()->warn("packet too large,len=%d.", len);
        return -1;
    }
    
    if (NULL == data || NULL == ip) {
        XMDLoggerWrapper::instance()->warn("input invalid, ip or data is null.");
        return -1;
    }
    XMDPacketManager packetMan;
    packetMan.buildDatagram((unsigned char*)data, len);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return 0;
    }

    SendQueueData* queueData = new SendQueueData((uint32_t)inet_addr(ip), port, (unsigned char*)xmddata, packetLen);

    if (0 == delay_ms) {
        queueData->sendTime = 0;
    } else {
        queueData->sendTime = delay_ms + current_ms();
    }

    commonData_->datagramQueuePush(queueData);

    return 0;
}

pthread_mutex_t XMDTransceiver::create_conn_mutex_ = PTHREAD_MUTEX_INITIALIZER;

uint64_t XMDTransceiver::createConnection(char* ip, uint16_t port, char* data, int len, uint16_t timeout, void* ctx) {
    if (NULL == ip || (NULL == data && len != 0)) {
        XMDLoggerWrapper::instance()->warn("input invalid, ip is null");
        return 0;
    }
    //uint32_t ip_int = (uint32_t)inet_addr(ip);
    //uint64_t conn_id = rand64(ip_int, port);
	uint64_t conn_id = rand64();
    
    //uint32_t local_ip = 0;
    //uint16_t local_port = 0;
    //getLocalInfo(local_ip, local_port);
    //uint64_t conn_id = rand64(local_ip, local_port);

    
    ConnInfo connInfo;
    connInfo.ip = (in_addr_t)inet_addr(ip);
    connInfo.port = port;
    connInfo.timeout = timeout;
    connInfo.connState = CONNECTING;
    connInfo.created_stream_id = 0;  
    connInfo.max_stream_id = 0;
    connInfo.ctx = ctx;
    commonData_->insertConn(conn_id, connInfo);

    XMDPacketManager packetMan;
    packetMan.buildConn(conn_id, (unsigned char*)data, len, timeout, 0, NULL, 0, NULL, false);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return 0;
    }

    SendQueueData* sendData = new SendQueueData((in_addr_t)inet_addr(ip), port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);


    ResendData* resendData = new ResendData((unsigned char*)xmddata, packetLen);
    resendData->connId = conn_id;
    resendData->packetId = 0;
    resendData->ip = connInfo.ip;
    resendData->port = connInfo.port;
    resendData->reSendTime = current_ms() + commonData_->getResendTimeInterval();
    resendData->reSendCount = CONN_RESEND_TIME;
    commonData_->resendQueuePush(resendData);

    std::stringstream ss_ack;
    ss_ack << conn_id << 0;
    std::string ackpacketKey = ss_ack.str();
    commonData_->insertIsPacketRecvAckMap(ackpacketKey, false);

    PacketCallbackInfo packetInfo;
    packetInfo.connId = conn_id;
    packetInfo.streamId = 0;
    packetInfo.packetId = 0;
    packetInfo.groupId = 0;
    packetInfo.sliceId = 0;
    packetInfo.ctx = ctx;
    commonData_->insertPacketCallbackInfoMap(ackpacketKey, packetInfo);


    return conn_id;
}

int XMDTransceiver::closeConnection(uint64_t connId) {
    ConnInfo connInfo;
    if(!commonData_->getConnInfo(connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("connection(%ld) not exist.", connId);
        return -1;
    }
    
    if (commonData_->deleteConn(connId) != 0) {
        return -1;
    }

    XMDPacketManager packetMan;
    packetMan.buildConnClose(connId);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return 0;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);

    return 0;
}

uint16_t XMDTransceiver::createStream(uint64_t connId, StreamType streamType, uint16_t waitTime, bool isEncrypt) {  
    ConnInfo connInfo;
    if(!commonData_->getConnInfo(connId, connInfo)){
        return 0;
    }
    
    uint16_t streamId = commonData_->getConnStreamId(connId);
    StreamInfo streamInfo;
    streamInfo.sType = streamType;
    streamInfo.isEncrypt = isEncrypt;
    streamInfo.callbackWaitTimeout = waitTime;
    if (commonData_->insertStream(connId, streamId, streamInfo) != 0) {
        return 0;
    }
    
    return streamId;
}

int XMDTransceiver::closeStream(uint64_t connId, uint16_t streamId) {
    if (commonData_->deleteStream(connId, streamId) != 0) {
        return -1;
    }

    ConnInfo connInfo;
    if(!commonData_->getConnInfo(connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("connection(%ld) not exist.", connId);
        return -1;
    }
    if (connInfo.connState != CONNECTED) {
        XMDLoggerWrapper::instance()->warn("connection(%ld) has not been established.", connId);
        return -1;
    }

    XMDPacketManager packetMan;
    packetMan.buildStreamClose(connId, streamId, false, connInfo.sessionKey);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return -1;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);

    return 0;
}

int XMDTransceiver::sendRTData(uint64_t connId, uint16_t streamId, char* data, int len, void* ctx) {
    return sendRTData(connId, streamId, data, len, false, P1, 2, ctx);
}


int XMDTransceiver::sendRTData(uint64_t connId, uint16_t streamId, char* data, int len, bool canBeDropped, DataPriority priority, int resendCount, void* ctx) {
    if (len > MAX_PACKET_LEN) {
        XMDLoggerWrapper::instance()->warn("packet too large,len=%d.", len);
        return -1;
    }

    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("input invalid, data is null.");
        return -1;
    }

    /*ConnInfo connInfo;
    if(!commonData_->getConnInfo(connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("connection(%ld) not exist.", connId);
        return -1;
    }

    StreamInfo sInfo;
    if(!commonData_->getStreamInfo(connId, streamId, sInfo)) {
        XMDLoggerWrapper::instance()->warn("stream(%d) not exist.", streamId);
        return -1;
    }*/

    uint32_t groupId = commonData_->getGroupId(connId, streamId);
    StreamQueueData* queueData = new StreamQueueData(len);
    queueData->connId = connId;
    queueData->streamId = streamId;
    queueData->groupId = groupId;
    queueData->len = len;
    queueData->canBeDropped = canBeDropped;
    queueData->dataPriority = priority;
    queueData->ctx = ctx;
    if (resendCount >= 0) {
        queueData->resendCount = resendCount + 1;
    }
    memcpy(queueData->data, data, len);
    commonData_->streamQueuePush(queueData);
    return groupId;
}


int XMDTransceiver::updatePeerInfo(uint64_t connId, char* ip, uint16_t port) {
    if (NULL == ip) {
        XMDLoggerWrapper::instance()->warn("input invalid, ip is null");
        return -1;
    }

    return commonData_->updateConnIpInfo(connId, (in_addr_t)inet_addr(ip), port);
}

int XMDTransceiver::getPeerInfo(uint64_t connId, std::string &ip, int32_t& port) {
    ConnInfo connInfo;
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
    ip = tmpIp;

    return 0;
}

int XMDTransceiver::getLocalInfo(std::string &ip, uint16_t& port) {
    struct sockaddr_in  loc_addr;  
    socklen_t len = sizeof(loc_addr);  
    memset(&loc_addr, 0, len); 
    if (-1 == getsockname(recvThread_->listenfd(), (struct sockaddr *)&loc_addr, &len)) {
        XMDLoggerWrapper::instance()->error("get ip failed.");
        return -1;
    }

    port = ntohs(loc_addr.sin_port);

    uint32_t ipv4;
    int err = get_eth0_ipv4(&ipv4);
    if (err) {
      ipv4 = 0;
    }
    const int IP_STR_LEN = 16;
    char ip_chars[IP_STR_LEN];
    inet_ntop(AF_INET, &ipv4, ip_chars, IP_STR_LEN);
    int ipLen = 0;
    for (ipLen = 0; ipLen < IP_STR_LEN; ipLen++) {
        if (ip_chars[ipLen] == '\0') {
            break;
        }
    }
    char cIP[ipLen];
    memcpy(cIP, ip_chars, ipLen);
    std::string tmpIp(cIP, ipLen);

    ip = tmpIp;

    return 0;
}

int XMDTransceiver::getLocalInfo(uint32_t &ip, uint16_t& port) {
    struct sockaddr_in  loc_addr;  
    socklen_t len = sizeof(loc_addr);  
    memset(&loc_addr, 0, len); 
    if (-1 == getsockname(recvThread_->listenfd(), (struct sockaddr *)&loc_addr, &len)) {
        XMDLoggerWrapper::instance()->error("get ip failed.");
        return -1;
    }

    port = ntohs(loc_addr.sin_port);

    uint32_t ipv4;
    int err = get_eth0_ipv4(&ipv4);
    if (err) {
      ipv4 = 0;
    }

    ip = ipv4;

    return 0;
}


void XMDTransceiver::setSendBufferSize(int size) {
    commonData_->setResendQueueSize(size);
}

void XMDTransceiver::setRecvBufferSize(int size) {
    commonData_->setCallbackQueueSize(size);
}

int XMDTransceiver::getSendBufferSize() {
    return commonData_->getResendQueueSize();
}

int XMDTransceiver::getRecvBufferSize() {
    return commonData_->getCallbackQueueSize();
}


float XMDTransceiver::getSendBufferUsageRate() {
    return commonData_->getResendQueueUsageRate();
}


float XMDTransceiver::getRecvBufferUsageRate() {
    return commonData_->getCallbackQueueUsegeRate();
}

void XMDTransceiver::clearSendBuffer() {
    commonData_->clearResendQueue();
}

void XMDTransceiver::clearRecvBuffer() {
    commonData_->clearCallbackQueue();
}

ConnectionState XMDTransceiver::getConnState(uint64_t connId) {
    ConnInfo connInfo;
    if(!commonData_->getConnInfo(connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("connection(%ld) not exist.", connId);
        return CLOSED;
    }

    return connInfo.connState;
}


void XMDTransceiver::run() {
    sendThread_->run();
    recvThread_->run();
    packetbuildThreadPool_->run();
    packetDecodeThreadPool_->run();
    packetRecoverThreadPool_->run();
    callbackThread_->run();
    pingThread_->run();
    pongThread_->run();
}

void XMDTransceiver::join() {
    packetbuildThreadPool_->join();
    sendThread_->join();
    recvThread_->join();
    packetDecodeThreadPool_->join();
    packetRecoverThreadPool_->join();
    callbackThread_->join();
    pingThread_->join();
    pongThread_->join();
}

void XMDTransceiver::stop() {
    std::vector<uint64_t> connVec = commonData_->getConnVec();
    for (size_t i = 0; i < connVec.size(); i++) {
        closeConnection(connVec[i]);
    }
    //usleep(1000);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
    packetbuildThreadPool_->stop();

    sendThread_->stop();
    recvThread_->stop();
    packetDecodeThreadPool_->stop();
    packetRecoverThreadPool_->stop();
    callbackThread_->stop();
    pingThread_->stop();
    pongThread_->stop();
    //usleep(1000);
	commonData_->NotifyBlockQueue();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
