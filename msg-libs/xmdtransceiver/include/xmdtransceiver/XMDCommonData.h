#ifndef UDPSENDQUEUE_H
#define UDPSENDQUEUE_H

#include <queue>
#include <pthread.h>
#include <stdint.h>
#include <vector>
#include <functional>
#include <string.h>
#include <map>
#include <unordered_map>
#include <openssl/rsa.h>



const int SESSION_KEY_LEN = 128;
const int MAX_RESEND_TIME = 3;
const int FIRST_RESEND_INTERVAL = 100;
const int SECOND_RESEND_INTERVAL = 500;
const int THIRD_RESEND_INTERVAL = 1000;
const int CALCULATE_PACKET_LOSS_PACKET_NUM = 100;
const int CALCULATE_PACKET_LOSS_INTERVAL = 1000;  //1s
const int CHECKOUT_CONN_TIMEOUT_INTERVAL = 1000;  //1s
const int PING_INTERVAL = 100; //100ms
const int PING_TIMEOUT = 500; //500ms
const int PING_PACKET_SIZE = CALCULATE_PACKET_LOSS_PACKET_NUM + (PING_TIMEOUT / PING_INTERVAL) + 1;
const int FLOW_CONTROL_MAX_PACKET_SIZE = 2 * 1024;
const int FLOW_CONTROL_SEND_SPEED = 2;   // 2/ms
const int RESEND_DATA_INTEVAL = 100;
const int MAX_SEND_TIME = 3;


enum StreamType {
    FEC_STREAM = 7,
    ACK_STREAM = 11,
};

enum ConnCloseType {
    CLOSE_NORMAL,
    CLOSE_TIMEOUT,
    CLOSE_CONN_NOT_EXIST,
    CLOSE_CONN_RESET,
    CLOSE_SEND_FAIL,
};

struct StreamInfo {
    int timeout;
    StreamType sType;
};

struct ConnInfo {
    uint32_t ip;
    int port;
    int timeout;
    uint32_t created_stream_id;
    uint32_t max_stream_id;
    bool isConnected;
    bool isEncrypt;
    RSA* rsa;
    std::string sessionKey;
    void* ctx;
    std::unordered_map<uint16_t, StreamInfo> streamMap;
};

struct netStatus {
    double packetLossRate;
    int ttl;
    netStatus() {
        packetLossRate = 0;
        ttl = 0;
    }
};

struct streamDataSendCallback {
    uint32_t groupSize;
    std::unordered_map<uint32_t, bool> sliceMap;
};

struct ackPacketInfo {
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
};


struct SendQueueData {
    uint32_t ip;
    uint16_t port;
    int len;
    unsigned char* data;
    uint64_t sendTime;

    SendQueueData(uint32_t i, uint16_t p, unsigned char* d, int l) {
        ip = i;
        port = p;
        len = l;
        data = d;
        sendTime = 0;
    }
    ~SendQueueData() {
        if (data) {
            ::operator delete((void*)data);
            data = NULL;
        }
    }
};

struct DatagramDataCmp {
    bool operator() (SendQueueData* a, SendQueueData* b) {
        return a->sendTime > b->sendTime;
    }
};


struct StreamQueueData {
    uint64_t connId;
    uint16_t streamId;
    uint32_t groupId;
    uint32_t len;
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
    unsigned char* data;

    CallbackQueueData(uint64_t conn, uint16_t stream, uint32_t group, StreamType t, int l, unsigned char* d) {
        connId = conn;
        streamId = stream;
        groupId = group;
        len = l;
        type = t;
        data = d;
    }
    ~CallbackQueueData() {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
};

struct PingPacket {
    uint64_t connId;
    uint64_t packetId;
    uint64_t sendTime;
    int ttl;
};


struct PingPackets {
    std::vector<PingPacket> pingVec;
    int pingIndex;
};


struct ResendData {
    uint64_t connId;
    uint64_t packetId;
    uint32_t ip;
    int port;
    uint64_t lastSendTime;
    uint64_t reSendTime;
    int sendCount;
    int len;
    unsigned char* data;
    ResendData(unsigned char* d, int l) {
        data = new unsigned char[l];
        memcpy(data, d, l);
        len = l;
    }
    ~ResendData() {
        if (data != NULL) { delete[] data; }
    }
};


struct ResendDataCmp {
    bool operator() (ResendData* a, ResendData* b) {
        return a->reSendTime > b->reSendTime;
    }
};


typedef std::priority_queue<SendQueueData*, std::vector<SendQueueData*>, DatagramDataCmp> DatagramQueue;

typedef std::priority_queue<ResendData*, std::vector<ResendData*>, ResendDataCmp> ResendQueue;


class XMDCommonData {
private:
    DatagramQueue datagramQueue_;
    ResendQueue resendQueue_;
    std::queue<StreamQueueData*> streamQueue_;
    std::queue<SendQueueData*> socketSendQueue_;
    std::queue<SocketData*> socketRecvQueue_;
    std::vector<std::queue<StreamData*>> packetRecoverQueueVec_;
    std::queue<CallbackQueueData*> callbackQueue_;
    std::unordered_map<std::string, bool> packetResendMap_;
    std::map<uint64_t, PingPackets>  pingMap_;
    std::unordered_map<uint64_t, ConnInfo> connectionMap_;
    std::vector<uint64_t> connVec_;
    std::unordered_map<uint64_t, uint64_t> packetIdMap_;
    std::unordered_map<uint64_t, netStatus> netStatusMap_;
    std::unordered_map<std::string, uint64_t> lastPacketTimeMap_;
    std::unordered_map<std::string, uint32_t> groupIdMap_;
    std::unordered_map<std::string, uint32_t> lastRecvedGroupIdMap_;
    std::unordered_map<std::string, uint32_t> lastCallbackGroupIdMap_;
    std::unordered_map<std::string, std::map<uint32_t, CallbackQueueData*>> callbackDataMap_;
    std::unordered_map<std::string, streamDataSendCallback> streamDataSendCbMap_;
    std::unordered_map<std::string, ackPacketInfo> ackPacketMap_;
    
