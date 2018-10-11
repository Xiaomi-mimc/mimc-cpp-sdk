#include "XMDTransceiver.h"
#include "common.h"
#include <string.h>
#include <arpa/inet.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <sstream>

int XMDTransceiver::sendDatagram(char* ip, int port, char* data, int len, uint64_t delay_ms) {
    if (len > MAX_PACKET_LEN) {
        LoggerWrapper::instance()->warn("packet too large,len=%d.", len);
        return -1;
    }
    
    if (NULL == data || NULL == ip) {
        LoggerWrapper::instance()->warn("input invalid, ip or data is null.");
        return -1;
    }
    XMDPacketManager packetMan;
    packetMan.buildDatagram((unsigned char*)data, len);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return 0;
    }

    SendQueueData* queueData = new SendQueueData((in_addr_t)inet_addr(ip), port, (unsigned char*)xmddata, packetLen);

    if (0 == delay_ms) {
        queueData->sendTime = 0;
    } else {
        queueData->sendTime = delay_ms + current_ms();
    }

    commonData_->datagramQueuePush(queueData);

    return 0;
}

uint64_t XMDTransceiver::createConnection(char* ip, int port, char* data, int len, int timeout, void* ctx, bool isEncrypt) {
    if (NULL == ip || (NULL == data && len != 0)) {
        LoggerWrapper::instance()->warn("input invalid, ip is null");
        return 0;
    }
    uint64_t conn_id = rand64();

    RSA* rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL);
    
    uint16_t n_len = BN_num_bytes(rsa->n);
    uint16_t e_len = BN_num_bytes(rsa->e);
    unsigned char nbignum[n_len];
    BN_bn2bin(rsa->n, nbignum);
    std::string tmpnstr((char*)nbignum, n_len);
    unsigned char ebignum[e_len];
    BN_bn2bin(rsa->e, ebignum);

    
    ConnInfo connInfo;
    connInfo.ip = (in_addr_t)inet_addr(ip);
    connInfo.port = port;
    connInfo.timeout = timeout;
    connInfo.isConnected = false;
    connInfo.created_stream_id = 0;  
    connInfo.max_stream_id = 0;
    connInfo.rsa = rsa;
    connInfo.isEncrypt = isEncrypt;
    connInfo.ctx = ctx;
    commonData_->insertConn(conn_id, connInfo);

    
    XMDPacketManager packetMan;
    packetMan.buildConn(conn_id, (unsigned char*)data, len, timeout, n_len, nbignum, e_len, ebignum, isEncrypt);
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
    resendData->lastSendTime = current_ms();
    resendData->reSendTime = current_ms() + RESEND_DATA_INTEVAL;
    resendData->sendCount = 1;
    commonData_->resendQueuePush(resendData);

    std::stringstream ss_ack;
    ss_ack << conn_id << 0;
    std::string ackpacketKey = ss_ack.str();
    commonData_->updateResendMap(ackpacketKey, false);


    return conn_id;
}

int XMDTransceiver::closeConnection(uint64_t conn_id) {
    ConnInfo connInfo;
    if(!commonData_->getConnInfo(conn_id, connInfo)){
        LoggerWrapper::instance()->warn("connection(%ld) not exist.", conn_id);
        return -1;
    }
    
    if (commonData_->deleteConn(conn_id) != 0) {
        return -1;
    }

    XMDPacketManager packetMan;
    packetMan.buildConnClose(conn_id);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return 0;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);

    return 0;
}

uint16_t XMDTransceiver::createStream(uint64_t conn_id, StreamType streamType, int timeout) {  
    ConnInfo connInfo;
    if(!commonData_->getConnInfo(conn_id, connInfo)){
        return 0;
    }
    
    uint16_t streamId = commonData_->getConnStreamId(conn_id);
    StreamInfo streamInfo;
    streamInfo.timeout = timeout;
    streamInfo.sType = streamType;
    if (commonData_->insertStream(conn_id, streamId, streamInfo) != 0) {
        return 0;
    }
    
    return streamId;
}

