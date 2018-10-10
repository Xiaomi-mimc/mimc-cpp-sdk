#ifndef PACKETRECOVER_H
#define PACKETRECOVER_H

#include "XMDPacket.h"
#include "PacketBuilder.h"
#include "PacketDispatcher.h"
#include "XMDCommonData.h"
#include <map>
#include <vector>
#include "fec.h"


class PacketDecoder {
private:
    PacketDispatcher* dispatcher_;
    XMDCommonData* commonData_;
public:
    PacketDecoder(PacketDispatcher* dispatcher, XMDCommonData* commonData);
    ~PacketDecoder();
    void decode(uint32_t ip, int port, char* data, int len);
    void handleNewConn(uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt);
    void handleConnResp(uint32_t ip, int port, unsigned char* data, int len);
    void handleConnClose(unsigned char* data, int len);
    void handleFECStreamData(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len);
    void handleCloseStream(unsigned char* data, int len);
    void handleConnReset(unsigned char* data, int len);
    void handleStreamDataAck(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len);
    void handlePing(uint64_t connId, ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len);
    void handlePong(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len);
    void handleAckStreamData(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len);
    void sendConnReset(uint32_t ip, int port, uint64_t conn_id, ConnResetType type);
};

#endif //PACKETRECOVER_H
