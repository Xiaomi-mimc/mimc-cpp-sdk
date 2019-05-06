#include "XMDCommonData.h"
#include "XMDLoggerWrapper.h"
#include "common.h"
#include <sstream>

std::mutex XMDCommonData::resend_queue_mutex_;
std::mutex XMDCommonData::datagram_queue_mutex_;
std::mutex XMDCommonData::packetId_mutex_;
std::mutex XMDCommonData::callback_data_map_mutex_;
std::mutex XMDCommonData::stream_data_send_callback_map_mutex_;
std::mutex XMDCommonData::packet_loss_map_mutex_;
std::mutex XMDCommonData::conn_mutex_;
std::mutex XMDCommonData::group_id_mutex_;
std::mutex XMDCommonData::socket_send_queue_mutex_;


XMDCommonData::XMDCommonData(int decodeThreadSize) {
    decodeThreadSize_ = decodeThreadSize;
    for (int i = 0; i < decodeThreadSize; i++) {
        STLSafeQueue<StreamData*> tmpQueue;
        packetRecoverQueueVec_.push_back(tmpQueue);
    }

    connVec_.clear();

    datagramQueueMaxLen_ = DEFAULT_DATAGRAM_QUEUE_LEN;
    callbackQueueMaxLen_ = DEFAULT_CALLBACK_QUEUE_LEN;
    resendQueueMaxLen_ = DEFAULT_RESEND_QUEUE_LEN;
    ping_interval_ = PING_INTERVAL / 1000;
    resend_interval_ = FIRST_RESEND_INTERVAL;
}

