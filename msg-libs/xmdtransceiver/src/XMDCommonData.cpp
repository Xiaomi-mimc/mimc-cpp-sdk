#include "XMDCommonData.h"
#include "XMDLoggerWrapper.h"
#include "common.h"
#include <sstream>

pthread_mutex_t XMDCommonData::resend_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::datagram_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::packetId_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::callback_data_map_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::stream_data_send_callback_map_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t XMDCommonData::packet_loss_map_mutex_ = PTHREAD_MUTEX_INITIALIZER;


XMDCommonData::XMDCommonData(int decodeThreadSize) {
    pthread_rwlock_init(&conn_mutex_, NULL);
    pthread_rwlock_init(&group_id_mutex_, NULL);

    decodeThreadSize_ = decodeThreadSize;
    for (int i = 0; i < decodeThreadSize; i++) {
        STLSafeQueue<StreamData*> tmpQueue;
        packetRecoverQueueVec_.push_back(tmpQueue);
    }

    connVec_.clear();

    datagramQueueMaxLen_ = DEFAULT_DATAGRAM_QUEUE_LEN;
    callbackQueueMaxLen_ = DEFAULT_CALLBACK_QUEUE_LEN;
    resendQueueMaxLen_ = DEFAULT_RESEND_QUEUE_LEN;
    callbackQueueSize_ = 0;
    datagramQueueSize_ = 0;
    resendQueueSize_ = 0;
}

XMDCommonData::~XMDCommonData() {
    pthread_rwlock_destroy(&conn_mutex_);
    pthread_rwlock_destroy(&group_id_mutex_);
    connVec_.clear();
}


void XMDCommonData::streamQueuePush(StreamQueueData * data) {
    streamQueue_.Push(data);
}

StreamQueueData* XMDCommonData::streamQueuePop() {
    StreamQueueData* data = NULL;
    bool result = streamQueue_.Pop(data);
    if (result) {
        return data;
    } else {
        return NULL;
    }
}

void XMDCommonData::socketSendQueuePush(SendQueueData* data) {
    socketSendQueue_.Push(data);
}

SendQueueData* XMDCommonData::socketSendQueuePop() {
    SendQueueData* data = NULL;
    bool result = socketSendQueue_.Pop(data);
    if (result) {
        return data;
    } else {
        return NULL;
    }
}

void XMDCommonData::socketRecvQueuePush(SocketData* data) {
    socketRecvQueue_.Push(data);
}

SocketData* XMDCommonData::socketRecvQueuePop() {
    SocketData* data = NULL;
    bool result = socketRecvQueue_.Pop(data);
    if (result) {
        return data;
    } else {
        return NULL;
    }
}



void XMDCommonData::packetRecoverQueuePush(StreamData* data, int id) {
    if (id >= (int)packetRecoverQueueVec_.size()) {
        XMDLoggerWrapper::instance()->warn("packetRecoverQueuePush invalid thread id:%d", id);
        delete data;
        return;
    }
    packetRecoverQueueVec_[id].Push(data);
}

StreamData* XMDCommonData::packetRecoverQueuePop(int id) {
    StreamData* data = NULL;
    if (id >= (int)packetRecoverQueueVec_.size()) {
        XMDLoggerWrapper::instance()->warn("packetRecoverQueuePop invalid thread id:%d, queue size=%d", 
                                            id, packetRecoverQueueVec_.size());
        return NULL;
    }

    bool result = packetRecoverQueueVec_[id].Pop(data);
    if (result) {
        return data;
    } else {
        return NULL;
    }
}

bool XMDCommonData::callbackQueuePush(CallbackQueueData* data) {
    if (callbackQueueSize_ >= callbackQueueMaxLen_) {
        XMDLoggerWrapper::instance()->warn("callbackQueue size(%d) bigger than queue max len(%d)", 
                                           callbackQueueSize_, callbackQueueMaxLen_);
        delete data;
        return false;
    }
    callbackQueue_.Push(data);
    callbackQueueSize_++;
    return true;
}

CallbackQueueData* XMDCommonData::callbackQueuePop() {
    CallbackQueueData* data = NULL;
    bool result = callbackQueue_.Pop(data);
    if (result) {
        callbackQueueSize_--;
        return data;
    } else {
        return NULL;
    }
}

