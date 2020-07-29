#ifndef UDPSENDQUEUE_H
#define UDPSENDQUEUE_H

#include <queue>
#include <pthread.h>
#include <stdint.h>
#include <vector>
#include <functional>
#include <string.h>
#include <map>
#include <string>
#include <unordered_map>
#include "queue.h"
#include "map.h"
#include "block_queue.h"
#include "common.h"
#include <set>



const int SESSION_KEY_LEN = 128;
const int FIRST_RESEND_INTERVAL = 200;
const int SECOND_RESEND_INTERVAL = 500;
const int THIRD_RESEND_INTERVAL = 1000;
const int DEFAULT_RESEND_INTERVAL = 200;
const int MAX_RESEND_INTERVAL = 2000;
const int PING_INTERVAL = 1000; //1s
const int CALUTE_PACKET_LOSS_DELAY = 200; //ms
const int FLOW_CONTROL_MAX_PACKET_SIZE = 10 * 1024;
const int FLOW_CONTROL_SEND_SPEED = 10;   // /ms
const int MAX_SEND_TIME = 3;
const int DEFAULT_RESEND_QUEUE_SIZE = 1024 * 1024;  
const float QUEUE_USAGERAGE_80 = 0.8;
const float QUEUE_USAGERAGE_90 = 0.9;
const float QUEUE_USAGERAGE_FULL = 0.999999;
const int FEC_GROUP_DELETE_INTERVAL = 3000;  //ms
const int ACK_GROUP_DELETE_INTERVAL = 10000;  //ms
const int CHECK_CALLBACK_BUFFER_INTERVAL = 5;
const int SEND_PONG_WAIT_TIME = 100; //ms
const int CREATE_CONN_RESEND_TIME = 10;
const float RTT_SMOOTH_FACTOR = 0.125;
const int NETSTATUS_MIN_PACKETS = 10;
const int MAX_CALLBACK_DATA_MAP_SIZE = 100;
const int SOCKET_SENDQUEUE_MAX_SIZE = 1024 * 1024;
const int WORKER_QUEUE_MAX_SIZE = 1024 * 1024;
const int DELAY_BATCH_ACK_MS = 30;
const int BATCH_ACK_MAX_NUM = 150;
const int DEFAULT_FLOW_CONTROL_SPEED = 5;



enum StreamType {
    FEC_STREAM = 7,
    ACK_STREAM = 11,
    BATCH_ACK_STREAM = 13,
};

enum ConnCloseType {
    CLOSE_NORMAL,
    CLOSE_TIMEOUT,
    CLOSE_CONN_NOT_EXIST,
    CLOSE_CONN_RESET,
    CLOSE_SEND_FAIL,
};

enum DataPriority {
    P0,
    P1,
    P2,
};

enum ConnectionState {
    CONNECTING,
    CONNECTED,
    CLOSING,
    CLOSED,
};

struct StreamInfo {
    int callbackWaitTimeout;
    bool isEncrypt;
    StreamType sType;
};

struct ConnInfo {
    uint64_t connId;
    uint32_t ip;
    int port;
    int timeout;
    uint32_t pingInterval;
    uint32_t max_stream_id;
    ConnectionState connState;
    std::string sessionKey;
    void* ctx;
    std::unordered_map<uint16_t, StreamInfo> streamMap;

    ConnInfo() {
    }
    ~ConnInfo() {
    }
};

struct netStatus {
    float packetLossRate;
    int rtt;
    uint32_t totalPackets;
    uint32_t tmpTotalPackets;
    uint32_t tmpRecvPackets;
    
    netStatus() {
        packetLossRate = 0;
        rtt = 0;
        totalPackets = 0;
        tmpTotalPackets = 0;
        tmpRecvPackets = 0;
    }
};

struct streamDataSendCallback {
    uint32_t groupSize;
    std::unordered_map<uint32_t, bool> sliceMap;
};

struct PacketCallbackInfo {
    uint64_t connId;
    uint64_t packetId;
    uint16_t streamId;
    uint32_t groupId;
    uint32_t sliceId;
    void* ctx;
};