XMDCommonData::~XMDCommonData() {
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

bool XMDCommonData::streamQueueEmpty() {
    return streamQueue_.empty();
}

void XMDCommonData::socketSendQueuePush(SendQueueData* data) {
    socket_send_queue_mutex_.lock();
    socketSendQueue_.push(data);
    socket_send_queue_mutex_.unlock();
}

SendQueueData* XMDCommonData::socketSendQueuePop() {
    SendQueueData* data = NULL;
    socket_send_queue_mutex_.lock();
    if (socketSendQueue_.empty()) {
        socket_send_queue_mutex_.unlock();
        return NULL;
    }
    data = socketSendQueue_.front();
    socketSendQueue_.pop();
    socket_send_queue_mutex_.unlock();

    return data;
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

bool XMDCommonData::socketRecvQueueEmpty() {
    return socketRecvQueue_.empty();
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

bool XMDCommonData::packetRecoverQueueEmpty(int id) {
    return packetRecoverQueueVec_[id].empty();
}


bool XMDCommonData::callbackQueuePush(CallbackQueueData* data) {
    if (callbackQueue_.Size() >= callbackQueueMaxLen_) {
        XMDLoggerWrapper::instance()->warn("callbackQueue size(%d) bigger than queue max len(%d)", 
                                           callbackQueue_.Size(), callbackQueueMaxLen_);
        delete data;
        return false;
    }
    callbackQueue_.Push(data);
    return true;
}

CallbackQueueData* XMDCommonData::callbackQueuePop() {
    CallbackQueueData* data = NULL;
    bool result = callbackQueue_.Pop(data);
    if (result) {
        return data;
    } else {
        return NULL;
    }
}

bool XMDCommonData::callbackQueueEmpty() {
    return callbackQueue_.empty();
}


void XMDCommonData::setCallbackQueueSize(int size) {
    callbackQueueMaxLen_ = size;
}

int XMDCommonData::getCallbackQueueSize() {
    return callbackQueueMaxLen_;
}


float XMDCommonData::getCallbackQueueUsegeRate() {
    return float(callbackQueue_.Size()) / float(callbackQueueMaxLen_);
}

void XMDCommonData::clearCallbackQueue() {
    CallbackQueueData* data = NULL;
    while (!callbackQueue_.empty()) {
        callbackQueue_.Pop(data);
    }
}


bool XMDCommonData::datagramQueuePush(SendQueueData* data) {
	datagram_queue_mutex_.lock();
	if (datagramQueue_.size() >= datagramQueueMaxLen_) {
		XMDLoggerWrapper::instance()->warn("datagramQueue size(%d) bigger than queue max len(%d)",
											datagramQueue_.size(), datagramQueueMaxLen_);
        datagram_queue_mutex_.unlock();
        delete data;
        return false;
    }
    datagramQueue_.push(data);
    datagram_queue_mutex_.unlock();
    return true;
}

SendQueueData* XMDCommonData::datagramQueuePriorityPop() {
    SendQueueData* data = NULL;
    datagram_queue_mutex_.lock();
    if (datagramQueue_.empty()) {
        datagram_queue_mutex_.unlock();
        return NULL;
    }
    data = datagramQueue_.top();
    uint64_t currentMs = current_ms();
    if (data->sendTime <= currentMs) {
        datagramQueue_.pop();
    } else {
        data = NULL;
    }
    datagram_queue_mutex_.unlock();
    return data;
}

SendQueueData* XMDCommonData::datagramQueuePop() {
    SendQueueData* data = NULL;
    datagram_queue_mutex_.lock();
    if (datagramQueue_.empty()) {
        datagram_queue_mutex_.unlock();
        return NULL;
    }
    data = datagramQueue_.top();
    datagramQueue_.pop();
    datagram_queue_mutex_.unlock();

    return data;
}



void XMDCommonData::setDatagramQueueSize(int size) {
    datagramQueueMaxLen_ = size;
}

float XMDCommonData::getDatagramQueueUsageRate() {
    return float(datagramQueue_.size()) / float(datagramQueueMaxLen_);
}

void XMDCommonData::clearDatagramQueue() {
    datagram_queue_mutex_.lock();
    while (!datagramQueue_.empty()) {
        datagramQueue_.pop();
    }
    datagram_queue_mutex_.unlock();
}


bool XMDCommonData::isConnExist(uint64_t connId) {
	conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("isConnExist connection(%ld) not exist", connId);
		conn_mutex_.unlock();
        return false;
    }
	conn_mutex_.unlock();
    return true;
}


int XMDCommonData::insertConn(uint64_t connId, ConnInfo connInfo) {
	conn_mutex_.lock();
    connectionMap_[connId] = connInfo;
    connVec_.push_back(connId);
	conn_mutex_.unlock();
    return 0;
}
int XMDCommonData::updateConn(uint64_t connId, ConnInfo connInfo) {
	conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
		conn_mutex_.unlock();
        return -1;
    }
    
    connectionMap_[connId] = connInfo;
	conn_mutex_.unlock();
    return 0;
}

int XMDCommonData::deleteConn(uint64_t connId) {
    uint32_t maxStreamId = 0;
	conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it != connectionMap_.end()) {
        maxStreamId = it->second.max_stream_id;
    }
    
	conn_mutex_.unlock();

    deleteFromConnVec(connId);

    pingMap_.deleteMapVaule(connId);

    deletePacketLossInfoMap(connId);

    deleteNetStatus(connId);

    packetId_mutex_.lock();
    std::unordered_map<uint64_t, uint64_t>::iterator packetIdIt = packetIdMap_.find(connId);
    if (packetIdIt != packetIdMap_.end()) {
        packetIdMap_.erase(packetIdIt);
    } 
    packetId_mutex_.unlock();

    std::stringstream ss_conn;
    ss_conn << connId;
    std::string connKey = ss_conn.str();
    deleteLastPacketTime(connKey);
    for (unsigned int i = 0; i <= maxStreamId; i++) {
        std::stringstream ss;
        ss << connId << i;
        std::string key = ss.str();
        deleteLastPacketTime(key);
        deleteGroupId(key);
        deleteLastCallbackGroupId(key);
        deletefromCallbackDataMap(key);
    }
    
    return 0;
}

bool XMDCommonData::getConnInfo(uint64_t connId, ConnInfo& cInfo) {
	conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("getConnInfo connection(%ld) not exist", connId);
		conn_mutex_.unlock();
        return false;
    }
    cInfo = connectionMap_[connId];
	conn_mutex_.unlock();
    return true;
}