    static pthread_mutex_t stream_queue_mutex_;
    static pthread_mutex_t socket_send_queue_mutex_;
    static pthread_mutex_t socket_recv_queue_mutex_;
    static pthread_mutex_t resend_queue_mutex_;
    static pthread_mutex_t datagram_queue_mutex_;
    pthread_rwlock_t conn_mutex_;
    static pthread_mutex_t callback_queue_mutex_;
    static pthread_mutex_t ping_mutex_;
    static pthread_mutex_t packetId_mutex_;
    static pthread_mutex_t resend_map_mutex_;
    std::vector<pthread_mutex_t> packetRecoverQueueMutexVec_;
    pthread_rwlock_t last_packet_time_mutex_;
    pthread_rwlock_t netstatus_mutex_;
    pthread_rwlock_t group_id_mutex_;
    pthread_rwlock_t last_recv_group_id_mutex_;
    pthread_rwlock_t last_callback_group_id_mutex_;
    static pthread_mutex_t callback_data_map_mutex_;
    static pthread_mutex_t stream_data_send_callback_map_mutex_;
    static pthread_mutex_t ack_packet_map_mutex_;

    int decodeThreadSize_;
public:
    XMDCommonData(int decodeThreadSize);
    ~XMDCommonData();
    void streamQueuePush(StreamQueueData* data);
    StreamQueueData* streamQueuePop();

    void socketSendQueuePush(SendQueueData* data);
    SendQueueData* socketSendQueuePop();

    void socketRecvQueuePush(SocketData* data);
    SocketData* socketRecvQueuePop();

    void packetRecoverQueuePush(StreamData* data, int id);
    StreamData* packetRecoverQueuePop(int id);

    void callbackQueuePush(CallbackQueueData* data);
    CallbackQueueData* callbackQueuePop();
    

    void datagramQueuePush(SendQueueData* data);
    SendQueueData* datagramQueuePop();


    void resendQueuePush(ResendData* data);
    ResendData* resendQueuePop();
    bool resendQueueEmpty() { return resendQueue_.empty(); }
    void updateResendMap(std::string key, bool value);
    bool getResendMapValue(std::string key);

    int insertConn(uint64_t connId, ConnInfo connInfo);
    int updateConn(uint64_t connId, ConnInfo connInfo);
    int deleteConn(uint64_t connId);
    bool isConnExist(uint64_t connId);
    bool getConnInfo(uint64_t connId, ConnInfo& cInfo);
    uint32_t getConnStreamId(uint64_t connId);
    int updateConnIpInfo(uint64_t connId, uint32_t ip, int port);
    std::vector<uint64_t> getConnVec();

    bool isStreamExist(uint64_t connId, uint16_t streamId);
    int insertStream(uint64_t connId, uint16_t streamId, StreamInfo streamInfo);
    int deleteStream(uint64_t connId, uint16_t streamId);
    bool getStreamInfo(uint64_t connId, uint16_t streamId, StreamInfo& sInfo);
    uint32_t getGroupId(uint64_t connId, uint16_t streamId);
    void deleteGroupId(std::string id);

    int getDecodeThreadSize() { return decodeThreadSize_; }

    int startPing(uint64_t connId);
    int insertPing(PingPacket packet);
    int insertPong(uint64_t connId, uint64_t packetId, uint64_t currentTime, uint64_t ts);
    int calculatePacketLoss(uint64_t connId, double& packetLossRate, int& packetTtl);

    uint64_t getPakcetId(uint64_t connId);

    uint64_t getLastPacketTime(std::string id);
    void updateLastPacketTime(std::string id, uint64_t value);
    void deleteLastPacketTime(std::string id);

    netStatus getNetStatus(uint64_t connId);
    void updateNetStatus(uint64_t connId, netStatus status);
    void deleteNetStatus(uint64_t connId);

    bool getLastRecvGroupId(std::string id, uint32_t& groupId);
    void updateLastRecvGroupId(std::string id, uint32_t groupId);
    void deleteLastRecvGroupId(std::string id);

    uint32_t getLastCallbackGroupId(std::string id);
    void updateLastCallbackGroupId(std::string id, uint32_t groupId);
    void deleteLastCallbackGroupId(std::string id);

    void insertCallbackDataMap(std::string key, uint32_t groupId, CallbackQueueData* data);
    CallbackQueueData* getCallbackData(std::string key, uint32_t groupId);
    void deletefromCallbackDataMap(std::string key);

    
    void insertSendCallbackMap(std::string key, uint32_t groupSize);
    int updateSendCallbackMap(std::string key, uint32_t sliceId);
    void deleteSendCallbackMap(std::string key);

    void insertAckPacketmap(std::string key, ackPacketInfo info);
    bool getDeleteAckPacketInfo(std::string key, ackPacketInfo& info);
    
};


#endif //UDPSENDQUEUE_H
