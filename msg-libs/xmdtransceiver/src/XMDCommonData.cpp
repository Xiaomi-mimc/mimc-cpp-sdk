#include "XMDCommonData.h"
#include "LoggerWrapper.h"
#include "common.h"
#include <sstream>

pthread_mutex_t XMDCommonData::stream_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::socket_send_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::socket_recv_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::resend_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::datagram_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::callback_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::ping_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::packetId_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::resend_map_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::callback_data_map_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::stream_data_send_callback_map_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::ack_packet_map_mutex_ = PTHREAD_MUTEX_INITIALIZER;



XMDCommonData::XMDCommonData(int decodeThreadSize) {
    pthread_rwlock_init(&conn_mutex_, NULL);
    pthread_rwlock_init(&last_packet_time_mutex_, NULL);
    pthread_rwlock_init(&netstatus_mutex_, NULL);
    pthread_rwlock_init(&group_id_mutex_, NULL);
    pthread_rwlock_init(&last_recv_group_id_mutex_, NULL);
    pthread_rwlock_init(&last_callback_group_id_mutex_, NULL);

    decodeThreadSize_ = decodeThreadSize;
    for (int i = 0; i < decodeThreadSize; i++) {
        std::queue<StreamData*> socketRecvQueue_;
        packetRecoverQueueVec_.push_back(socketRecvQueue_);
        pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
        packetRecoverQueueMutexVec_.push_back(queueMutex);
    }

    connVec_.clear();
}

XMDCommonData::~XMDCommonData() {
    pthread_rwlock_destroy(&conn_mutex_);
    pthread_rwlock_destroy(&last_packet_time_mutex_);
    pthread_rwlock_destroy(&netstatus_mutex_);
    pthread_rwlock_destroy(&group_id_mutex_);
    pthread_rwlock_destroy(&last_recv_group_id_mutex_);
    pthread_rwlock_destroy(&last_callback_group_id_mutex_);
    connVec_.clear();
}


void XMDCommonData::streamQueuePush(StreamQueueData * data) {
    pthread_mutex_lock(&stream_queue_mutex_);
    streamQueue_.push(data);
    pthread_mutex_unlock(&stream_queue_mutex_);
}

StreamQueueData* XMDCommonData::streamQueuePop() {
    StreamQueueData* data = NULL;
    pthread_mutex_lock(&stream_queue_mutex_);
    if (streamQueue_.empty()) {
        pthread_mutex_unlock(&stream_queue_mutex_);
        return NULL;
    }
    data = streamQueue_.front();
    streamQueue_.pop();
    pthread_mutex_unlock(&stream_queue_mutex_);
    return data;
}

void XMDCommonData::socketSendQueuePush(SendQueueData* data) {
    pthread_mutex_lock(&socket_send_queue_mutex_);
    socketSendQueue_.push(data);
    pthread_mutex_unlock(&socket_send_queue_mutex_);
}

SendQueueData* XMDCommonData::socketSendQueuePop() {
    SendQueueData* data = NULL;
    pthread_mutex_lock(&socket_send_queue_mutex_);
    if (socketSendQueue_.empty()) {
        pthread_mutex_unlock(&socket_send_queue_mutex_);
        return NULL;
    }
    data = socketSendQueue_.front();
    socketSendQueue_.pop();
    pthread_mutex_unlock(&socket_send_queue_mutex_);
    
    return data;
}

void XMDCommonData::socketRecvQueuePush(SocketData* data) {
    pthread_mutex_lock(&socket_recv_queue_mutex_);
    socketRecvQueue_.push(data);
    pthread_mutex_unlock(&socket_recv_queue_mutex_);
}

SocketData* XMDCommonData::socketRecvQueuePop() {
    SocketData* data = NULL;
    pthread_mutex_lock(&socket_recv_queue_mutex_);
    if (socketRecvQueue_.empty()) {
        pthread_mutex_unlock(&socket_recv_queue_mutex_);
        return NULL;
    }
    data = socketRecvQueue_.front();
    socketRecvQueue_.pop();
    pthread_mutex_unlock(&socket_recv_queue_mutex_);
    return data;
}