enum PacketType {
    CONN_BEGIN = 0,
    CONN_RESP_SUPPORT = 1,
    CONN_RESP_NOT_SUPPORT = 2,
    CONN_CLOSE = 3,
    CONN_RESET = 4,
    STREAM_STREAT = 5,
    STREAM_END = 6,
    FEC_STREAM_DATA = 7,
    ACK = 8,
    PING = 9,
    PONG = 10,
    ACK_STREAM_DATA = 11,
    TEST_RTT_PACKET = 12,
    BATCH_ACK_STREAM_DATA = 13,
    BATCH_ACK = 14,
    P2P_PACKET = 20,
};

struct TestRttPacket {
    uint64_t connId;
    unsigned int delayMs;
    unsigned int packetCount;
};


struct SendQueueData {
    uint32_t ip;
    uint16_t port;
    int len;
    bool isResend;
    uint64_t connId;
    uint64_t packetId;
    int reSendCount;
    int reSendInterval;
    unsigned char* data;

    SendQueueData(uint32_t i, uint16_t p, unsigned char* d, int l) {
        ip = i;
        port = p;
        len = l;
        data = d;
        isResend = false;
    }
    SendQueueData(uint32_t i, uint16_t p, unsigned char* d, int l, uint64_t cId, uint64_t pId, int rCount, int rInterval) {
        ip = i;
        port = p;
        len = l;
        data = d;
        isResend = true;
        connId = cId;
        packetId = pId;
        reSendCount = rCount;
        reSendInterval = rInterval;
    }
    ~SendQueueData() {
        if (data) {
            ::operator delete((void*)data);
            data = NULL;
        }
    }
};



struct StreamQueueData {
    uint64_t connId;
    uint16_t streamId;
    uint32_t groupId;
    uint32_t len;
    bool canBeDropped;
    DataPriority dataPriority;
	int resendCount;
    void* ctx;
    unsigned char* data;

    StreamQueueData(int len) {
        data = new unsigned char[len];
    }
    StreamQueueData() {
        data = NULL;
    }
    ~StreamQueueData() {
        if (data != NULL) { delete[] data; }
    }
};

struct SocketData {
    uint32_t ip;
    uint16_t port;
    int len;
    unsigned char* data;

    SocketData(uint32_t i, uint16_t p, int l, unsigned char* d) {
        ip = i;
        port = p;
        len = l;
        data = new unsigned char[len];
        memcpy(data, d, len);
    }
    ~SocketData() {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
};

struct StreamData {
    StreamType type;
    int len;
    unsigned char* data;
    StreamData(StreamType t, int l, unsigned char* d) {
        type = t;
        len = l;
        data = new unsigned char[len];
        memcpy(data, d, len);
    }
    ~StreamData() {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
};

struct CallbackQueueData {
    uint64_t connId;
    uint16_t streamId;
    uint32_t groupId;
    StreamType type;
    int len;
    uint64_t recvTime;
    unsigned char* data;

    CallbackQueueData(uint64_t conn, uint16_t stream, uint32_t group, StreamType t, int l, uint64_t time, unsigned char* d) {
        connId = conn;
        streamId = stream;
        groupId = group;
        len = l;
        type = t;
        recvTime = time;
        data = d;
    }
    ~CallbackQueueData() {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
};

struct CallBackSortBuffer {
    int StreamWaitTime;
    std::map<uint32_t, CallbackQueueData*> groupMap;
};

struct PingPacket {
    uint64_t connId;
    uint64_t packetId;
    uint64_t sendTime;
};

struct PacketLossInfo {
    int oldPakcetCount;
    int newPacketCount;
    uint64_t maxPacketId;
    uint64_t minPacketId;
    uint64_t caculatePacketId;
    std::set<uint64_t> oldPakcetSet;
    std::set<uint64_t> newPakcetSet;

