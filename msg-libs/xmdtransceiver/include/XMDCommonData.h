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
#include <openssl/rsa.h>
#include "queue.h"
#include "map.h"


const int SESSION_KEY_LEN = 128;
const int FIRST_RESEND_INTERVAL = 200;
const int SECOND_RESEND_INTERVAL = 500;
const int THIRD_RESEND_INTERVAL = 1000;
const int PING_INTERVAL = 1000; //1s
const int CALUTE_PACKET_LOSS_DELAY = 200; //200ms
const int FLOW_CONTROL_MAX_PACKET_SIZE = 2 * 1024;
const int FLOW_CONTROL_SEND_SPEED = 2;   // 2/ms
const int MAX_SEND_TIME = 3;
const int DEFAULT_CALLBACK_QUEUE_LEN = 10000;
const int DEFAULT_DATAGRAM_QUEUE_LEN = 10000;
const int DEFAULT_RESEND_QUEUE_LEN = 10000;
const float QUEUE_USAGERAGE_80 = 0.8;
const float QUEUE_USAGERAGE_90 = 0.9;
const float QUEUE_USAGERAGE_FULL = 0.999999;



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
    int timeout;
    int callbackWaitTimeout;
    bool isEncrypt;
    StreamType sType;
};

struct ConnInfo {
    uint32_t ip;
    int port;
    int timeout;
    uint32_t created_stream_id;
    uint32_t max_stream_id;
    ConnectionState connState;
    RSA* rsa;
    std::string sessionKey;
    void* ctx;
    std::unordered_map<uint16_t, StreamInfo> streamMap;
};

struct netStatus {
    float packetLossRate;
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


struct ResendData {
    uint64_t connId;
    uint64_t packetId;
    uint32_t ip;
    int port;
    uint64_t reSendTime;
    int reSendCount;
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
    STLSafeQueue<StreamQueueData*> streamQueue_;
    STLSafeQueue<SendQueueData*> socketSendQueue_;
    STLSafeQueue<SocketData*> socketRecvQueue_;
    std::vector<STLSafeQueue<StreamData*>> packetRecoverQueueVec_;
    STLSafeQueue<CallbackQueueData*> callbackQueue_;
    STLSafeQueue<PongThreadData> pongThreadQueue_;
    
    std::unordered_map<uint64_t, ConnInfo> connectionMap_;
    std::vector<uint64_t> connVec_;
    std::unordered_map<uint64_t, uint64_t> packetIdMap_;
    std::unordered_map<std::string, uint32_t> groupIdMap_;
    std::unordered_map<std::string, CallBackSortBuffer> callbackDataMap_;
    std::unordered_map<std::string, streamDataSendCallback> streamDataSendCbMap_;
    std::unordered_map<uint64_t, PacketLossInfo> PacketLossInfoMap_;
    
    STLSafeHashMap<std::string, PacketCallbackInfo> packetCallbackInfoMap_;
    STLSafeHashMap<uint64_t, netStatus> netStatusMap_;
    STLSafeHashMap<std::string, uint64_t> lastPacketTimeMap_;
    STLSafeHashMap<std::string, uint32_t> lastCallbackGroupIdMap_;
    STLSafeHashMap<std::string, bool> isPacketRecvAckMap_;
    STLSafeHashMap<uint64_t, PingPacket>  pingMap_;
    
    static pthread_mutex_t resend_queue_mutex_;
    static pthread_mutex_t datagram_queue_mutex_;
    static pthread_mutex_t packetId_mutex_;
    static pthread_mutex_t callback_data_map_mutex_;
    static pthread_mutex_t stream_data_send_callback_map_mutex_;
    static pthread_mutex_t packet_loss_map_mutex_;
    pthread_rwlock_t conn_mutex_;
    pthread_rwlock_t group_id_mutex_;

    unsigned int decodeThreadSize_;
    unsigned int datagramQueueMaxLen_;
    unsigned int datagramQueueSize_;
    unsigned int callbackQueueSize_;
    unsigned int callbackQueueMaxLen_;
    unsigned int resendQueueSize_;
    unsigned int resendQueueMaxLen_;
    
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

