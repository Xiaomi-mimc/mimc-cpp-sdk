#include "XMDCommonData.h"
#include "XMDLoggerWrapper.h"
#include "common.h"
#include <sstream>
#include "MutexLock.h"


XMDCommonData::XMDCommonData(unsigned int workerThreadSize, unsigned int timerThreadSize) {
    workerThreadSize_ = workerThreadSize;
    for (unsigned int i = 0; i < workerThreadSize; i++) {
        STLBlockQueue<WorkerThreadData*> tmpQueue;
        workerQueueVec_.push_back(tmpQueue);
        workerQueueSizeVec_.push_back(0);
        pthread_mutex_t worker_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
        worker_queue_mutex_vec_.push_back(worker_queue_mutex_);
        workerQueueMaxSizeVec_.push_back(WORKER_QUEUE_MAX_SIZE);
    }


    timerThreadSize_ = timerThreadSize;
    for (unsigned int i = 0; i < timerThreadSize_; i++) {
        TimerThreadQueue timerqueue;
        timerQueueVec_.push_back(timerqueue);
        pthread_mutex_t timer_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
        timer_queue_mutex_vec_.push_back(timer_queue_mutex_);
        timerQueueMaxSizeVec_.push_back(DEFAULT_RESEND_QUEUE_SIZE);
        timerQueueSizeVec_.push_back(0);
        
    }

    socketSendQueueSize_ = 0;
    socketSendQueueMaxSize_ = SOCKET_SENDQUEUE_MAX_SIZE;

    group_id_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    stream_id_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    ping_interval_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    socket_send_queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;

}

XMDCommonData::~XMDCommonData() {
    while(!socketSendQueueEmpty()) {
        SendQueueData* sendData = socketSendQueuePop();
        if (NULL != sendData) {
            delete sendData;
        }
    }

    for (size_t i = 0; i < workerQueueVec_.size(); i++) {
        while (!workerQueueEmpty(i)) {
            WorkerThreadData* workerThreadData = workerQueuePop(i);
            deleteWorkerData(workerThreadData);
        }
        workerQueueSizeVec_[i] = 0;
    }

    for (size_t j = 0; j < timerQueueVec_.size(); j++) {
        while (!timerQueueEmpty(j)) {
            TimerThreadData* timerThreadData = timerQueuePop(j);
            if (timerThreadData != NULL) {
                deleteWorkerData((WorkerThreadData*)timerThreadData->data);
                delete timerThreadData;
            }
        }
        timerQueueSizeVec_[j] = 0;
    }
}

void XMDCommonData::workerQueuePush(WorkerThreadData* data, uint64_t connid) {
    unsigned int id = connid % workerThreadSize_;
    {
    MutexLock lock(&worker_queue_mutex_vec_[id]);
    workerQueueSizeVec_[id] += (data->len + sizeof(WorkerThreadData));
    }
    workerQueueVec_[id].Push(data);
}
WorkerThreadData* XMDCommonData::workerQueuePop(unsigned int id) {
    WorkerThreadData* data = NULL;
    if (id >= workerThreadSize_) {
        XMDLoggerWrapper::instance()->warn("workerQueuePop invalid thread id:%d, queue size=%d", 
                                                id, workerThreadSize_);
        return NULL;
    }
    bool result = workerQueueVec_[id].Pop(data);
    if (result) {
        MutexLock lock(&worker_queue_mutex_vec_[id]);
        workerQueueSizeVec_[id] -= (data->len + sizeof(WorkerThreadData));
        return data;
    } else {
        return NULL;
    }

}
bool XMDCommonData::workerQueueEmpty(unsigned int id) {
    if (id >= workerThreadSize_) {
        XMDLoggerWrapper::instance()->warn("workerQueueEmpty invalid thread id:%d, queue size=%d", 
                                                id, workerThreadSize_);
        return false;
    }
    bool result = workerQueueVec_[id].empty();
    return result;
}

bool XMDCommonData::isWorkerQueueFull(unsigned int id, unsigned int len) {
    return workerQueueSizeVec_[id] + len > workerQueueMaxSizeVec_[id];
}

int XMDCommonData::getWorkerQueueUsedSize(int id) {
    MutexLock lock(&worker_queue_mutex_vec_[id]);
    return workerQueueSizeVec_[id];
}