void XMDCommonData::packetRecoverQueuePush(StreamData* data, int id) {
    pthread_mutex_lock(&packetRecoverQueueMutexVec_[id]);
    if (id >= (int)packetRecoverQueueVec_.size()) {
        LoggerWrapper::instance()->warn("packetRecoverQueuePush invalid thread id:%d", id);
        pthread_mutex_unlock(&packetRecoverQueueMutexVec_[id]);
        return;
    }
    packetRecoverQueueVec_[id].push(data);
    pthread_mutex_unlock(&packetRecoverQueueMutexVec_[id]);
}

StreamData* XMDCommonData::packetRecoverQueuePop(int id) {
    StreamData* data = NULL;
    pthread_mutex_lock(&packetRecoverQueueMutexVec_[id]);
    if (id >= (int)packetRecoverQueueVec_.size()) {
        LoggerWrapper::instance()->warn("packetRecoverQueuePop invalid thread id:%d, queue size=%d", id, packetRecoverQueueVec_.size());
        pthread_mutex_unlock(&packetRecoverQueueMutexVec_[id]);
        return NULL;
    }
    
    
    if (packetRecoverQueueVec_[id].empty()) {
        pthread_mutex_unlock(&packetRecoverQueueMutexVec_[id]);
        return NULL;
    }
    
    data = packetRecoverQueueVec_[id].front();
    packetRecoverQueueVec_[id].pop();
    pthread_mutex_unlock(&packetRecoverQueueMutexVec_[id]);

    return data;
}

void XMDCommonData::callbackQueuePush(CallbackQueueData* data) {
    pthread_mutex_lock(&callback_queue_mutex_);
    callbackQueue_.push(data);
    pthread_mutex_unlock(&callback_queue_mutex_);
}

CallbackQueueData* XMDCommonData::callbackQueuePop() {
    CallbackQueueData* data = NULL;
    if (callbackQueue_.empty()) {
        return NULL;
    }
    pthread_mutex_lock(&callback_queue_mutex_);
    data = callbackQueue_.front();
    callbackQueue_.pop();
    pthread_mutex_unlock(&callback_queue_mutex_);

    return data;
}

void XMDCommonData::datagramQueuePush(SendQueueData* data) {
    pthread_mutex_lock(&datagram_queue_mutex_);
    datagramQueue_.push(data);
    pthread_mutex_unlock(&datagram_queue_mutex_);
}

SendQueueData* XMDCommonData::datagramQueuePop() {
    SendQueueData* data = NULL;
    pthread_mutex_lock(&datagram_queue_mutex_);
    if (datagramQueue_.empty()) {
        pthread_mutex_unlock(&datagram_queue_mutex_);
        return NULL;
    }
    data = datagramQueue_.top();
    uint64_t currentMs = current_ms();
    if (data->sendTime <= currentMs) {
        datagramQueue_.pop();
    } else {
        data = NULL;
    }
    pthread_mutex_unlock(&datagram_queue_mutex_);
    return data;
}


bool XMDCommonData::isConnExist(uint64_t connId) {
    pthread_rwlock_rdlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        LoggerWrapper::instance()->debug("isConnExist connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return false;
    }
    pthread_rwlock_unlock(&conn_mutex_);
    return true;
}


int XMDCommonData::insertConn(uint64_t connId, ConnInfo connInfo) {
    pthread_rwlock_wrlock(&conn_mutex_);
    connectionMap_[connId] = connInfo;
    connVec_.push_back(connId);
    pthread_rwlock_unlock(&conn_mutex_);
    return 0;
}
int XMDCommonData::updateConn(uint64_t connId, ConnInfo connInfo) {
    pthread_rwlock_wrlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        LoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return -1;
    }
    
    connectionMap_[connId] = connInfo;
    pthread_rwlock_unlock(&conn_mutex_);
    return 0;
}