uint32_t XMDCommonData::getConnStreamId(uint64_t connId) {
    uint32_t streamId = 0;
	conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it != connectionMap_.end()) {
        ConnInfo& info = it->second;
        streamId = info.created_stream_id + 2;
        info.created_stream_id = streamId;
    }
	conn_mutex_.unlock();
    return streamId;
}

int XMDCommonData::updateConnIpInfo(uint64_t connId, uint32_t ip, int port) {
	conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it != connectionMap_.end()) {
        ConnInfo& info = it->second;
        info.ip = ip;
        info.port = port;
    } else {
		conn_mutex_.unlock();
        return -1;
    }
    
	conn_mutex_.unlock();
    return 0;
}



std::vector<uint64_t> XMDCommonData::getConnVec() {
    std::vector<uint64_t> tmpvec; 
	conn_mutex_.lock();
    tmpvec = connVec_;
	conn_mutex_.unlock();
    return tmpvec;
}

bool XMDCommonData::deleteFromConnVec(uint64_t connId) {
	conn_mutex_.lock();
    std::vector<uint64_t>::iterator it = connVec_.begin();
    for (; it != connVec_.end(); it++) {
        if (*it == connId) {
            connVec_.erase(it);
            break;
        }
    }
	conn_mutex_.unlock();

    return true;
}


bool XMDCommonData::isStreamExist(uint64_t connId, uint16_t streamId) {
	conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
		conn_mutex_.unlock();
        return false;
    }
    
    std::unordered_map<uint16_t, StreamInfo>::iterator it = itConn->second.streamMap.find(streamId);
    if (it == itConn->second.streamMap.end()) {
        XMDLoggerWrapper::instance()->debug("isStreamExist stream(%d) not exist.", streamId);
		conn_mutex_.unlock();
        return false;
    }
	conn_mutex_.unlock();
    return true;
}


int XMDCommonData::insertStream(uint64_t connId, uint16_t streamId, StreamInfo streamInfo) {
	conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        conn_mutex_.unlock();
        return -1;
    }
    
    ConnInfo& connInfo = it->second;
    if (!connInfo.connState == CONNECTED) {
        XMDLoggerWrapper::instance()->warn("insertStream connection(%ld) has not been established.", connId);
        conn_mutex_.unlock();
        return -1;
    }
    if (connInfo.max_stream_id < streamId) {
        connInfo.max_stream_id = streamId;
    }
    connInfo.streamMap[streamId] = streamInfo;
    conn_mutex_.unlock();
    return 0;
}

int XMDCommonData::deleteStream(uint64_t connId, uint16_t streamId) {
    conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        conn_mutex_.unlock();
        return -1;
    }
    ConnInfo& connInfo = itConn->second;

    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamId);
    if (it == connInfo.streamMap.end()) {
        XMDLoggerWrapper::instance()->warn("stream(%d) not exist.", streamId);
        conn_mutex_.unlock();
        return -1;
    }
    connInfo.streamMap.erase(it);
    conn_mutex_.unlock();

    std::stringstream ss;
    ss << connId << streamId;
    std::string tmpKey = ss.str();
    deletefromCallbackDataMap(tmpKey);
    return 0;
}

bool XMDCommonData::getStreamInfo(uint64_t connId, uint16_t streamId, StreamInfo& sInfo) {
    conn_mutex_.lock();
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%ld) not exist", connId);
        conn_mutex_.unlock();
        return false;
    }
    ConnInfo& connInfo = itConn->second;
    

    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamId);
    if (it == connInfo.streamMap.end()) {
        XMDLoggerWrapper::instance()->warn("stream(%d) not exist.", streamId);
        conn_mutex_.unlock();
        return false;
    }
    sInfo = it->second;
    conn_mutex_.unlock();

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
    packetId_mutex_.lock();
    std::unordered_map<uint64_t, uint64_t>::iterator it = packetIdMap_.find(connId);
    if (it == packetIdMap_.end()) {
        packetIdMap_[connId] = packetid + 1;
    } else {
        packetid = packetIdMap_[connId];
        packetIdMap_[connId] = packetid + 1;
    }
    packetId_mutex_.unlock();

    return packetid;
}