void XMDCommonData::setWorkerQueueMaxSize(int id, int size) {
    MutexLock lock(&worker_queue_mutex_vec_[id]);
    workerQueueMaxSizeVec_[id] = size;
}


int XMDCommonData::getWorkerQueueMaxSize(int id) {
    MutexLock lock(&worker_queue_mutex_vec_[id]);
    return workerQueueMaxSizeVec_[id];
}

float XMDCommonData::getWorkerQueueUsageRate(int id) {
    MutexLock lock(&worker_queue_mutex_vec_[id]);
    return float(workerQueueSizeVec_[id]) /  float(workerQueueMaxSizeVec_[id]);
}

void XMDCommonData::clearWorkerQueue(int id) {
}



void XMDCommonData::setTimerQueueMaxSize(int queueId, int size) {
    MutexLock lock(&timer_queue_mutex_vec_[queueId]);
    timerQueueMaxSizeVec_[queueId] = size;
}

float XMDCommonData::getTimerQueueUsageRate(int queueId) {
    MutexLock lock(&timer_queue_mutex_vec_[queueId]);
    return float(timerQueueSizeVec_[queueId]) /  float(timerQueueMaxSizeVec_[queueId]);
}

bool XMDCommonData::isTimerQueueFull(int queueId, unsigned int len) {
    return timerQueueSizeVec_[queueId] + len > timerQueueMaxSizeVec_[queueId];
}

int XMDCommonData::getTimerQueueUsedSize(int queueId) {
    MutexLock lock(&timer_queue_mutex_vec_[queueId]);
    return timerQueueSizeVec_[queueId];
}

int XMDCommonData::getTimerQueueMaxSize(int queueId) {
    MutexLock lock(&timer_queue_mutex_vec_[queueId]);
    return timerQueueMaxSizeVec_[queueId];
}


void XMDCommonData::clearTimerQueue(int queueId) {
    MutexLock lock(&timer_queue_mutex_vec_[queueId]);
    std::vector<TimerThreadData*> tmpVec;
    while (!timerQueueVec_[queueId].empty()) {
        TimerThreadData* timerData = timerQueueVec_[queueId].top();
        timerQueueVec_[queueId].pop();
        WorkerThreadData* workerData = (WorkerThreadData*)timerData->data;
        if (workerData->dataType == WORKER_RESEND_TIMEOUT || workerData->dataType == WORKER_DATAGREAM_TIMEOUT) {
            SendQueueData* queueData = (SendQueueData*)workerData->data;
            delete queueData;
            delete workerData;
            delete timerData;
        } else {
            tmpVec.push_back(timerData);
        }
    }

    timerQueueSizeVec_[queueId] = 0;

    for (size_t i = 0; i < tmpVec.size(); i++) {
        timerQueueVec_[queueId].push(tmpVec[i]);
        timerQueueSizeVec_[queueId] += tmpVec[i]->len;
    }

}


bool XMDCommonData::timerQueuePush(TimerThreadData* data , uint64_t connid) {
    unsigned int id = connid % timerThreadSize_;
    MutexLock lock(&timer_queue_mutex_vec_[id]);
    timerQueueSizeVec_[id] += data->len;
    timerQueueVec_[id].push(data);
    return true;
}

size_t XMDCommonData::timerQueueSize(uint64_t connId) {
    unsigned int id = connId % timerThreadSize_;
    MutexLock lock(&timer_queue_mutex_vec_[id]);
    return timerQueueSizeVec_[id];
}


TimerThreadData* XMDCommonData::timerQueuePop(unsigned int id) {
    TimerThreadData* data = NULL;
    if (id >= timerThreadSize_) {
        XMDLoggerWrapper::instance()->warn("timerQueuePop invalid thread id:%d, queue size=%d", 
                                                id, timerThreadSize_);
        return NULL;
    }

    MutexLock lock(&timer_queue_mutex_vec_[id]);
    if (timerQueueVec_[id].empty()) {
        return NULL;
    }
    
    data = timerQueueVec_[id].top();
    timerQueueVec_[id].pop();
    timerQueueSizeVec_[id] -= data->len;
    return data;

}