    PacketLossInfo() {
        oldPakcetCount = 0;
        newPacketCount = 0;
        maxPacketId = 0;
        minPacketId = 0;
        caculatePacketId = 0;
    }
};

struct PongThreadData {
    uint64_t connId;
    uint64_t packetId;
    uint64_t ts;
};

/*
struct ResendData {
    uint64_t connId;
    uint64_t packetId;
    uint32_t ip;
    int port;
    int reSendCount;
    int len;
    unsigned char* data;
    ResendData(unsigned char* d, int l) {
        data = new unsigned char[l];
        memcpy(data, d, l);
        len = l;
    }
    ResendData() {
        data = NULL;
    }
    ~ResendData() {
        if (data != NULL) { delete[] data; }
    }
};
*/


struct TimerThreadData {
    uint64_t time;
    uint64_t connId;
    void* data;
    int len;
};

struct TimerDataCmp {
    bool operator() (TimerThreadData* a, TimerThreadData* b) {
        return a->time > b->time;
    }
};


typedef std::priority_queue<TimerThreadData*, std::vector<TimerThreadData*>, TimerDataCmp> TimerThreadQueue;


enum WorkerThreadDataType {
    WORKER_CREATE_NEW_CONN,
    WORKER_CLOSE_CONN,
    WORKER_CREATE_STREAM,
    WORKER_CLOSE_STREAM,
    WORKER_SEND_RTDATA,
    WORKER_RECV_DATA,
    WORKER_RESEND_TIMEOUT,
    WORKER_DATAGREAM_TIMEOUT,
    WORKER_DELETE_ACKGROUP_TIMEOUT,
    WORKER_DELETE_FECGROUP_TIMEOUT,
    WORKER_CHECK_CALLBACK_BUFFER,
    WORKER_PING_TIMEOUT,
    WORKER_PONG_TIMEOUT,
    WORKER_CHECK_CONN_AVAILABLE,
    WORKER_TEST_RTT,
    WORKER_BATCH_ACK
};
struct WorkerThreadData {
    WorkerThreadDataType dataType;
    void* data;
    int len;

    WorkerThreadData(WorkerThreadDataType type, void* d, int l) {
        dataType = type;
        data = d;
        len = l;
        //XMDLoggerWrapper::instance()->error("new data len len %d.", len);
    }
};

struct NewConnWorkerData {
    ConnInfo connInfo;
    unsigned char* data;
    unsigned int len;
    NewConnWorkerData(unsigned char* d, unsigned l) {
        len = l;
        data = new unsigned char[len];
        memcpy(data, d, len);
    }
    ~NewConnWorkerData() {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
};

struct WorkerData {
    uint64_t connId;
    uint16_t streamId;
};

struct NewStreamWorkerData {
    uint64_t connId;
    uint16_t streamId;
    int callbackWaitTimeout;
    bool isEncrypt;
    StreamType sType;
};

struct RTWorkerData {
    uint64_t connId;
    uint16_t streamId;
    uint32_t groupId;
    bool canBeDropped;
    DataPriority dataPriority;
    int resendCount;
    void* ctx;
    unsigned char* data;
    unsigned int len;
    RTWorkerData(unsigned char* d, unsigned int l) {
        data = new unsigned char[l];
        memcpy(data, d, l);
        len = l;
    }
    ~RTWorkerData() {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
};

struct DeleteGroupTimeout {
    std::string key;
};

struct CheckCallbackBufferData{
    uint64_t connId;
    std::string key;
};

class XMDCommonData {
private:
    std::vector<TimerThreadQueue>  timerQueueVec_;
    STLBlockQueue<SendQueueData*> socketSendQueue_;
    std::vector<STLBlockQueue<WorkerThreadData*>> workerQueueVec_;

    std::unordered_map<std::string, uint32_t> groupIdMap_;
    std::unordered_map<uint64_t, uint16_t> streamIdMap_;
    std::unordered_map<uint64_t, uint16_t> pingIntervalMap_;

    STLSafeHashMap<uint64_t, netStatus> netStatusMap_;

    pthread_mutex_t group_id_mutex_;
    pthread_mutex_t stream_id_mutex_;
    //pthread_mutex_t socket_send_queue_mutex_;
    pthread_mutex_t ping_interval_mutex_;
    std::vector<pthread_mutex_t> timer_queue_mutex_vec_;
    std::vector<pthread_mutex_t> worker_queue_mutex_vec_;
    pthread_mutex_t socket_send_queue_mutex_;
    