    bool callbackQueuePush(CallbackQueueData* data);
    CallbackQueueData* callbackQueuePop();
    

    bool datagramQueuePush(SendQueueData* data);
    SendQueueData* datagramQueuePop();


    bool resendQueuePush(ResendData* data);
    ResendData* resendQueuePop();
    bool resendQueueEmpty() { return resendQueue_.empty(); }
    void updateIsPacketRecvAckMap(std::string key, bool value);
    bool getIsPacketRecvAckMapValue(std::string key);
	void insertIsPacketRecvAckMap(std::string key, bool value);
	bool deleteIsPacketRecvAckMap(std::string key);

    int insertConn(uint64_t connId, ConnInfo connInfo);
    int updateConn(uint64_t connId, ConnInfo connInfo);
    int deleteConn(uint64_t connId);
    bool isConnExist(uint64_t connId);
    bool getConnInfo(uint64_t connId, ConnInfo& cInfo);
    uint32_t getConnStreamId(uint64_t connId);
    int updateConnIpInfo(uint64_t connId, uint32_t ip, int port);
    std::vector<uint64_t> getConnVec();
    bool deleteFromConnVec(uint64_t connId);

    bool isStreamExist(uint64_t connId, uint16_t streamId);
    int insertStream(uint64_t connId, uint16_t streamId, StreamInfo streamInfo);
    int deleteStream(uint64_t connId, uint16_t streamId);
    bool getStreamInfo(uint64_t connId, uint16_t streamId, StreamInfo& sInfo);
    uint32_t getGroupId(uint64_t connId, uint16_t streamId);
    void deleteGroupId(std::string id);

    int getDecodeThreadSize() { return decodeThreadSize_; }

    int insertPing(PingPacket packet);
    bool getPingPacket(uint64_t connId, PingPacket& packet);

    uint64_t getPakcetId(uint64_t connId);

    uint64_t getLastPacketTime(std::string id);
    void updateLastPacketTime(std::string id, uint64_t value);
    void deleteLastPacketTime(std::string id);

    netStatus getNetStatus(uint64_t connId);
    void updateNetStatus(uint64_t connId, netStatus status);
    void deleteNetStatus(uint64_t connId);

    bool getLastCallbackGroupId(std::string id, uint32_t& groupId);
    void updateLastCallbackGroupId(std::string id, uint32_t groupId);
    void deleteLastCallbackGroupId(std::string id);

    void insertCallbackDataMap(std::string key, uint32_t groupId, int waitTime, CallbackQueueData* data);
    int getCallbackData(std::string key, uint32_t groupId, CallbackQueueData* &data);
    void deletefromCallbackDataMap(std::string key);

    
    void insertSendCallbackMap(std::string key, uint32_t groupSize);
    int updateSendCallbackMap(std::string key, uint32_t sliceId);
    void deleteSendCallbackMap(std::string key);
    bool SendCallbackMapRecordExist(std::string key);

    void insertPacketCallbackInfoMap(std::string key, PacketCallbackInfo info);
    bool getDeletePacketCallbackInfo(std::string key, PacketCallbackInfo& info);

    void setDatagramQueueSize(int size);
    float getDatagramQueueUsageRate();
    void clearDatagramQueue();

    void setResendQueueSize(int size);
    float getResendQueueUsageRate();
    void clearResendQueue();
    int getResendQueueSize();

    void setCallbackQueueSize(int size);
    float getCallbackQueueUsegeRate();
    void clearCallbackQueue();
    int getCallbackQueueSize();

    void insertPacketLossInfoMap(uint64_t connId);
    void updatePacketLossInfoMap(uint64_t connId, uint64_t packetId);
    void deletePacketLossInfoMap(uint64_t connId);
    bool getPacketLossInfo(uint64_t connId, PacketLossInfo& data);
    void startPacketLossCalucate(uint64_t connId);

    void pongThreadQueuePush(PongThreadData data);
    bool pongThreadQueuePop(PongThreadData& data);
};

int getResendTimeInterval(int sendcount);


#endif //UDPSENDQUEUE_H