TimerThreadData* XMDCommonData::timerQueuePriorityPop(unsigned int id) {
    TimerThreadData* data = NULL;
    if (id >= timerThreadSize_) {
        XMDLoggerWrapper::instance()->warn("timerQueuePriorityPop invalid thread id:%d, queue size=%d", 
                                                id, timerThreadSize_);
        return NULL;
    }
    MutexLock lock(&timer_queue_mutex_vec_[id]);
    if (timerQueueVec_[id].empty()) {
        return NULL;
    }
    data = timerQueueVec_[id].top();
    uint64_t currentMs = current_ms();
    if (data->time <= currentMs) {
        timerQueueVec_[id].pop();
        timerQueueSizeVec_[id] -= data->len;
    } else {
        data = NULL;
    }
    return data;
}

bool XMDCommonData::timerQueueEmpty(unsigned int id) {
    if (id >= timerThreadSize_) {
        XMDLoggerWrapper::instance()->warn("timerQueueEmpty invalid thread id:%d, queue size=%d", 
                                                id, timerThreadSize_);
        return false;
    }
    MutexLock lock(&timer_queue_mutex_vec_[id]);
    bool result = timerQueueVec_[id].empty();
    return result;
}


bool XMDCommonData::isSocketSendQueueFull(unsigned int len) {
    MutexLock lock(&socket_send_queue_mutex_);
    return socketSendQueueSize_ + len > socketSendQueueMaxSize_;
}

void XMDCommonData::setSocketSendQueueMaxSize(int size) {
    MutexLock lock(&socket_send_queue_mutex_);
    socketSendQueueMaxSize_ = size;
}

int XMDCommonData::getSocketSendQueueMaxSize() {
    MutexLock lock(&socket_send_queue_mutex_);
    return socketSendQueueMaxSize_;
}

int XMDCommonData::getSocketSendQueueUsedSize() {
    MutexLock lock(&socket_send_queue_mutex_);
    return socketSendQueueSize_;
}

float XMDCommonData::getSocketSendQueueUsageRate() {
    MutexLock lock(&socket_send_queue_mutex_);
    return float(socketSendQueueSize_) /  float(socketSendQueueMaxSize_);
}



void XMDCommonData::socketSendQueuePush(SendQueueData* data) {
    {
    MutexLock lock(&socket_send_queue_mutex_);
    socketSendQueueSize_ += (sizeof(SendQueueData) + data->len);
    }
    socketSendQueue_.Push(data);
}

SendQueueData* XMDCommonData::socketSendQueuePop() {
    SendQueueData* data = NULL;
    socketSendQueue_.Pop(data);
    if (data != NULL) {
        MutexLock lock(&socket_send_queue_mutex_);
        socketSendQueueSize_ -= (sizeof(SendQueueData) + data->len);
    }
    return data;
}

void XMDCommonData::clearSocketQueue() {
}


uint32_t XMDCommonData::getGroupId(uint64_t connId, uint16_t streamId) {
    uint32_t groupId = 0;
    std::stringstream ss;
    ss << connId << streamId;
    std::string key = ss.str();
    MutexLock lock(&group_id_mutex_);
    std::unordered_map<std::string, uint32_t>::iterator it = groupIdMap_.find(key);
    if (it != groupIdMap_.end()) {
        groupId = it->second;
        it->second = groupId + 1;
    } else {
        groupIdMap_[key] = 1;
    }
    return groupId;
}

void XMDCommonData::deleteGroupId(std::string id) {
    MutexLock lock(&group_id_mutex_);
    std::unordered_map<std::string, uint32_t>::iterator it = groupIdMap_.find(id);
    if (it != groupIdMap_.end()) {
        groupIdMap_.erase(it);
    }
}

void XMDCommonData::addConnStreamId(uint64_t connId, uint16_t streamId) {
    MutexLock lock(&group_id_mutex_);
    streamIdMap_[connId] = streamId;
}

uint32_t XMDCommonData::getConnStreamId(uint64_t connId) {
    uint16_t streamId = 0;
    MutexLock lock(&group_id_mutex_);
    std::unordered_map<uint64_t, uint16_t>::iterator it = streamIdMap_.find(connId);
    if (it != streamIdMap_.end()) {
        uint16_t& tmpId = it->second;
        tmpId += 2;
        streamId = tmpId;
    }
    
    return streamId;
}

void XMDCommonData::deleteConnStreamId(uint64_t connId) {
    MutexLock lock(&group_id_mutex_);
    std::unordered_map<uint64_t, uint16_t>::iterator it = streamIdMap_.find(connId);
    if (it != streamIdMap_.end()) {
        streamIdMap_.erase(it);
    }
}