int XMDTransceiver::closeStream(uint64_t conn_id, uint16_t stream_id) {
    if (commonData_->deleteStream(conn_id, stream_id) != 0) {
        return -1;
    }

    ConnInfo connInfo;
    if(!commonData_->getConnInfo(conn_id, connInfo)){
        LoggerWrapper::instance()->warn("connection(%ld) not exist.", conn_id);
        return -1;
    }
    if (!connInfo.isConnected) {
        LoggerWrapper::instance()->warn("connection(%ld) has not been established.", conn_id);
        return -1;
    }

    XMDPacketManager packetMan;
    packetMan.buildStreamClose(conn_id, stream_id, connInfo.isEncrypt, connInfo.sessionKey);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return -1;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);

    return 0;
}

int XMDTransceiver::sendRTData(uint64_t conn_id, uint16_t stream_id, char* data, int len, void* ctx) {
    if (len > MAX_PACKET_LEN) {
        LoggerWrapper::instance()->warn("packet too large,len=%d.", len);
        return -1;
    }

    if (NULL == data) {
        LoggerWrapper::instance()->warn("input invalid, data is null.");
        return -1;
    }

    ConnInfo connInfo;
    if(!commonData_->getConnInfo(conn_id, connInfo)){
        LoggerWrapper::instance()->warn("connection(%ld) not exist.", conn_id);
        return -1;
    }

    StreamInfo sInfo;
    if(!commonData_->getStreamInfo(conn_id, stream_id, sInfo)) {
        LoggerWrapper::instance()->warn("stream(%d) not exist.", stream_id);
        return -1;
    }

    
    StreamQueueData* queueData = new StreamQueueData(len);
    queueData->connId = conn_id;
    queueData->streamId = stream_id;
    queueData->groupId = commonData_->getGroupId(conn_id, stream_id);
    queueData->len = len;
    queueData->ctx = ctx;
    memcpy(queueData->data, data, len);
    commonData_->streamQueuePush(queueData);
    return queueData->groupId;
}


int XMDTransceiver::updatePeerInfo(uint64_t conn_id, char* ip, int port) {
    if (NULL == ip) {
        LoggerWrapper::instance()->warn("input invalid, ip is null");
        return -1;
    }

    return commonData_->updateConnIpInfo(conn_id, (in_addr_t)inet_addr(ip), port);;
}

int XMDTransceiver::getPeerInfo(uint64_t conn_id, std::string &ip, int& port) {
    ConnInfo connInfo;
    if(!commonData_->getConnInfo(conn_id, connInfo)){
        LoggerWrapper::instance()->warn("connection(%ld) not exist.", conn_id);
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

int XMDTransceiver::getLocalInfo(std::string &ip, int& port) {
    struct sockaddr_in  loc_addr;  
    socklen_t len = sizeof(loc_addr);  
    memset(&loc_addr, 0, len); 
    if (-1 == getsockname(recvThread_->listenfd(), (struct sockaddr *)&loc_addr, &len)) {
        LoggerWrapper::instance()->error("get ip failed.");
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



void XMDTransceiver::run() {
    sendThread_->run();
    recvThread_->run();
    packetbuildThreadPool_->run();
    packetDecodeThreadPool_->run();
    packetRecoverThreadPool_->run();
    callbackThread_->run();
    pingThread_->run();
}

void XMDTransceiver::join() {
    packetbuildThreadPool_->join();
    sendThread_->join();
    recvThread_->join();
    packetDecodeThreadPool_->join();
    packetRecoverThreadPool_->join();
    callbackThread_->join();
    pingThread_->join();
}

void XMDTransceiver::stop() {
    std::vector<uint64_t> connVec = commonData_->getConnVec();
    for (size_t i = 0; i < connVec.size(); i++) {
        closeConnection(connVec[i]);
    }
    usleep(1000);
    
    packetbuildThreadPool_->stop();

    sendThread_->stop();
    recvThread_->stop();
    packetDecodeThreadPool_->stop();
    packetRecoverThreadPool_->stop();
    callbackThread_->stop();
    pingThread_->stop();
    usleep(10);
}