void XMDCommonData::setCallbackQueueSize(int size) {
    callbackQueueMaxLen_ = size;
}

float XMDCommonData::getCallbackQueueUsegeRate() {
    return float(callbackQueueSize_) / float(callbackQueueMaxLen_);
}

void XMDCommonData::clearCallbackQueue() {
    CallbackQueueData* data = NULL;
    while (!callbackQueue_.empty()) {
        callbackQueue_.Pop(data);
    }
}


bool XMDCommonData::datagramQueuePush(SendQueueData* data) {
    pthread_mutex_lock(&datagram_queue_mutex_);
    if (datagramQueueSize_ >= datagramQueueMaxLen_) {
        XMDLoggerWrapper::instance()->warn("datagramQueue size(%d) bigger than queue max len(%d)", 
                                           datagramQueueSize_, datagramQueueMaxLen_);
        pthread_mutex_unlock(&datagram_queue_mutex_);
        delete data;
        return false;
    }
    datagramQueue_.push(data);
    datagramQueueSize_++;
    pthread_mutex_unlock(&datagram_queue_mutex_);
    return true;
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
        datagramQueueSize_--;
    } else {
        data = NULL;
    }
    pthread_mutex_unlock(&datagram_queue_mutex_);
    return data;
}


void XMDCommonData::setDatagramQueueSize(int size) {
    datagramQueueMaxLen_ = size;
}

float XMDCommonData::getDatagramQueueUsageRate() {
    return float(datagramQueueSize_) / float(datagramQueueMaxLen_);
}

void XMDCommonData::clearDatagramQueue() {
    pthread_mutex_lock(&datagram_queue_mutex_);
    while (!datagramQueue_.empty()) {
        datagramQueue_.pop();
    }
    datagramQueueSize_ = 0;
    pthread_mutex_unlock(&datagram_queue_mutex_);
}


bool XMDCommonData::isConnExist(uint64_t connId) {
    pthread_rwlock_rdlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("isConnExist connection(%ld) not exist", connId);
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
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
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
    
    pthread_rwlock_unlock(&conn_mutex_);

    deleteFromConnVec(connId);

    pingMap_.deleteMapVaule(connId);

    deletePacketLossInfoMap(connId);

    deleteNetStatus(connId);

    pthread_mutex_lock(&packetId_mutex_);
    std::unordered_map<uint64_t, uint64_t>::iterator packetIdIt = packetIdMap_.find(connId);
    if (packetIdIt != packetIdMap_.end()) {
        packetIdMap_.erase(packetIdIt);
    } 
    pthread_mutex_unlock(&packetId_mutex_);

    std::stringstream ss_conn;
    ss_conn << connId;
    std::string connKey = ss_conn.str();
    deleteLastPacketTime(connKey);
    for (unsigned int i = 0; i <= maxStreamId; i++) {
        std::stringstream ss;
        ss << connId << i;
        std::string key = ss.str();
        deleteLastPacketTime(key);
        //deleteLastRecvGroupId(key);
        deleteGroupId(key);
        deleteLastCallbackGroupId(key);
        deletefromCallbackDataMap(key);
    }
    
    return 0;
}

bool XMDCommonData::getConnInfo(uint64_t connId, ConnInfo& cInfo) {
    pthread_rwlock_rdlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("getConnInfo connection(%ld) not exist", connId);
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

bool XMDCommonData::deleteFromConnVec(uint64_t connId) {
    pthread_rwlock_rdlock(&conn_mutex_);
    std::vector<uint64_t>::iterator it = connVec_.begin();
    for (; it != connVec_.end(); it++) {
        if (*it == connId) {
            connVec_.erase(it);
            break;
        }
    }
    pthread_rwlock_unlock(&conn_mutex_);

    return true;
}


bool XMDCommonData::isStreamExist(uint64_t connId, uint16_t streamId) {
    pthread_rwlock_rdlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return false;
    }
    
    std::unordered_map<uint16_t, StreamInfo>::iterator it = itConn->second.streamMap.find(streamId);
    if (it == itConn->second.streamMap.end()) {
        XMDLoggerWrapper::instance()->debug("isStreamExist stream(%d) not exist.", streamId);
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
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return -1;
    }
    
    ConnInfo& connInfo = it->second;
    if (!connInfo.connState == CONNECTED) {
        XMDLoggerWrapper::instance()->warn("insertStream connection(%ld) has not been established.", connId);
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
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return -1;
    }
    ConnInfo& connInfo = itConn->second;

    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamId);
    if (it == connInfo.streamMap.end()) {
        XMDLoggerWrapper::instance()->warn("stream(%d) not exist.", streamId);
        pthread_rwlock_unlock(&conn_mutex_);
        return -1;
    }
    connInfo.streamMap.erase(it);
    pthread_rwlock_unlock(&conn_mutex_);

    std::stringstream ss;
    ss << connId << streamId;
    std::string tmpKey = ss.str();
    deletefromCallbackDataMap(tmpKey);
    return 0;
}