void XMDCommonData::NotifyBlockQueue() {
    for (unsigned int i = 0; i < workerThreadSize_; i++) {
        workerQueueVec_[i].Notify();
    }
    socketSendQueue_.Notify();
}

bool XMDCommonData::SendQueueHasNewData() {
    return true;
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


void XMDCommonData::SetPingIntervalMs(uint64_t connId, uint16_t value) {
    value = value < PING_INTERVAL ? PING_INTERVAL : value;
    MutexLock lock(&ping_interval_mutex_);
    pingIntervalMap_[connId] = value;
}

uint16_t XMDCommonData::GetPingIntervalMs(uint64_t connId) {
    MutexLock lock(&ping_interval_mutex_);
    std::unordered_map<uint64_t, uint16_t>::iterator it = pingIntervalMap_.find(connId);
    if (it != pingIntervalMap_.end()) {
        return it->second;
    }
    return PING_INTERVAL;
}

void XMDCommonData::DeletePingIntervalMap(uint64_t connId) {
    MutexLock lock(&ping_interval_mutex_);
    std::unordered_map<uint64_t, uint16_t>::iterator it = pingIntervalMap_.find(connId);
    if (it != pingIntervalMap_.end()) {
        pingIntervalMap_.erase(it);
    }
}



void XMDCommonData::deleteWorkerData(WorkerThreadData* workerThreadData) {
    switch (workerThreadData->dataType)
    {
        case WORKER_CREATE_NEW_CONN: {
            NewConnWorkerData* connData = (NewConnWorkerData*)workerThreadData->data;
            delete connData;
            break;
        }
        case WORKER_CLOSE_CONN: {
            WorkerData* workerData = (WorkerData*)workerThreadData->data;
            delete workerData;
            break;
        }
        case WORKER_CREATE_STREAM: {
            NewStreamWorkerData* workerData = (NewStreamWorkerData*)workerThreadData->data;
            delete workerData;
            break;
        }
        case WORKER_CLOSE_STREAM: {
            WorkerData* workerData = (WorkerData*)workerThreadData->data;
            delete workerData;
            break;
        }
        case WORKER_SEND_RTDATA: {
            RTWorkerData* workerData = (RTWorkerData*)workerThreadData->data;
            delete workerData;
            break;
        }
        case WORKER_RECV_DATA: {
            SocketData* socketData = (SocketData*)workerThreadData->data;
            delete socketData;
            break;
        }
        case WORKER_RESEND_TIMEOUT: {
            SendQueueData* resendData = (SendQueueData*)workerThreadData->data;
            delete resendData;
            break;
        }
        case WORKER_DATAGREAM_TIMEOUT: {
            SendQueueData* queueData = (SendQueueData*)workerThreadData->data;
            delete queueData;
            break;
        }
        case WORKER_DELETE_ACKGROUP_TIMEOUT: {
            DeleteGroupTimeout* delteData = (DeleteGroupTimeout*)workerThreadData->data;
            delete delteData;
            break;
        }
        case WORKER_DELETE_FECGROUP_TIMEOUT: {
            DeleteGroupTimeout* delteData = (DeleteGroupTimeout*)workerThreadData->data;
            delete delteData;
            break;
        }
        case WORKER_CHECK_CALLBACK_BUFFER: {
            CheckCallbackBufferData* key = (CheckCallbackBufferData*)workerThreadData->data;
            delete key;
            break;
        }
        case WORKER_PING_TIMEOUT: {
            WorkerData* workerData = (WorkerData*)workerThreadData->data;
            delete workerData;
            break;
        }
        case WORKER_PONG_TIMEOUT: {
            PongThreadData* pongthreadData = (PongThreadData*)workerThreadData->data;
            delete pongthreadData;
            break;
        }
        case WORKER_CHECK_CONN_AVAILABLE: {
            WorkerData* workerData = (WorkerData*)workerThreadData->data;
            delete workerData;
            break;
        }
        case WORKER_TEST_RTT: {
            TestRttPacket* testRtt = (TestRttPacket*)workerThreadData->data;
            delete testRtt;
            break;
        }
        case WORKER_BATCH_ACK: {
            WorkerData* bacthAckData = (WorkerData*)workerThreadData->data;
            delete bacthAckData;
            break;
        }
    }

    delete workerThreadData;
}


WorkerCommonData::~WorkerCommonData() {
    closeAllConn();
    std::unordered_map<std::string, CallBackSortBuffer>::iterator it = callbackDataMap_.begin();
    for (; it != callbackDataMap_.end();) {
        std::map<uint32_t, CallbackQueueData*>::iterator it2 = it->second.groupMap.begin();
        for (; it2 != it->second.groupMap.end(); it2++) {
            delete it2->second;
        }
        callbackDataMap_.erase(it++);
    }
}


netStatus WorkerCommonData::getNetStatus(uint64_t connId) {
    netStatus status;
    netStatusMap_.getMapValue(connId, status);
    return status;
}

void WorkerCommonData::updateNetStatus(uint64_t connId, netStatus status) {
    netStatusMap_.updateMapVaule(connId, status);
    commonData_->updateNetStatus(connId, status);
}

void WorkerCommonData::deleteNetStatus(uint64_t connId) {
    netStatusMap_.deleteMapVaule(connId);
}


bool WorkerCommonData::isConnExist(uint64_t connId) {
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("isConnExist connection(%llu) not exist", connId);
        return false;
    }
    return true;
}


