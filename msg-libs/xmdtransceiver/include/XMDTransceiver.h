#ifndef RTSTREAM_H
#define RTSTREAM_H

#ifdef _WIN32
#include <winsock2.h>
#include "pthread.h"
#pragma comment(lib, "Ws2_32.lib")
#endif // _WIN32

#include "XMDSendThread.h"
#include "XMDRecvThread.h"
#include "XMDCommonData.h"
#include "PacketDispatcher.h"
//#include "log4cplus/logger.h"
#include "XMDPacket.h"
#include "XMDPacketRecoverThreadPool.h"
#include "XMDCallbackThread.h"
#include "XMDPacketBuildThreadPool.h"
#include "XMDPacketDecodeThreadPool.h"
#include "PingThread.h"
#include "PongThread.h"
#include "ExternalLog.h"

const int MAX_PACKET_LEN = 512 * 1024;


class XMDTransceiver {
private:
    XMDCommonData* commonData_;
    XMDSendThread* sendThread_;
    XMDRecvThread* recvThread_;
    PacketDispatcher* packetDispatcher_;
    XMDPacketRecoverThreadPool* packetRecoverThreadPool_;
    XMDPacketBuildThreadPool* packetbuildThreadPool_;
    XMDPacketDecodeThreadPool* packetDecodeThreadPool_;
    XMDCallbackThread* callbackThread_;
    PingThread* pingThread_;
    PongThread* pongThread_;
    int port_;
    static pthread_mutex_t create_conn_mutex_;
    std::mutex reset_socket_mutex_;
    int decodeThreadSize_;
    
public:
    XMDTransceiver(int decodeThreadSize, int port = 0) {
        port_ = port;
        decodeThreadSize_ = decodeThreadSize;
    }

    ~XMDTransceiver() {
        if (commonData_) {
            delete commonData_;
            commonData_ = NULL;
        }
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
        if (packetRecoverThreadPool_) {
            delete packetRecoverThreadPool_;
            packetRecoverThreadPool_ = NULL;
        }
        if (callbackThread_) {
            delete callbackThread_;
            callbackThread_ = NULL;
        }
        if (pingThread_) {
            delete pingThread_;
            pingThread_ = NULL;
        }
        if (pongThread_) {
            delete pongThread_;
            pongThread_ = NULL;
        }
        if (packetbuildThreadPool_) {
            delete packetbuildThreadPool_;
            packetbuildThreadPool_ = NULL;
        }
        if (packetDecodeThreadPool_) {
            delete packetDecodeThreadPool_;
            packetDecodeThreadPool_ = NULL;
        }
    }

    int start();

    int resetSocket();
    
    int sendDatagram(char* ip, uint16_t port, char* data, int len, uint64_t delay_ms);

    uint64_t createConnection(char* ip, uint16_t port, char* data, int len, uint16_t timeout, void* ctx);
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

    int sendRTData(uint64_t connId, uint16_t streamId, char* data, int len, void* ctx = NULL);
    
    int sendRTData(uint64_t connId, uint16_t streamId, char* data, int len, bool canBeDropped, DataPriority priority, int resendCount, void* ctx = NULL);

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

    void setTestPacketLoss (int value) {
        sendThread_->setTestPacketLoss(value);
        recvThread_->setTestPacketLoss(value);
    }

    void setSendBufferSize(int size);
    void setRecvBufferSize(int size);
    int getSendBufferSize();
    int getRecvBufferSize();
    float getSendBufferUsageRate();
    float getRecvBufferUsageRate();
    void clearSendBuffer();
    void clearRecvBuffer();

    ConnectionState getConnState(uint64_t connId);

    void SetPingTimeIntervalSecond(unsigned int value) { commonData_->SetPingTimeInterval(value); }

    void SetAckPacketResendIntervalMicroSecond(unsigned int value) { commonData_->SetResendTimeInterval(value); }
    
};

#endif //RTSTREAM_H