int XMDCommonData::deleteConn(uint64_t connId) {
    uint32_t maxStreamId = 0;
    pthread_rwlock_wrlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it != connectionMap_.end()) {
        maxStreamId = it->second.max_stream_id;
        RSA_free(it->second.rsa);
        connectionMap_.erase(it);
    }

    std::vector<uint64_t>::iterator it2 = connVec_.begin();
    for (; it2 != connVec_.end(); it2++) {
        if (*it2 == connId) {
            connVec_.erase(it2);
            break;
        }
    }
    
    pthread_rwlock_unlock(&conn_mutex_);

    pthread_mutex_lock(&ping_mutex_);
    std::map<uint64_t, PingPackets>::iterator pingIt = pingMap_.find(connId);
    if (pingIt != pingMap_.end()) {
        pingMap_.erase(pingIt);
        LoggerWrapper::instance()->debug("connection(%ld) stop ping", connId);
    }
    pthread_mutex_unlock(&ping_mutex_);

    pthread_mutex_lock(&packetId_mutex_);
    std::unordered_map<uint64_t, uint64_t>::iterator packetIdIt = packetIdMap_.find(connId);
    if (packetIdIt != packetIdMap_.end()) {
        packetIdMap_.erase(packetIdIt);
    } 
    pthread_mutex_unlock(&packetId_mutex_);

    deleteNetStatus(connId);

    std::stringstream ss_conn;
    ss_conn << connId;
    std::string connKey = ss_conn.str();
    deleteLastPacketTime(connKey);
    for (unsigned int i = 0; i <= maxStreamId; i++) {
        std::stringstream ss;
        ss << connId << i;
        std::string key = ss.str();
        deleteLastPacketTime(key);
        deleteLastRecvGroupId(key);
        deleteGroupId(key);
        deleteLastCallbackGroupId(key);
    }
    
    return 0;
}

bool XMDCommonData::getConnInfo(uint64_t connId, ConnInfo& cInfo) {
    pthread_rwlock_rdlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        LoggerWrapper::instance()->debug("getConnInfo connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return false;
    }
    cInfo = connectionMap_[connId];
    pthread_rwlock_unlock(&conn_mutex_);
    return true;
}

uint32_t XMDCommonData::getConnStreamId(uint64_t connId) {
    uint32_t streamId = 0;
    pthread_rwlock_wrlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it != connectionMap_.end()) {
        ConnInfo& info = it->second;
        streamId = info.created_stream_id + 2;
        info.created_stream_id = streamId;
    }
    
    pthread_rwlock_unlock(&conn_mutex_);
    return streamId;
}

int XMDCommonData::updateConnIpInfo(uint64_t connId, uint32_t ip, int port) {
    pthread_rwlock_wrlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it != connectionMap_.end()) {
        ConnInfo& info = it->second;
        info.ip = ip;
        info.port = port;
    } else {
        pthread_rwlock_unlock(&conn_mutex_);
        return -1;
    }
    
    pthread_rwlock_unlock(&conn_mutex_);
    return 0;
}



std::vector<uint64_t> XMDCommonData::getConnVec() {
    std::vector<uint64_t> tmpvec; 
    pthread_rwlock_rdlock(&conn_mutex_);
    tmpvec = connVec_;
    pthread_rwlock_unlock(&conn_mutex_);
    return tmpvec;
}


bool XMDCommonData::isStreamExist(uint64_t connId, uint16_t streamId) {
    pthread_rwlock_rdlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        LoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return false;
    }
    
    std::unordered_map<uint16_t, StreamInfo>::iterator it = itConn->second.streamMap.find(streamId);
    if (it == itConn->second.streamMap.end()) {
        LoggerWrapper::instance()->debug("isStreamExist stream(%d) not exist.", streamId);
        pthread_rwlock_unlock(&conn_mutex_);
        return false;
    }
    pthread_rwlock_unlock(&conn_mutex_);
    return true;
}


int XMDCommonData::insertStream(uint64_t connId, uint16_t streamId, StreamInfo streamInfo) {
    pthread_rwlock_wrlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        LoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return -1;
    }
    
    ConnInfo& connInfo = it->second;
    if (!connInfo.isConnected) {
        LoggerWrapper::instance()->warn("insertStream connection(%ld) has not been established.", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return -1;
    }
    if (connInfo.max_stream_id < streamId) {
        connInfo.max_stream_id = streamId;
    }
    connInfo.streamMap[streamId] = streamInfo;
    pthread_rwlock_unlock(&conn_mutex_);
    return 0;
}

int XMDCommonData::deleteStream(uint64_t connId, uint16_t streamId) {
    pthread_rwlock_wrlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        LoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return -1;
    }
    ConnInfo& connInfo = itConn->second;

    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamId);
    if (it == connInfo.streamMap.end()) {
        LoggerWrapper::instance()->warn("stream(%d) not exist.", streamId);
        pthread_rwlock_unlock(&conn_mutex_);
        return -1;
    }
    connInfo.streamMap.erase(it);
    pthread_rwlock_unlock(&conn_mutex_);
    return 0;
}