int WorkerCommonData::insertConn(uint64_t connId, ConnInfo connInfo) {
    connectionMap_[connId] = connInfo;
    return 0;
}
int WorkerCommonData::updateConn(uint64_t connId, ConnInfo connInfo) {
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%llu) not exist", connId);
        return -1;
    }
    
    connectionMap_[connId] = connInfo;
    return 0;
}

int WorkerCommonData::deleteConn(uint64_t connId) {
    uint32_t maxStreamId = 0;
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it != connectionMap_.end()) {
        maxStreamId = it->second.max_stream_id;
        connectionMap_.erase(it);
    }

    deleteConnResource(connId, maxStreamId);

    return 0;
}

int WorkerCommonData::deleteConnResource(uint64_t connId, uint32_t maxStreamId) {
    pingMap_.deleteMapVaule(connId);

    deletePacketLossInfoMap(connId);

    deleteNetStatus(connId);
    commonData_->deleteNetStatus(connId);

    deleteFromResendInterval(connId);

    commonData_->deleteConnStreamId(connId);
    commonData_->DeletePingIntervalMap(connId);

    deletePacketIdMap(connId);

    std::stringstream ss_conn;
    ss_conn << connId;
    std::string connKey = ss_conn.str();
    deleteLastPacketTime(connKey);
    for (unsigned int i = 0; i <= maxStreamId; i++) {
        std::stringstream ss;
        ss << connId << i;
        std::string key = ss.str();
        deleteLastPacketTime(key);
        commonData_->deleteGroupId(key);
        deleteLastCallbackGroupId(key);
        deletefromCallbackDataMap(key);
    }

    deleteBatchAckMap(connId);
    
    return 0;
}


bool WorkerCommonData::getConnInfo(uint64_t connId, ConnInfo& cInfo) {
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("getConnInfo connection(%llu) not exist", connId);
        return false;
    }
    cInfo = it->second;
    return true;
}

int WorkerCommonData::updateConnIpInfo(uint64_t connId, uint32_t ip, int port) {
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it != connectionMap_.end()) {
        ConnInfo& info = it->second;
        info.ip = ip;
        info.port = port;
    } else {
        return -1;
    }
    
    return 0;
}

void WorkerCommonData::closeAllConn() {
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.begin();
    for (; it != connectionMap_.end(); ) {
        uint64_t connId = it->first;
        uint32_t maxStreamId = it->second.max_stream_id;
        deleteConnResource(connId, maxStreamId);
        connectionMap_.erase(it++);
    }
}




bool WorkerCommonData::isStreamExist(uint64_t connId, uint16_t streamId) {
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%llu) not exist", connId);
        return false;
    }
    
    std::unordered_map<uint16_t, StreamInfo>::iterator it = itConn->second.streamMap.find(streamId);
    if (it == itConn->second.streamMap.end()) {
        XMDLoggerWrapper::instance()->debug("isStreamExist stream(%d) not exist.", streamId);
        return false;
    }

    return true;
}


int WorkerCommonData::insertStream(uint64_t connId, uint16_t streamId, StreamInfo streamInfo) {
    std::unordered_map<uint64_t, ConnInfo>::iterator it = connectionMap_.find(connId);
    if (it == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%llu) not exist", connId);
        return -1;
    }
    
    ConnInfo& connInfo = it->second;
    if (connInfo.connState != CONNECTED) {
        XMDLoggerWrapper::instance()->warn("insertStream connection(%ld) has not been established.", connId);
        return -1;
    }
    if (connInfo.max_stream_id < streamId) {
        connInfo.max_stream_id = streamId;
    }
    connInfo.streamMap[streamId] = streamInfo;
    return 0;
}

