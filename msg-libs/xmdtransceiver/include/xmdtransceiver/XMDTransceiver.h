#ifndef RTSTREAM_H
#define RTSTREAM_H

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
    int port_;
    
public:
    XMDTransceiver(int decodeThreadSize, int port = 0) {
        port_ = port;
        commonData_ = new XMDCommonData(decodeThreadSize);
        packetDispatcher_ = new PacketDispatcher();
        recvThread_ = new XMDRecvThread(port, commonData_);
        sendThread_ = new XMDSendThread(recvThread_->listenfd(), port, commonData_, packetDispatcher_);
        packetbuildThreadPool_ = new XMDPacketBuildThreadPool(1, commonData_);
        packetRecoverThreadPool_ = new XMDPacketRecoverThreadPool(decodeThreadSize, commonData_);
        packetDecodeThreadPool_ = new XMDPacketDecodeThreadPool(1, commonData_, packetDispatcher_);
        callbackThread_ = new XMDCallbackThread(packetDispatcher_, commonData_);
        pingThread_ = new PingThread(packetDispatcher_, commonData_);
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
        if (packetbuildThreadPool_) {
            delete packetbuildThreadPool_;
            packetbuildThreadPool_ = NULL;
        }
        if (packetDecodeThreadPool_) {
            delete packetDecodeThreadPool_;
            packetDecodeThreadPool_ = NULL;
        }
    }
    
    int sendDatagram(char* ip, int port, char* data, int len, uint64_t delay_ms);

    uint64_t createConnection(char* ip, int port, char* data, int len, int timeout, void* ctx, bool isEncrypt = true);
    int closeConnection(uint64_t conn_id);

    uint16_t createStream(uint64_t conn_id, StreamType streamType, int timeout);
    int closeStream(uint64_t conn_id, uint16_t stream_id);

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
    
    int sendRTData(uint64_t conn_id, uint16_t stream_id, char* data, int len, void* ctx = NULL);

    int updatePeerInfo(uint64_t conn_id, char* ip, int port);

    int getPeerInfo(uint64_t conn_id, std::string &ip, int& port);

    int getLocalInfo(std::string &ip, int& port);
    
    void run();
    void join();
    void stop();

    /*void logger(log4cplus::Logger logger) {
        LoggerWrapper::instance()->logger(logger);
    }*/

    void setExternalLog(ExternalLog* externalLog) {
        LoggerWrapper::instance()->externalLog(externalLog);
    }

    bool setTestNetFlag (bool flag) {
        sendThread_->setTestFlat(flag);
        recvThread_->setTestFlat(flag);
    }
};

#endif //RTSTREAM_H