bool XMDCommonData::getStreamInfo(uint64_t connId, uint16_t streamId, StreamInfo& sInfo) {
    pthread_rwlock_rdlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        LoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return false;
    }
    ConnInfo& connInfo = itConn->second;
    

    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamId);
    if (it == connInfo.streamMap.end()) {
        LoggerWrapper::instance()->warn("stream(%d) not exist.", streamId);
        pthread_rwlock_unlock(&conn_mutex_);
        return false;
    }
    sInfo = it->second;
    pthread_rwlock_unlock(&conn_mutex_);

    return true;
}


int XMDCommonData::startPing(uint64_t connId) {
    pthread_mutex_lock(&ping_mutex_);
    PingPackets packets;
    packets.pingIndex = 0;
    pingMap_[connId] = packets;
    pthread_mutex_unlock(&ping_mutex_);

    LoggerWrapper::instance()->debug("connection(%ld) start ping", connId);

    return 0;
}

int XMDCommonData::insertPing(PingPacket packet) {
    pthread_mutex_lock(&ping_mutex_);
    std::map<uint64_t, PingPackets>::iterator it = pingMap_.find(packet.connId);
    if (it == pingMap_.end()) {
        LoggerWrapper::instance()->warn("conn(%ld) ping not exist.", packet.connId);
    } else {
        PingPackets& packets = it->second;
        if (packets.pingVec.size() < PING_PACKET_SIZE) {
            packets.pingVec.push_back(packet);
            packets.pingIndex = (packets.pingIndex + 1) % PING_PACKET_SIZE;
        } else {
            packets.pingVec[packets.pingIndex] = packet;
            packets.pingIndex = (packets.pingIndex + 1) % PING_PACKET_SIZE;
        }
    }
    pthread_mutex_unlock(&ping_mutex_);

    return 0;
}

int XMDCommonData::insertPong(uint64_t connId, uint64_t packetId, uint64_t currentTime, uint64_t ts) {
    pthread_mutex_lock(&ping_mutex_);
    std::map<uint64_t, PingPackets>::iterator it = pingMap_.find(connId);
    if (it == pingMap_.end()) {
        LoggerWrapper::instance()->warn("conn(%ld) ping packet not exist. packetid(%ld).", connId, packetId);
        pthread_mutex_unlock(&ping_mutex_);
        return -1;
    }
    PingPackets& packets = it->second;
    for (size_t i = 0; i < packets.pingVec.size(); i++) {
        if (packets.pingVec[i].connId == connId && packets.pingVec[i].packetId == packetId) {
            packets.pingVec[i].ttl = currentTime - packets.pingVec[i].sendTime - ts;
            //LoggerWrapper::instance()->warn("conn(%ld) update pong. packetid(%ld).", connId, packetId);
            break;
        }
    }

    
    pthread_mutex_unlock(&ping_mutex_);

    return 0;
}

int XMDCommonData::calculatePacketLoss(uint64_t connId, double& packetLossRate, int& packetTtl) {
    pthread_mutex_lock(&ping_mutex_);
    std::map<uint64_t, PingPackets>::iterator it = pingMap_.find(connId);
    if (it == pingMap_.end()) {
        LoggerWrapper::instance()->warn("conn(%ld) nost exist.", connId);
        pthread_mutex_unlock(&ping_mutex_);
        return -1;
    }
    PingPackets packets = it->second;
    pthread_mutex_unlock(&ping_mutex_);

    int totalPackets = 0;
    if (packets.pingVec.size() <= PING_TIMEOUT / PING_INTERVAL) {
        packetLossRate = 0;
        packetTtl = 0;
        return 0;
    } else if (packets.pingVec.size() < PING_PACKET_SIZE) {
        totalPackets = packets.pingVec.size() - PING_TIMEOUT / PING_INTERVAL;
    } else {
        totalPackets = CALCULATE_PACKET_LOSS_PACKET_NUM;
    }

    int startIndex = packets.pingIndex - PING_TIMEOUT / PING_INTERVAL;
    startIndex = startIndex < 0 ? startIndex + PING_PACKET_SIZE : startIndex;
    int ackedPacket = 0;
    int lostPacket = 0;
    int totalttl = 0;
    for (int i = 0; i < totalPackets; i++) {
        if (startIndex < 0) {
            startIndex = startIndex + PING_PACKET_SIZE;
        }
        if (packets.pingVec[startIndex].ttl == 0) {
            lostPacket++;
        } else {
            ackedPacket++;
            totalttl += packets.pingVec[startIndex].ttl;
        }
        startIndex--;
    }

    packetLossRate = double(lostPacket) / CALCULATE_PACKET_LOSS_PACKET_NUM;
    if (ackedPacket != 0) {
        packetTtl = totalttl / ackedPacket;
    } else {
        packetTtl = -1;
    }

    return 0;
}