int WorkerCommonData::deleteStream(uint64_t connId, uint16_t streamId) {
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%llu) not exist", connId);
        return -1;
    }
    ConnInfo& connInfo = itConn->second;

    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamId);
    if (it == connInfo.streamMap.end()) {
        XMDLoggerWrapper::instance()->warn("delete stream(%d) not exist.", streamId);
        return -1;
    }
    connInfo.streamMap.erase(it);

    std::stringstream ss;
    ss << connId << streamId;
    std::string tmpKey = ss.str();
    deletefromCallbackDataMap(tmpKey);
    return 0;
}

bool WorkerCommonData::getStreamInfo(uint64_t connId, uint16_t streamId, StreamInfo& sInfo) {
    std::unordered_map<uint64_t, ConnInfo>::iterator itConn = connectionMap_.find(connId);
    if (itConn == connectionMap_.end()) {
        XMDLoggerWrapper::instance()->debug("connection(%llu) not exist", connId);
        return false;
    }
    ConnInfo& connInfo = itConn->second;
    

    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamId);
    if (it == connInfo.streamMap.end()) {
        XMDLoggerWrapper::instance()->warn("get stream(%d) not exist.", streamId);
        return false;
    }
    sInfo = it->second;

    return true;
}


int WorkerCommonData::insertPing(PingPacket packet) {
    pingMap_.updateMapVaule(packet.connId, packet);
    return 0;
}

bool WorkerCommonData::getPingPacket(uint64_t connId, PingPacket& packet) {
    return pingMap_.getMapValue(connId, packet);
}


uint64_t WorkerCommonData::getPakcetId(uint64_t connId) {
    uint64_t packetid = 1;
    std::unordered_map<uint64_t, uint64_t>::iterator it = packetIdMap_.find(connId);
    if (it == packetIdMap_.end()) {
        packetIdMap_[connId] = packetid + 1;
    } else {
        packetid = packetIdMap_[connId];
        packetIdMap_[connId] = packetid + 1;
    }

    return packetid;
}

void WorkerCommonData::deletePacketIdMap(uint64_t connId) {
    std::unordered_map<uint64_t, uint64_t>::iterator packetIdIt = packetIdMap_.find(connId);
    if (packetIdIt != packetIdMap_.end()) {
        packetIdMap_.erase(packetIdIt);
    } 
}


void WorkerCommonData::updateIsPacketRecvAckMap(std::string key, bool value) {
    bool result = false;
    if (isPacketRecvAckMap_.getMapValue(key, result)) {
        isPacketRecvAckMap_.updateMapVaule(key, value);
    }
}
bool WorkerCommonData::getIsPacketRecvAckMapValue(std::string key) {
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
bool WorkerCommonData::deleteIsPacketRecvAckMap(std::string key) {
    return isPacketRecvAckMap_.deleteMapVaule(key);
}
void WorkerCommonData::insertIsPacketRecvAckMap(std::string key, bool value) {
    isPacketRecvAckMap_.updateMapVaule(key, value);
}


uint64_t WorkerCommonData::getLastPacketTime(std::string id) {
    uint64_t result = 0;
    lastPacketTimeMap_.getMapValue(id, result);
    return result;    
}

void WorkerCommonData::updateLastPacketTime(std::string id, uint64_t value) {
    lastPacketTimeMap_.updateMapVaule(id, value);
}

void WorkerCommonData::deleteLastPacketTime(std::string id) {
    lastPacketTimeMap_.deleteMapVaule(id);
}






bool WorkerCommonData::getLastCallbackGroupId(std::string id, uint32_t& groupId) {
    return lastCallbackGroupIdMap_.getMapValue(id, groupId);
}

void WorkerCommonData::updateLastCallbackGroupId(std::string id, uint32_t groupId) {
    lastCallbackGroupIdMap_.updateMapVaule(id, groupId);
}

void WorkerCommonData::deleteLastCallbackGroupId(std::string id) {
    lastCallbackGroupIdMap_.deleteMapVaule(id);
}

int WorkerCommonData::insertCallbackDataMap(std::string key, uint32_t groupId, int waitTime, CallbackQueueData* data) {
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
        std::map<uint32_t, CallbackQueueData*>::iterator iter = buffer.groupMap.find(groupId);
        if (iter == buffer.groupMap.end()) {
            buffer.groupMap[groupId] = data;
        } else {
            delete data;
            return 1;
        }
    }
    return 0;
}

