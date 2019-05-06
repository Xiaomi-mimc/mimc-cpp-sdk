#ifndef PACKETDISPATCHER_H
#define PACKETDISPATCHER_H

#include "DatagramHandler.h"
#include "ConnectionHandler.h"
#include "DatagramHandler.h"
#include "StreamHandler.h"
#include "NetStatusChangeHandler.h"
#include <stdint.h>
#include <stdlib.h>


class PacketDispatcher {
private:
    DatagramRecvHandler* datagramRecvHandler_;
    ConnectionHandler* connectionHandler_;
    StreamHandler* streamHandler_;
    NetStatusChangeHandler* netStatusChangeHandler_;

public:
    PacketDispatcher() {
        datagramRecvHandler_ = NULL;
        connectionHandler_ = NULL;
        streamHandler_ = NULL;
        netStatusChangeHandler_ = NULL;
    }
    void registerRecvDatagramHandler(DatagramRecvHandler* handler) { datagramRecvHandler_ = handler; }
    void handleRecvDatagram(char * ip, int port, char * data, uint32_t len) {
        if (datagramRecvHandler_) {
            datagramRecvHandler_->handle(ip, port, data, len);
        }
    }
    void registerConnHanlder(ConnectionHandler* handler) { connectionHandler_ = handler; }
    void handleConnCreateSucc(uint64_t connId, void* ctx) {
        if (connectionHandler_) {
            connectionHandler_->ConnCreateSucc(connId, ctx);
        }
    }
    void handleCreateConnFail(uint64_t connId, void* ctx) {
        if (connectionHandler_) {
            connectionHandler_->ConnCreateFail(connId, ctx);
        }
    }
    void handleNewConn(uint64_t connId, char* data, int len) {
        if (connectionHandler_) {
            connectionHandler_->NewConnection(connId, data, len);
        }
    }
    void handleCloseConn(uint64_t connId, ConnCloseType type) {
        if (connectionHandler_) {
            connectionHandler_->CloseConnection(connId, type);
        }
    }
    void handleConnIpChange(uint64_t connId, std::string ip, int port) {
        if (connectionHandler_) {
            connectionHandler_->ConnIpChange(connId, ip, port);
        }
    }
    void registerStreamhandler(StreamHandler* handler) { streamHandler_ = handler; }
    void handleNewStream(uint64_t connId, uint16_t streamId) {
        if (streamHandler_) {
            streamHandler_->NewStream(connId, streamId);
        }
    }
    void handleCloseStream(uint64_t connId, uint16_t streamId) {
        if (streamHandler_) {
            streamHandler_->CloseStream(connId, streamId);
        }
    }
    void handleStreamData(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, char* data, int len) {
        if (streamHandler_) {
            streamHandler_->RecvStreamData(conn_id, stream_id, groupId, data, len);
        }
    }
    void streamDataSendSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        if (streamHandler_) {
            streamHandler_->sendStreamDataSucc(conn_id, stream_id, groupId, ctx);
        }
    }
    void streamDataSendFail(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        if (streamHandler_) {
            streamHandler_->sendStreamDataFail(conn_id, stream_id, groupId, ctx);
        }
    }
    void FECStreamDataSendComplete(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        if (streamHandler_) {
            streamHandler_->sendFECStreamDataComplete(conn_id, stream_id, groupId, ctx);
        }
    }
    void registerNetStatusChangeHandler(NetStatusChangeHandler* handler) { netStatusChangeHandler_ = handler; }
    void handleNetStatusChange(uint64_t conn_id, short delay_ms, float packet_loss) {
        netStatusChangeHandler_->handle(conn_id, delay_ms, packet_loss);
    }
};



#endif //PACKETDISPATCHER_H