uint64_t XMDCommonData::getPakcetId(uint64_t connId) {
    uint64_t packetid = 1;
    pthread_mutex_lock(&packetId_mutex_);
    std::unordered_map<uint64_t, uint64_t>::iterator it = packetIdMap_.find(connId);
    if (it == packetIdMap_.end()) {
        packetIdMap_[connId] = packetid + 1;
    } else {
        packetid = packetIdMap_[connId];
        packetIdMap_[connId] = packetid + 1;
    }
    pthread_mutex_unlock(&packetId_mutex_);

    return packetid;
}



void XMDCommonData::resendQueuePush(ResendData* data) {
    pthread_mutex_lock(&resend_queue_mutex_);
    resendQueue_.push(data);
    pthread_mutex_unlock(&resend_queue_mutex_);
}


ResendData* XMDCommonData::resendQueuePop() {
    ResendData* data = NULL;
    pthread_mutex_lock(&resend_queue_mutex_);
    uint64_t currentMs = current_ms();
    data = resendQueue_.top();
    if (data->reSendTime <= currentMs) {
        resendQueue_.pop();
    } else {
        data = NULL;
    }
    
    if (data != NULL) {
        std::stringstream ss_ack;
        ss_ack << data->connId << data->packetId;
        std::string ackpacketKey = ss_ack.str();
        if (getResendMapValue(ackpacketKey)) {
            delete data;
            data = NULL;
        }
    }
    pthread_mutex_unlock(&resend_queue_mutex_);
    return data;
}

void XMDCommonData::updateResendMap(std::string key, bool value) {
    //lock
    pthread_mutex_lock(&resend_map_mutex_);
    packetResendMap_[key] = value;
    pthread_mutex_unlock(&resend_map_mutex_);
}
bool XMDCommonData::getResendMapValue(std::string key) {
    pthread_mutex_lock(&resend_map_mutex_);
    std::unordered_map<std::string, bool>::iterator it = packetResendMap_.find(key);
    if (it == packetResendMap_.end()) {
        pthread_mutex_unlock(&resend_map_mutex_);
        return true;
    }
    bool result = it->second;
    if (it->second == true) {
        packetResendMap_.erase(it);
    }
    pthread_mutex_unlock(&resend_map_mutex_);
    return result;
}

uint64_t XMDCommonData::getLastPacketTime(std::string id) {
    uint64_t result = 0;
    pthread_rwlock_rdlock(&last_packet_time_mutex_);
    std::unordered_map<std::string, uint64_t>::iterator it = lastPacketTimeMap_.find(id);
    if (it != lastPacketTimeMap_.end()) {
        result = it->second;
    }
    pthread_rwlock_unlock(&last_packet_time_mutex_);
    return result;    
}

void XMDCommonData::updateLastPacketTime(std::string id, uint64_t value) {
    pthread_rwlock_wrlock(&last_packet_time_mutex_);
    lastPacketTimeMap_[id] = value;
    pthread_rwlock_unlock(&last_packet_time_mutex_);
}

void XMDCommonData::deleteLastPacketTime(std::string id) {
    pthread_rwlock_wrlock(&last_packet_time_mutex_);
    std::unordered_map<std::string, uint64_t>::iterator it = lastPacketTimeMap_.find(id);
    if (it != lastPacketTimeMap_.end()) {
        lastPacketTimeMap_.erase(it);
    }
    pthread_rwlock_unlock(&last_packet_time_mutex_);
}