bool WorkerCommonData::isCallbackBufferMapEmpty(std::string key) {
    std::unordered_map<std::string, CallBackSortBuffer>::iterator it = callbackDataMap_.find(key);
    if (it != callbackDataMap_.end()) {
        if (it->second.groupMap.size() != 0) {
            return false;
        }
    }

    return true;
}


int WorkerCommonData::getCallbackData(std::string key, uint32_t groupId, CallbackQueueData* &data) {
    int result = 0;
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
            } else if ((int)it->second.groupMap.size() >= MAX_CALLBACK_DATA_MAP_SIZE) {
                XMDLoggerWrapper::instance()->debug("callback map reach max size, connid=%ld,streamid=%d,groupid=%d, groupMap.size=%d",
                                                                     data->connId, data->streamId, groupId + 1, it->second.groupMap.size());
                it->second.groupMap.erase(it2);
                result = 1;
            } else {
                it->second.groupMap.erase(it2);
                result = 0;
            }
        }
    } else {
        result = -1;
    }

    return result;
}

void WorkerCommonData::deletefromCallbackDataMap(std::string key) {
    std::unordered_map<std::string, CallBackSortBuffer>::iterator it = callbackDataMap_.find(key);
    if (it != callbackDataMap_.end()) {
        std::map<uint32_t, CallbackQueueData*>::iterator it2 = it->second.groupMap.begin();
        for (; it2 != it->second.groupMap.end(); it2++) {
            delete it2->second;
        }
        callbackDataMap_.erase(it);
    }
}

void WorkerCommonData::insertSendCallbackMap(std::string key, uint32_t groupSize) {
    streamDataSendCallback data;
    data.groupSize = groupSize;
    streamDataSendCbMap_[key] = data;
}

int WorkerCommonData::updateSendCallbackMap(std::string key, uint32_t sliceId) {
    int result = -1;
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
    return result;
}

void WorkerCommonData::deleteSendCallbackMap(std::string key) {
    std::unordered_map<std::string, streamDataSendCallback>::iterator it = streamDataSendCbMap_.find(key);
    if (it != streamDataSendCbMap_.end()) {
        streamDataSendCbMap_.erase(it);
    }
}

bool WorkerCommonData::SendCallbackMapRecordExist(std::string key) {
    bool result = false;
    std::unordered_map<std::string, streamDataSendCallback>::iterator it = streamDataSendCbMap_.find(key);
    if (it != streamDataSendCbMap_.end()) {
        result = true;
    }

    return result;
}

bool WorkerCommonData::getDeleteCallbackMapRecord(std::string key) {
    bool result = false;
    std::unordered_map<std::string, streamDataSendCallback>::iterator it = streamDataSendCbMap_.find(key);
    if (it != streamDataSendCbMap_.end()) {
        result = true;
        streamDataSendCbMap_.erase(it);
    }

    return result;
}




void WorkerCommonData::insertPacketCallbackInfoMap(std::string key, PacketCallbackInfo info) {
    packetCallbackInfoMap_.updateMapVaule(key, info);
}

bool WorkerCommonData::getDeletePacketCallbackInfo(std::string key, PacketCallbackInfo& info) {
    bool result = packetCallbackInfoMap_.getMapValue(key, info);
    if (result) {
        packetCallbackInfoMap_.deleteMapVaule(key);
    }
    return result;
}

bool WorkerCommonData::getPacketCallbackInfo(std::string key, PacketCallbackInfo& info) {
    bool result = packetCallbackInfoMap_.getMapValue(key, info);
    return result;
}

bool WorkerCommonData::deletePacketCallbackInfo(std::string key) {
    return packetCallbackInfoMap_.deleteMapVaule(key);
}


void WorkerCommonData::insertPacketLossInfoMap(uint64_t connId) {
    PacketLossInfo packet;
    PacketLossInfoMap_[connId] = packet;
}