bool XMDCommonData::resendQueuePush(ResendData* data) {
    resend_queue_mutex_.lock();
	if (resendQueue_.size() >= resendQueueMaxLen_) {
        XMDLoggerWrapper::instance()->warn("resendQueue size(%d) bigger than queue max len(%d)", 
										resendQueue_.size(), resendQueueMaxLen_);
        resend_queue_mutex_.unlock();
        delete data;
        return false;
    }
    resendQueue_.push(data);
    resend_queue_mutex_.unlock();
    return true;
}


ResendData* XMDCommonData::resendQueuePriorityPop() {
    ResendData* data = NULL;
    resend_queue_mutex_.lock();
	if (resendQueue_.empty()) {
        resend_queue_mutex_.unlock();
        return NULL;
    }
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
        if (getIsPacketRecvAckMapValue(ackpacketKey)) {
            delete data;
            data = NULL;
        }
    }
    resend_queue_mutex_.unlock();
    return data;
}

ResendData* XMDCommonData::resendQueuePop() {
    ResendData* data = NULL;
    resend_queue_mutex_.lock();
    if (resendQueue_.empty()) {
        resend_queue_mutex_.unlock();
        return NULL;
    }
    data = resendQueue_.top();
    resendQueue_.pop();
    resend_queue_mutex_.unlock();
    return data;
}


void XMDCommonData::setResendQueueSize(int size) {
    resendQueueMaxLen_ = size;
}

int XMDCommonData::getResendQueueSize() {
    return resendQueueMaxLen_;
}


float XMDCommonData::getResendQueueUsageRate() {
    return float(resendQueue_.size()) / float(resendQueueMaxLen_);
}

void XMDCommonData::clearResendQueue() {
    resend_queue_mutex_.lock();
    while (!resendQueue_.empty()) {
        resendQueue_.pop();
    }
    resend_queue_mutex_.unlock();
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
	group_id_mutex_.lock();
    std::unordered_map<std::string, uint32_t>::iterator it = groupIdMap_.find(key);
    if (it != groupIdMap_.end()) {
        groupId = it->second;
        it->second = groupId + 1;
    } else {
        groupIdMap_[key] = 1;
    }
    group_id_mutex_.unlock();
    return groupId;
}

void XMDCommonData::deleteGroupId(std::string id) {
    group_id_mutex_.lock();
    std::unordered_map<std::string, uint32_t>::iterator it = groupIdMap_.find(id);
    if (it != groupIdMap_.end()) {
        groupIdMap_.erase(it);
    }
    group_id_mutex_.unlock();
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
    callback_data_map_mutex_.lock();
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
    callback_data_map_mutex_.unlock();
}