bool XMDCommonData::getStreamInfo(uint64_t connId, uint16_t streamId, StreamInfo& sInfo) {
    pthread_rwlock_rdlock(&conn_mutex_);
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        pthread_rwlock_unlock(&conn_mutex_);
        return false;
    }
    ConnInfo& connInfo = itConn->second;
    

    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamId);
    if (it == connInfo.streamMap.end()) {
        XMDLoggerWrapper::instance()->warn("stream(%d) not exist.", streamId);
        pthread_rwlock_unlock(&conn_mutex_);
        return false;
    }
    sInfo = it->second;
    pthread_rwlock_unlock(&conn_mutex_);

    return true;
}


int XMDCommonData::insertPing(PingPacket packet) {
    pingMap_.updateMapVaule(packet.connId, packet);
    return 0;
}

bool XMDCommonData::getPingPacket(uint64_t connId, PingPacket& packet) {
    return pingMap_.getMapValue(connId, packet);
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



bool XMDCommonData::resendQueuePush(ResendData* data) {
    pthread_mutex_lock(&resend_queue_mutex_);
    if (resendQueueSize_ >= resendQueueMaxLen_) {
        XMDLoggerWrapper::instance()->warn("resendQueue size(%d) bigger than queue max len(%d)", 
                                           resendQueueSize_, resendQueueMaxLen_);
        pthread_mutex_unlock(&resend_queue_mutex_);
        delete data;
        return false;
    }
    resendQueue_.push(data);
    resendQueueSize_++;
    pthread_mutex_unlock(&resend_queue_mutex_);
    return true;
}


ResendData* XMDCommonData::resendQueuePop() {
    ResendData* data = NULL;
    pthread_mutex_lock(&resend_queue_mutex_);
    uint64_t currentMs = current_ms();
    data = resendQueue_.top();
    if (data->reSendTime <= currentMs) {
        resendQueue_.pop();
        resendQueueSize_--;
    } else {
        data = NULL;
    }
    
    if (data != NULL) {
        std::stringstream ss_ack;
        ss_ack << data->connId << data->packetId;
        std::string ackpacketKey = ss_ack.str();
        if (getIsPacketRecvAckMapValue(ackpacketKey)) {
            delete data;
            data = NULL;
        }
    }
    pthread_mutex_unlock(&resend_queue_mutex_);
    return data;
}

void XMDCommonData::setResendQueueSize(int size) {
    resendQueueMaxLen_ = size;
}

float XMDCommonData::getResendQueueUsageRate() {
    return float(resendQueueSize_) / float(resendQueueMaxLen_);
}

void XMDCommonData::clearResendQueue() {
    pthread_mutex_lock(&resend_queue_mutex_);
    while (!resendQueue_.empty()) {
        resendQueue_.pop();
    }
    resendQueueSize_ = 0;
    pthread_mutex_unlock(&resend_queue_mutex_);
}


void XMDCommonData::updateIsPacketRecvAckMap(std::string key, bool value) {
    bool result = false;
    if (isPacketRecvAckMap_.getMapValue(key, result)) {
        isPacketRecvAckMap_.updateMapVaule(key, value);
    }
}
bool XMDCommonData::getIsPacketRecvAckMapValue(std::string key) {
    bool result = false;
    bool isfound = isPacketRecvAckMap_.getMapValue(key, result);
    if (!isfound) {
        return true;
    }

    if (result) {
        isPacketRecvAckMap_.deleteMapVaule(key);
    }
    
    return result;
}
bool XMDCommonData::deleteIsPacketRecvAckMap(std::string key) {
    return isPacketRecvAckMap_.deleteMapVaule(key);
}
void XMDCommonData::insertIsPacketRecvAckMap(std::string key, bool value) {
    isPacketRecvAckMap_.updateMapVaule(key, value);
}


uint64_t XMDCommonData::getLastPacketTime(std::string id) {
    uint64_t result = 0;
    lastPacketTimeMap_.getMapValue(id, result);
    return result;    
}

void XMDCommonData::updateLastPacketTime(std::string id, uint64_t value) {
    lastPacketTimeMap_.updateMapVaule(id, value);
}

void XMDCommonData::deleteLastPacketTime(std::string id) {
    lastPacketTimeMap_.deleteMapVaule(id);
}


netStatus XMDCommonData::getNetStatus(uint64_t connId) {
    netStatus status;
    netStatusMap_.getMapValue(connId, status);
    return status;
}

void XMDCommonData::updateNetStatus(uint64_t connId, netStatus status) {
    netStatusMap_.updateMapVaule(connId, status);
}

void XMDCommonData::deleteNetStatus(uint64_t connId) {
    netStatusMap_.deleteMapVaule(connId);
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

/*
bool XMDCommonData::getLastRecvGroupId(std::string id, uint32_t& groupId) {
    bool result = lastRecvedGroupIdMap_.getMapValue(id, groupId);
    return result;
}

void XMDCommonData::updateLastRecvGroupId(std::string id, uint32_t groupId) {
    lastRecvedGroupIdMap_.updateMapVaule(id, groupId);
}

void XMDCommonData::deleteLastRecvGroupId(std::string id) {
    lastRecvedGroupIdMap_.deleteMapVaule(id);
}*/

bool XMDCommonData::getLastCallbackGroupId(std::string id, uint32_t& groupId) {
    return lastCallbackGroupIdMap_.getMapValue(id, groupId);
}

void XMDCommonData::updateLastCallbackGroupId(std::string id, uint32_t groupId) {
    lastCallbackGroupIdMap_.updateMapVaule(id, groupId);
}

void XMDCommonData::deleteLastCallbackGroupId(std::string id) {
    lastCallbackGroupIdMap_.deleteMapVaule(id);
}

void XMDCommonData::insertCallbackDataMap(std::string key, uint32_t groupId, int waitTime, CallbackQueueData* data) {
    pthread_mutex_lock(&callback_data_map_mutex_);
    std::unordered_map<std::string, CallBackSortBuffer>::iterator it = callbackDataMap_.find(key);
    if (it == callbackDataMap_.end()) {
        std::map<uint32_t, CallbackQueueData*> tmpMap;
        tmpMap[groupId] = data;
        CallBackSortBuffer buffer;
        buffer.StreamWaitTime = waitTime;
        buffer.groupMap = tmpMap;
        callbackDataMap_[key] = buffer;
    } else {
        CallBackSortBuffer& buffer = it->second;
        buffer.groupMap[groupId] = data;
    }
    pthread_mutex_unlock(&callback_data_map_mutex_);
}

int XMDCommonData::getCallbackData(std::string key, uint32_t groupId, CallbackQueueData* &data) {
    int result = 0;
    pthread_mutex_lock(&callback_data_map_mutex_);
    std::unordered_map<std::string, CallBackSortBuffer>::iterator it = callbackDataMap_.find(key);
    if (it != callbackDataMap_.end()) {
        if (it->second.groupMap.size() == 0) {
            result = -1;
        } else {
            std::map<uint32_t, CallbackQueueData*>::iterator it2 = it->second.groupMap.begin();
            data = it2->second;
            if (data->groupId > groupId + 1) {
                if ((it->second.StreamWaitTime != -1) && ((int)(current_ms() - data->recvTime) > it->second.StreamWaitTime)) {
                    it->second.groupMap.erase(it2);
                    result = 1;
                } else {
                    result = -1;
                }
            } else {
                it->second.groupMap.erase(it2);
                result = 0;
            }
        }
    } else {
        result = -1;
    }

    pthread_mutex_unlock(&callback_data_map_mutex_);
    return result;
}

void XMDCommonData::deletefromCallbackDataMap(std::string key) {
    pthread_mutex_lock(&callback_data_map_mutex_);
    std::unordered_map<std::string, CallBackSortBuffer>::iterator it = callbackDataMap_.find(key);
    if (it != callbackDataMap_.end()) {
        std::map<uint32_t, CallbackQueueData*>::iterator it2 = it->second.groupMap.begin();
        for (; it2 != it->second.groupMap.end(); it2++) {
            delete it2->second;
        }
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

bool XMDCommonData::SendCallbackMapRecordExist(std::string key) {
    bool result = false;
    pthread_mutex_lock(&stream_data_send_callback_map_mutex_);
    std::unordered_map<std::string, streamDataSendCallback>::iterator it = streamDataSendCbMap_.find(key);
    if (it != streamDataSendCbMap_.end()) {
        result = true;
    }
    pthread_mutex_unlock(&stream_data_send_callback_map_mutex_);

    return result;
}




void XMDCommonData::insertPacketCallbackInfoMap(std::string key, PacketCallbackInfo info) {
    packetCallbackInfoMap_.updateMapVaule(key, info);
}

bool XMDCommonData::getDeletePacketCallbackInfo(std::string key, PacketCallbackInfo& info) {
    bool result = packetCallbackInfoMap_.getMapValue(key, info);
    if (result) {
        packetCallbackInfoMap_.deleteMapVaule(key);
    }
    return result;
}

void XMDCommonData::insertPacketLossInfoMap(uint64_t connId) {
    pthread_mutex_lock(&packet_loss_map_mutex_);
    PacketLossInfo packet;
    PacketLossInfoMap_[connId] = packet;
    pthread_mutex_unlock(&packet_loss_map_mutex_);
}

void XMDCommonData::updatePacketLossInfoMap(uint64_t connId, uint64_t packetId) {
    //XMDLoggerWrapper::instance()->debug("updatePacketLossInfoMap connid(%ld), packetid(%ld)", connId, packetId);
    pthread_mutex_lock(&packet_loss_map_mutex_);
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it != PacketLossInfoMap_.end()) {
        PacketLossInfo& packet = it->second;
        if (packetId > packet.caculatePacketId) {
            packet.newPacketCount++;
            if (packetId > packet.maxPacketId) {
                packet.maxPacketId = packetId;
            }
        } else {
            if (packetId > packet.minPacketId) {
                packet.oldPakcetCount++;
            }
        }
    }
    
    pthread_mutex_unlock(&packet_loss_map_mutex_);
}

void XMDCommonData::deletePacketLossInfoMap(uint64_t connId) {
    pthread_mutex_lock(&packet_loss_map_mutex_);
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it != PacketLossInfoMap_.end()) {
        PacketLossInfoMap_.erase(it);
    }
    pthread_mutex_unlock(&packet_loss_map_mutex_);
}

bool XMDCommonData::getPacketLossInfo(uint64_t connId, PacketLossInfo& data) {
    pthread_mutex_lock(&packet_loss_map_mutex_);
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it == PacketLossInfoMap_.end()) {
        pthread_mutex_unlock(&packet_loss_map_mutex_);
        return false;
    }
    data = it->second;
    pthread_mutex_unlock(&packet_loss_map_mutex_);
    return true;
}

void XMDCommonData::startPacketLossCalucate(uint64_t connId) {
    pthread_mutex_lock(&packet_loss_map_mutex_);
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it != PacketLossInfoMap_.end()) {
        PacketLossInfo& packet = it->second;
        packet.minPacketId = packet.caculatePacketId;
        packet.caculatePacketId = packet.maxPacketId;
        packet.oldPakcetCount = packet.newPacketCount;
        packet.newPacketCount = 0;
    }
    pthread_mutex_unlock(&packet_loss_map_mutex_);
}



void XMDCommonData::pongThreadQueuePush(PongThreadData data) {
    pongThreadQueue_.Push(data);
}

bool XMDCommonData::pongThreadQueuePop(PongThreadData& data) {
    if (pongThreadQueue_.Front(data)) {
        uint64_t currentTime = current_ms();
        if (currentTime > data.ts + CALUTE_PACKET_LOSS_DELAY) {
            pongThreadQueue_.Pop(data);
            return true;
        }
    }
    return false;
}


int getResendTimeInterval(int sendcount) {
    if (sendcount == 1) {
        return FIRST_RESEND_INTERVAL;
    } else if (sendcount == 2) {
        return SECOND_RESEND_INTERVAL;
    } else {
        return THIRD_RESEND_INTERVAL;
    }
}