void WorkerCommonData::updatePacketLossInfoMap(uint64_t connId, uint64_t packetId) {
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it != PacketLossInfoMap_.end()) {
        PacketLossInfo& packet = it->second;
        if (packetId > packet.caculatePacketId) {
            packet.newPacketCount++;
            packet.newPakcetSet.insert(packetId);
            if (packetId > packet.maxPacketId) {
                packet.maxPacketId = packetId;
            }
        } else {
            if (packetId > packet.minPacketId) {
                packet.oldPakcetCount++;
                packet.oldPakcetSet.insert(packetId);
            }
        }
    }
    
}

void WorkerCommonData::deletePacketLossInfoMap(uint64_t connId) {
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it != PacketLossInfoMap_.end()) {
        PacketLossInfoMap_.erase(it);
    }
}

bool WorkerCommonData::getPacketLossInfo(uint64_t connId, PacketLossInfo& data) {
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it == PacketLossInfoMap_.end()) {
        return false;
    }
    data = it->second;
    return true;
}

void WorkerCommonData::startPacketLossCalucate(uint64_t connId) {
    std::unordered_map<uint64_t, PacketLossInfo>::iterator it = PacketLossInfoMap_.find(connId);
    if (it != PacketLossInfoMap_.end()) {
        PacketLossInfo& packet = it->second;
        packet.minPacketId = packet.caculatePacketId;
        packet.caculatePacketId = packet.maxPacketId;
        packet.oldPakcetCount = packet.newPacketCount;
        packet.newPacketCount = 0;
        
        packet.oldPakcetSet = packet.newPakcetSet;
        packet.newPakcetSet.clear();
    }
}

void WorkerCommonData::startTimer(uint64_t connId, WorkerThreadDataType type, uint64_t time, void* data, int len) {
    //XMDLoggerWrapper::instance()->warn("start timer len ==== %d", len);
    WorkerThreadData* workerThreadData = new WorkerThreadData(type, data, len);

    TimerThreadData* timerThreadData = new TimerThreadData();
    timerThreadData->connId = connId;
    timerThreadData->time = current_ms() + time;
    timerThreadData->data = (void*)workerThreadData;
    timerThreadData->len = sizeof(WorkerThreadData) + len;
    
    commonData_->timerQueuePush(timerThreadData, connId);
}

void WorkerCommonData::updateResendInterval(uint64_t connId, unsigned int rtt) {
    int value = DEFAULT_RESEND_INTERVAL;
    if (rtt * 3 < 100) {
        value = 100;
    } else if ((int)rtt * 3 > DEFAULT_RESEND_INTERVAL) {
        value = rtt * 3;
    }
    PacketResendIntervalMap_[connId] = value;
}

unsigned int WorkerCommonData::getResendInterval(uint64_t connId) {
    std::unordered_map<uint64_t, unsigned int>::iterator it = PacketResendIntervalMap_.find(connId);
    if (it != PacketResendIntervalMap_.end()) {
        return it->second;
    }

    return DEFAULT_RESEND_INTERVAL;
}

void WorkerCommonData::deleteFromResendInterval(uint64_t connId) {
    std::unordered_map<uint64_t, unsigned int>::iterator it = PacketResendIntervalMap_.find(connId);
    if (it != PacketResendIntervalMap_.end()) {
        PacketResendIntervalMap_.erase(it);
    }
}

int WorkerCommonData::insertBatchAckMap(uint64_t connId, uint64_t packetId) {
    std::unordered_map<uint64_t, std::vector<uint64_t>>::iterator it = batchAckMap_.find(connId);
    if (it != batchAckMap_.end()) {
        it->second.push_back(packetId);
        return 0;
    } else {
        std::vector<uint64_t> tmpVec;
        tmpVec.push_back(packetId);
        batchAckMap_[connId] = tmpVec;
        return 1;
    }
}

bool WorkerCommonData::getBatchAckVec(uint64_t connId, std::vector<uint64_t>& ackVec) {
    std::unordered_map<uint64_t, std::vector<uint64_t>>::iterator it = batchAckMap_.find(connId);
    if (it != batchAckMap_.end()) {
        ackVec = it->second;
        batchAckMap_.erase(it);
        return true;
    } else {
        return false;
    }
}

void WorkerCommonData::deleteBatchAckMap(uint64_t connId) {
    std::unordered_map<uint64_t, std::vector<uint64_t>>::iterator it = batchAckMap_.find(connId);
    if (it != batchAckMap_.end()) {
        batchAckMap_.erase(it);
    }
}