netStatus XMDCommonData::getNetStatus(uint64_t connId) {
    netStatus status;
    pthread_rwlock_rdlock(&netstatus_mutex_);
    std::unordered_map<uint64_t, netStatus>::iterator it = netStatusMap_.find(connId);
    if (it != netStatusMap_.end()) {
        status = it->second;
    }
    pthread_rwlock_unlock(&netstatus_mutex_);
    return status;
}

void XMDCommonData::updateNetStatus(uint64_t connId, netStatus status) {
    pthread_rwlock_wrlock(&netstatus_mutex_);
    netStatusMap_[connId] = status;
    pthread_rwlock_unlock(&netstatus_mutex_);
}

void XMDCommonData::deleteNetStatus(uint64_t connId) {
    pthread_rwlock_wrlock(&netstatus_mutex_);
    std::unordered_map<uint64_t, netStatus>::iterator it = netStatusMap_.find(connId);
    if (it != netStatusMap_.end()) {
        netStatusMap_.erase(it);
    }
    pthread_rwlock_unlock(&netstatus_mutex_);
}


uint32_t XMDCommonData::getGroupId(uint64_t connId, uint16_t streamId) {
    uint32_t groupId = 0;
    std::stringstream ss;
    ss << connId << streamId;
    std::string key = ss.str();
    pthread_rwlock_wrlock(&group_id_mutex_);
    std::unordered_map<std::string, uint32_t>::iterator it = groupIdMap_.find(key);
    if (it != groupIdMap_.end()) {
        groupId = it->second;
        it->second = groupId + 1;
    } else {
        groupIdMap_[key] = 1;
    }
    pthread_rwlock_unlock(&group_id_mutex_);
    return groupId;
}

void XMDCommonData::deleteGroupId(std::string id) {
    pthread_rwlock_wrlock(&group_id_mutex_);
    std::unordered_map<std::string, uint32_t>::iterator it = groupIdMap_.find(id);
    if (it != groupIdMap_.end()) {
        groupIdMap_.erase(it);
    }
    pthread_rwlock_unlock(&group_id_mutex_);
}


bool XMDCommonData::getLastRecvGroupId(std::string id, uint32_t& groupId) {
    bool result = false;
    pthread_rwlock_rdlock(&last_recv_group_id_mutex_);
    std::unordered_map<std::string, uint32_t>::iterator it = lastRecvedGroupIdMap_.find(id);
    if (it != lastRecvedGroupIdMap_.end()) {
        groupId = it->second;
        result = true;
    }
    
    pthread_rwlock_unlock(&last_recv_group_id_mutex_);
    return result;
}

void XMDCommonData::updateLastRecvGroupId(std::string id, uint32_t groupId) {
    pthread_rwlock_wrlock(&last_recv_group_id_mutex_);
    lastRecvedGroupIdMap_[id] = groupId;
    pthread_rwlock_unlock(&last_recv_group_id_mutex_);
}

void XMDCommonData::deleteLastRecvGroupId(std::string id) {
    pthread_rwlock_wrlock(&last_recv_group_id_mutex_);
    std::unordered_map<std::string, uint32_t>::iterator it = lastRecvedGroupIdMap_.find(id);
    if (it != lastRecvedGroupIdMap_.end()) {
        lastRecvedGroupIdMap_.erase(it);
    }
    pthread_rwlock_unlock(&last_recv_group_id_mutex_);
}

uint32_t XMDCommonData::getLastCallbackGroupId(std::string id) {
    uint32_t groupId = 0;
    pthread_rwlock_rdlock(&last_callback_group_id_mutex_);
    std::unordered_map<std::string, uint32_t>::iterator it = lastCallbackGroupIdMap_.find(id);
    if (it != lastCallbackGroupIdMap_.end()) {
        groupId = it->second;
    }
    
    pthread_rwlock_unlock(&last_callback_group_id_mutex_);
    return groupId;
}

void XMDCommonData::updateLastCallbackGroupId(std::string id, uint32_t groupId) {
    pthread_rwlock_wrlock(&last_callback_group_id_mutex_);
    lastCallbackGroupIdMap_[id] = groupId;
    pthread_rwlock_unlock(&last_callback_group_id_mutex_);
}

void XMDCommonData::deleteLastCallbackGroupId(std::string id) {
    pthread_rwlock_wrlock(&last_callback_group_id_mutex_);
    std::unordered_map<std::string, uint32_t>::iterator it = lastCallbackGroupIdMap_.find(id);
    if (it != lastCallbackGroupIdMap_.end()) {
        lastCallbackGroupIdMap_.erase(it);
    }
    pthread_rwlock_unlock(&last_callback_group_id_mutex_);
}