    unsigned int workerThreadSize_;
    unsigned int timerThreadSize_;
    std::vector<unsigned int> timerQueueMaxSizeVec_;
    std::vector<unsigned int> timerQueueSizeVec_;
    std::vector<unsigned int> workerQueueSizeVec_;
    std::vector<unsigned int> workerQueueMaxSizeVec_;
    unsigned int socketSendQueueSize_;
    unsigned int socketSendQueueMaxSize_;
    
public:
    XMDCommonData(unsigned int workerThreadSize, unsigned int timerThreadSize);
    ~XMDCommonData();
    void workerQueuePush(WorkerThreadData* data, uint64_t connid);
    WorkerThreadData* workerQueuePop(unsigned int id);
    bool workerQueueEmpty(unsigned int id);
    bool isWorkerQueueFull(unsigned int id, unsigned int len);
    void setWorkerQueueMaxSize(int id, int size);
    int getWorkerQueueUsedSize(int id);
    int getWorkerQueueMaxSize(int id);
    float getWorkerQueueUsageRate(int id);
    void clearWorkerQueue(int id);

    void socketSendQueuePush(SendQueueData* data);
    SendQueueData* socketSendQueuePop();
    bool socketSendQueueEmpty() { return socketSendQueue_.empty(); }
    bool isSocketSendQueueFull(unsigned int len);
    void setSocketSendQueueMaxSize(int size);
    int getSocketSendQueueMaxSize();
    int getSocketSendQueueUsedSize();
    float getSocketSendQueueUsageRate();
    void clearSocketQueue();

    bool timerQueuePush(TimerThreadData* data , uint64_t connid);
    TimerThreadData* timerQueuePop(unsigned int id);
    TimerThreadData* timerQueuePriorityPop(unsigned int id);
    bool timerQueueEmpty(unsigned int id); 
    size_t timerQueueSize(uint64_t connId);
    void setTimerQueueMaxSize(int queueId, int size);
    float getTimerQueueUsageRate(int queueId);
    bool isTimerQueueFull(int queueId, unsigned int len);
    int getTimerQueueUsedSize(int queueId);
    int getTimerQueueMaxSize(int queueId);
    void clearTimerQueue(int queueId);

    uint32_t getGroupId(uint64_t connId, uint16_t streamId);
    void deleteGroupId(std::string id);
    uint32_t getConnStreamId(uint64_t connId);
    void deleteConnStreamId(uint64_t connId);
    void addConnStreamId(uint64_t connId, uint16_t streamId);

    void NotifyBlockQueue();
    bool SendQueueHasNewData();

    void SetPingIntervalMs(uint64_t connId, uint16_t value);
    uint16_t GetPingIntervalMs(uint64_t connId);
    void DeletePingIntervalMap(uint64_t connId);

    netStatus getNetStatus(uint64_t connId);
    void updateNetStatus(uint64_t connId, netStatus status);
    void deleteNetStatus(uint64_t connId);
    void deleteWorkerData(WorkerThreadData* workerThreadData);
};



class WorkerCommonData {
private:
    std::unordered_map<uint64_t, ConnInfo> connectionMap_;
    std::unordered_map<uint64_t, uint64_t> packetIdMap_;
    std::unordered_map<std::string, CallBackSortBuffer> callbackDataMap_;
    std::unordered_map<std::string, streamDataSendCallback> streamDataSendCbMap_;
    std::unordered_map<uint64_t, PacketLossInfo> PacketLossInfoMap_;
    std::unordered_map<uint64_t, unsigned int> PacketResendIntervalMap_;
    STLHashMap<uint64_t, netStatus> netStatusMap_;
    STLHashMap<std::string, uint64_t> lastPacketTimeMap_;
    STLHashMap<std::string, uint32_t> lastCallbackGroupIdMap_;
    STLHashMap<std::string, bool> isPacketRecvAckMap_;
    STLHashMap<uint64_t, PingPacket>  pingMap_;
    STLHashMap<std::string, PacketCallbackInfo> packetCallbackInfoMap_;
    std::unordered_map<uint64_t, std::vector<uint64_t>> batchAckMap_;