int XMDCommonData::getCallbackData(std::string key, uint32_t groupId, CallbackQueueData* &data) {
    int result = 0;
    callback_data_map_mutex_.lock();
    std::unordered_map<std::string, CallBackSortBuffer>::iterator it = callbackDataMap_.find(key);
    if (it != callbackDataMap_.end()) {
        if (it->second.groupMap.size() == 0) {
            result = -1;
        } else {
            std::map<uint32_t, CallbackQueueData*>::iterator it2 = it->second.groupMap.begin();
            data = it2->second;
            if (data->groupId > groupId + 1) {
                if ((it->second.StreamWaitTime != -1) && ((int)(current_ms() - data->recvTime) > it->second.StreamWaitTime)) {
                    XMDLoggerWrapper::instance()->info("callback wait timeout,connid=%ld,streamid=%d,groupid=%d",
                                                         data->connId, data->streamId, groupId + 1);
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

    callback_data_map_mutex_.unlock();
    return result;
}

void XMDCommonData::deletefromCallbackDataMap(std::string key) {
    callback_data_map_mutex_.lock();
    std::unordered_map<std::string, CallBackSortBuffer>::iterator it = callbackDataMap_.find(key);
    if (it != callbackDataMap_.end()) {
        std::map<uint32_t, CallbackQueueData*>::iterator it2 = it->second.groupMap.begin();
        for (; it2 != it->second.groupMap.end(); it2++) {
            delete it2->second;
        }
        callbackDataMap_.erase(it);
    }
    callback_data_map_mutex_.unlock();
}

void XMDCommonData::insertSendCallbackMap(std::string key, uint32_t groupSize) {
    stream_data_send_callback_map_mutex_.lock();
    streamDataSendCallback data;
    data.groupSize = groupSize;
    streamDataSendCbMap_[key] = data;
    stream_data_send_callback_map_mutex_.unlock();
}

int XMDCommonData::updateSendCallbackMap(std::string key, uint32_t sliceId) {
    int result = -1;
    stream_data_send_callback_map_mutex_.lock();
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
    stream_data_send_callback_map_mutex_.unlock();
    return result;
}

void XMDCommonData::deleteSendCallbackMap(std::string key) {
    stream_data_send_callback_map_mutex_.lock();
    std::unordered_map<std::string, streamDataSendCallback>::iterator it = streamDataSendCbMap_.find(key);
    if (it != streamDataSendCbMap_.end()) {
        streamDataSendCbMap_.erase(it);
    }
    stream_data_send_callback_map_mutex_.unlock();
}

bool XMDCommonData::SendCallbackMapRecordExist(std::string key) {
    bool result = false;
    stream_data_send_callback_map_mutex_.lock();
    std::unordered_map<std::string, streamDataSendCallback>::iterator it = streamDataSendCbMap_.find(key);
    if (it != streamDataSendCbMap_.end()) {
        result = true;
    }
    stream_data_send_callback_map_mutex_.unlock();

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
    packet_loss_map_mutex_.lock();
    PacketLossInfo packet;
    PacketLossInfoMap_[connId] = packet;
    packet_loss_map_mutex_.unlock();
}

void XMDCommonData::updatePacketLossInfoMap(uint64_t connId, uint64_t packetId) {
    //XMDLoggerWrapper::instance()->debug("updatePacketLossInfoMap connid(%ld), packetid(%ld)", connId, packetId);
    packet_loss_map_mutex_.lock();
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
    
    packet_loss_map_mutex_.unlock();
}

void XMDCommonData::deletePacketLossInfoMap(uint64_t connId) {
    packet_loss_map_mutex_.lock();
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it != PacketLossInfoMap_.end()) {
        PacketLossInfoMap_.erase(it);
    }
    packet_loss_map_mutex_.unlock();
}

bool XMDCommonData::getPacketLossInfo(uint64_t connId, PacketLossInfo& data) {
    packet_loss_map_mutex_.lock();
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it == PacketLossInfoMap_.end()) {
        packet_loss_map_mutex_.unlock();
        return false;
    }
    data = it->second;
    packet_loss_map_mutex_.unlock();
    return true;
}

void XMDCommonData::startPacketLossCalucate(uint64_t connId) {
    packet_loss_map_mutex_.lock();
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it != PacketLossInfoMap_.end()) {
        PacketLossInfo& packet = it->second;
        packet.minPacketId = packet.caculatePacketId;
        packet.caculatePacketId = packet.maxPacketId;
        packet.oldPakcetCount = packet.newPacketCount;
        packet.newPacketCount = 0;
    }
    packet_loss_map_mutex_.unlock();
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

void XMDCommonData::NotifyBlockQueue() {
}
/*
bool XMDCommonData::SendQueueHasNewData() {
    XMDLoggerWrapper::instance()->debug("has new data");
    if (datagramQueue_.empty() && socketSendQueue_.empty() && resendQueue_.empty()) {
        std::unique_lock <std::mutex> lck(send_thread_mutex_);
        XMDLoggerWrapper::instance()->debug("has new data lock");
        con_var_send_thread_.wait(lck);
    }
    return true;
}
*/