void XMDCommonData::insertCallbackDataMap(std::string key, uint32_t groupId, CallbackQueueData* data) {
    pthread_mutex_lock(&callback_data_map_mutex_);
    std::unordered_map<std::string, std::map<uint32_t, CallbackQueueData*>>::iterator it = callbackDataMap_.find(key);
    if (it == callbackDataMap_.end()) {
        std::map<uint32_t, CallbackQueueData*> tmpMap;
        tmpMap[groupId] = data;
        callbackDataMap_[key] = tmpMap;
    } else {
        it->second[groupId] = data;
    }
    pthread_mutex_unlock(&callback_data_map_mutex_);
}

CallbackQueueData* XMDCommonData::getCallbackData(std::string key, uint32_t groupId) {
    CallbackQueueData* data = NULL;
    pthread_mutex_lock(&callback_data_map_mutex_);
    std::unordered_map<std::string, std::map<uint32_t, CallbackQueueData*>>::iterator it = callbackDataMap_.find(key);
    if (it != callbackDataMap_.end()) {
        std::map<uint32_t, CallbackQueueData*>::iterator it2 = it->second.begin();
        data = it2->second;
        if (data->groupId != groupId + 1) {
            data = NULL;
        } else {
            it->second.erase(it2);
        }
    }

    pthread_mutex_unlock(&callback_data_map_mutex_);
    return data;
}

void XMDCommonData::deletefromCallbackDataMap(std::string key) {
    pthread_mutex_lock(&callback_data_map_mutex_);
    std::unordered_map<std::string, std::map<uint32_t, CallbackQueueData*>>::iterator it = callbackDataMap_.find(key);
    if (it != callbackDataMap_.end()) {
        callbackDataMap_.erase(it);
    }
    pthread_mutex_unlock(&callback_data_map_mutex_);
}

void XMDCommonData::insertSendCallbackMap(std::string key, uint32_t groupSize) {
    pthread_mutex_lock(&stream_data_send_callback_map_mutex_);
    streamDataSendCallback data;
    data.groupSize = groupSize;
    streamDataSendCbMap_[key] = data;
    pthread_mutex_unlock(&stream_data_send_callback_map_mutex_);
}

int XMDCommonData::updateSendCallbackMap(std::string key, uint32_t sliceId) {
    int result = -1;
    pthread_mutex_lock(&stream_data_send_callback_map_mutex_);
    std::unordered_map<std::string, streamDataSendCallback>::iterator it = streamDataSendCbMap_.find(key);
    if (it != streamDataSendCbMap_.end()) {
        it->second.sliceMap[sliceId] = true;
        if (it->second.sliceMap.size() == it->second.groupSize) {
            streamDataSendCbMap_.erase(it);
            result = 1;
        } else {
            result = 0;
        }
    }
    pthread_mutex_unlock(&stream_data_send_callback_map_mutex_);
    return result;
}

void XMDCommonData::deleteSendCallbackMap(std::string key) {
    pthread_mutex_lock(&stream_data_send_callback_map_mutex_);
    std::unordered_map<std::string, streamDataSendCallback>::iterator it = streamDataSendCbMap_.find(key);
    if (it != streamDataSendCbMap_.end()) {
        streamDataSendCbMap_.erase(it);
    }
    pthread_mutex_unlock(&stream_data_send_callback_map_mutex_);
}



void XMDCommonData::insertAckPacketmap(std::string key, ackPacketInfo info) {
    pthread_mutex_lock(&ack_packet_map_mutex_);
    ackPacketMap_[key] = info;
    pthread_mutex_unlock(&ack_packet_map_mutex_);
}

bool XMDCommonData::getDeleteAckPacketInfo(std::string key, ackPacketInfo& info) {
    bool result = false;
    pthread_mutex_lock(&ack_packet_map_mutex_);
    std::unordered_map<std::string, ackPacketInfo>::iterator it = ackPacketMap_.find(key);
    if (it != ackPacketMap_.end()) {
        info = it->second;
        ackPacketMap_.erase(it);
        result = true;
    }
    pthread_mutex_unlock(&ack_packet_map_mutex_);
    return result;
}