    XMDCommonData* commonData_;
public:
    WorkerCommonData(XMDCommonData* commonData) {
        commonData_ = commonData;
    }
    ~WorkerCommonData();
    int insertConn(uint64_t connId, ConnInfo connInfo);
    int updateConn(uint64_t connId, ConnInfo connInfo);
    int deleteConn(uint64_t connId);
    int deleteConnResource(uint64_t connId, uint32_t maxStreamId);
    bool isConnExist(uint64_t connId);
    bool getConnInfo(uint64_t connId, ConnInfo& cInfo);
    void closeAllConn();
    
    int updateConnIpInfo(uint64_t connId, uint32_t ip, int port);

    bool isStreamExist(uint64_t connId, uint16_t streamId);
    int insertStream(uint64_t connId, uint16_t streamId, StreamInfo streamInfo);
    int deleteStream(uint64_t connId, uint16_t streamId);
    bool getStreamInfo(uint64_t connId, uint16_t streamId, StreamInfo& sInfo);

    uint64_t getPakcetId(uint64_t connId);
    void deletePacketIdMap(uint64_t connId);

    void updateIsPacketRecvAckMap(std::string key, bool value);
    bool getIsPacketRecvAckMapValue(std::string key);
	void insertIsPacketRecvAckMap(std::string key, bool value);
	bool deleteIsPacketRecvAckMap(std::string key);

    uint64_t getLastPacketTime(std::string id);
    void updateLastPacketTime(std::string id, uint64_t value);
    void deleteLastPacketTime(std::string id);

    netStatus getNetStatus(uint64_t connId);
    void updateNetStatus(uint64_t connId, netStatus status);
    void deleteNetStatus(uint64_t connId);

    bool getLastCallbackGroupId(std::string id, uint32_t& groupId);
    void updateLastCallbackGroupId(std::string id, uint32_t groupId);
    void deleteLastCallbackGroupId(std::string id);

    int insertCallbackDataMap(std::string key, uint32_t groupId, int waitTime, CallbackQueueData* data);
    int getCallbackData(std::string key, uint32_t groupId, CallbackQueueData* &data);
    void deletefromCallbackDataMap(std::string key);
    bool isCallbackBufferMapEmpty(std::string key);

    void insertSendCallbackMap(std::string key, uint32_t groupSize);
    int updateSendCallbackMap(std::string key, uint32_t sliceId);
    void deleteSendCallbackMap(std::string key);
    bool SendCallbackMapRecordExist(std::string key);
    bool getDeleteCallbackMapRecord(std::string key);

    void insertPacketCallbackInfoMap(std::string key, PacketCallbackInfo info);
    bool getDeletePacketCallbackInfo(std::string key, PacketCallbackInfo& info);
    bool getPacketCallbackInfo(std::string key, PacketCallbackInfo& info);
    bool deletePacketCallbackInfo(std::string key);

    void insertPacketLossInfoMap(uint64_t connId);
    void updatePacketLossInfoMap(uint64_t connId, uint64_t packetId);
    void deletePacketLossInfoMap(uint64_t connId);
    bool getPacketLossInfo(uint64_t connId, PacketLossInfo& data);
    void startPacketLossCalucate(uint64_t connId);

    int insertPing(PingPacket packet);
    bool getPingPacket(uint64_t connId, PingPacket& packet);

    void startTimer(uint64_t connId, WorkerThreadDataType type, uint64_t time, void* data, int len);

    void updateResendInterval(uint64_t connId, unsigned int rtt);
    unsigned int getResendInterval(uint64_t connId);
    void deleteFromResendInterval(uint64_t connId);

    int insertBatchAckMap(uint64_t connId, uint64_t packetId);
    bool getBatchAckVec(uint64_t connId, std::vector<uint64_t>& ackVec);
    void deleteBatchAckMap(uint64_t connId);

};


#endif //UDPSENDQUEUE_H
