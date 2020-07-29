#ifndef RTSTREAM_H
#define RTSTREAM_H

#include "XMDSendThread.h"
#include "XMDRecvThread.h"
#include "XMDCommonData.h"
#include "PacketDispatcher.h"
#include "XMDWorkerThreadPool.h"
#include "XMDTimerThreadPool.h"
#include "XMDPacket.h"
#include "ExternalLog.h"

const unsigned int MAX_PACKET_LEN = 512 * 1024;


class XMDTransceiver {
private:
    XMDCommonData* commonData_;
    XMDSendThread* sendThread_;
    XMDRecvThread* recvThread_;
    PacketDispatcher* packetDispatcher_;
    XMDWorkerThreadPool* workerThreadPool_;
    XMDTimerThreadPool* timerThreadPool_;
    int port_;
    static pthread_mutex_t reset_socket_mutex_;
    int workerThreadSize_;
    int timerThreadSize_;
    int bufferMaxSize_;
    uint16_t local_port_;
    uint32_t local_ip_;
    std::string local_ip_str_;
    
public:
    XMDTransceiver(unsigned int wokerThreadSize, uint16_t port = 0) {
        port_ = port;
        workerThreadSize_ = wokerThreadSize;
        timerThreadSize_ = 1;
        local_ip_ = 0;
        local_port_ = 0;
    }
    ~XMDTransceiver() {
        if (packetDispatcher_) {
            delete packetDispatcher_;
            packetDispatcher_ = NULL;
        }
        if (recvThread_) {
            delete recvThread_;
            recvThread_ = NULL;
        }
        if (sendThread_) {
            delete sendThread_;
            sendThread_ = NULL;
        }
        if (workerThreadPool_) {
            delete workerThreadPool_;
            workerThreadPool_ = NULL;
        }
        if (timerThreadPool_) {
            delete timerThreadPool_;
            timerThreadPool_ = NULL;
        }
        if (commonData_) {
            delete commonData_;
            commonData_ = NULL;
        }
    }

    int start();

    int resetSocket();
    
    int sendDatagram(char* ip, uint16_t port, char* data, unsigned int len, uint64_t delay_ms);
    int buildAndSendDatagram(char* ip, uint16_t port, char* data, unsigned int len, uint64_t delay_ms, unsigned char packetType);
    int sendTestRttPacket(uint64_t connId, unsigned int delayMs, unsigned int packetCount);

    uint64_t createConnection(char* ip, uint16_t port, char* data, unsigned int len, uint16_t timeout, void* ctx);
    int closeConnection(uint64_t connId);

    uint16_t createStream(uint64_t connId, StreamType streamType, uint16_t waitTime, bool isEncrypt);
    int closeStream(uint64_t connId, uint16_t streamId);

    void registerRecvDatagramHandler(DatagramRecvHandler* handler) { 
        packetDispatcher_->registerRecvDatagramHandler(handler); 
    }
    void registerConnHandler(ConnectionHandler* handler) {
        packetDispatcher_->registerConnHanlder(handler);
    }
    void registerStreamHandler(StreamHandler* handler) {
        packetDispatcher_->registerStreamhandler(handler);
    }
    void registerNetStatusChangeHandler(NetStatusChangeHandler* handler) {
        packetDispatcher_->registerNetStatusChangeHandler(handler);
    }
    void registerSocketErrHandler(XMDSocketErrHandler* handler) {
        packetDispatcher_->registerXMDSocketErrHandler(handler);
    }

    int sendRTData(uint64_t connId, uint16_t streamId, char* data, unsigned int len, void* ctx = NULL);
    
    int sendRTData(uint64_t connId, uint16_t streamId, char* data, unsigned int len, bool canBeDropped, DataPriority priority, int resendCount, void* ctx = NULL);

    int updatePeerInfo(uint64_t connId, char* ip, uint16_t port);

    int getPeerInfo(uint64_t connId, std::string &ip, int32_t& port);

    int getLocalInfo(std::string &ip, uint16_t& port);
    int getLocalInfo(uint32_t &ip, uint16_t& port);
    
    void run();
    void join();
    void stop();

    void static setXMDLogLevel(XMDLogLevel level) {
        XMDLoggerWrapper::instance()->setXMDLogLevel(level);
    }

    void static setExternalLog(ExternalLog* externalLog) {
        XMDLoggerWrapper::instance()->externalLog(externalLog);
    }

    void setTestPacketLoss (unsigned int value) {
        sendThread_->setTestPacketLoss(value);
        recvThread_->setTestPacketLoss(value);
    }

    void setSendBufferSize(int size);
    int getSendBufferUsedSize();
    int getSendBufferMaxSize();
    float getSendBufferUsageRate();
    void clearSendBuffer();
    void setRecvBufferSize(int size) { }
    int getRecvBufferUsedSize() { return 0; }
    int getRecvBufferMaxSize() { return 0; }
    float getRecvBufferUsageRate() { return 0; }
    void clearRecvBuffer() { }
    int getTimerQueueUsedSize(int id) { return commonData_->getTimerQueueUsedSize(id); }
    int getSocketSendQueueUsedSize() { return commonData_->getSocketSendQueueUsedSize(); }
    int getWorkerQueueUsedSize(int id) { return commonData_->getWorkerQueueUsedSize(id); }
    int getTimerQueueMaxSize(int id) { return commonData_->getTimerQueueMaxSize(id); }
    int getSocketSendQueueMaxSize() { return commonData_->getSocketSendQueueMaxSize(); }
    int getWorkerQueueMaxSize(int id) { return commonData_->getWorkerQueueMaxSize(id); }
    void setTimerQueueMaxSize(int id, int size) { commonData_->setTimerQueueMaxSize(id, size); }
    void setWorkerQueueMaxSize(int id, int size) { commonData_->setWorkerQueueMaxSize(id, size); }
    void setSocketSendQueueMaxSize(int size) { commonData_->setSocketSendQueueMaxSize(size); }
    float getTimerQueueUsageRate(int id) { return commonData_->getTimerQueueUsageRate(id); }
    float getWorkerQueueUsageRate(int id) { return commonData_->getWorkerQueueUsageRate(id); }
    float getSocketSendQueueUsageRate() { return commonData_->getSocketSendQueueUsageRate(); }

    ConnectionState getConnState(uint64_t connId);

    void SetPingTimeIntervalMs(uint64_t connId, uint16_t value) { commonData_->SetPingIntervalMs(connId, value); }

    void SetAckPacketResendIntervalMicroSecond(unsigned int value) {  }

    netStatus getNetStatus(uint64_t connId) {return commonData_->getNetStatus(connId); }
    
};

#endif //RTSTREAM_H
