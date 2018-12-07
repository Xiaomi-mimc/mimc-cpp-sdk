#ifndef XMDPACKET_H
#define XMDPACKET_H

#include <stdlib.h>
#include <memory>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "XMDCommonData.h"
#include "common.h"
#include "zlib.h"


const static unsigned char XMDPACKET_MAGIC[2] = {0x0C, 0x2D};
const uint16_t XMD_VERSION = 1;
const int XMD_CRC_LEN = 4;
const int MAX_PACKET_SIZE = 1398;  //1398 与sdk保持一致
const int MAX_ORIGIN_PACKET_NUM_IN_PARTITION = 40; //40
const int STREAM_LEN_SIZE = 2;
const int CONN_LEN = 8;

enum XMDType {
    DATAGRAM = 0,
    STREAM = 1,
};


/*
 0 1 2 3 4 5 6 7 8 9 1 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|        magic number=16            |T|    PT=6   |E|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct __attribute__((__packed__)) XMDPacket {
    unsigned char magic[2];
    unsigned char sign;
    unsigned char data[0];

    inline void  SetMagic() {
        memcpy(magic, XMDPACKET_MAGIC, 2);
    }
    inline void SetSign(unsigned char type, unsigned char packetType, bool encrypt) {
        sign = 0;
        sign += (type << 7);
        sign += (packetType << 1);
        sign += encrypt;
    }
    inline void SetPayload(int len, unsigned char* d) {
        memcpy(data, d, len);
    }
    inline bool getEncryptSign() {
        return sign & 0X01;
    }
    inline unsigned char getType() {
        return ((sign & 0x80) >> 7);
    }
    inline unsigned char getPacketType() {
        return ((sign & 0x7E) >> 1);
    }
};

/*  PT=0, connection packet:
    0        1        2       3        4        5        6         …  12
+--------+--------+--------+--------+--------+--------+--------+---  ---+
|     Public Flags(24)     |    Version(16)  |  Connection ID (64)  ... | ->
+--------+--------+--------+--------+--------+--------+--------+---  ---+
    13        14       15      16        17       18        …   M
+--------+--------+--------+--------+--------+--------+----   ----+
|    timeout(16)  |    Key N Len    |   Key E len     |  Payload  | ->
+--------+--------+--------+--------+--------+--------+----   ----+
    M+1      M+2     M+3       M+4
+--------+--------+--------+--------+
|               CRC(32)             |
+--------+--------+--------+--------+
*/
struct __attribute__((__packed__)) XMDConnection {
    uint16_t version;
    uint64_t connId;
    uint16_t timeout;
    uint16_t nLen;
    uint16_t eLen;
    unsigned char data[0];

    inline void SetVersion(uint16_t ver) {
        version = htons(ver);
    }
    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    }
    inline void SetTimeout(uint16_t t) {
        timeout = htons(t);
    }
    inline void SetNLen(uint16_t len) {
        nLen = htons(len);
    }
    inline void SetELen(uint16_t len) {
        eLen = htons(len);
    }
    inline void SetPublicKey(int nlen, unsigned char* n, int elen, unsigned char* e) {
        SetNLen(nlen);
        SetELen(elen);
        memcpy(data, n, nlen);
        memcpy(data + nlen, e, elen);
    }
    inline void SetPayload(int len ,unsigned char* payload) {
        memcpy(data + GetNLen() + GetELen(), payload, len);
    }
    inline uint16_t GetVersion() {
        return ntohs(version);
    }
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
    inline uint16_t GetTimeout() {
        return ntohs(timeout);
    }
    inline uint16_t GetNLen() {
        return ntohs(nLen);
    }
    inline uint16_t GetELen() {
        return ntohs(eLen);
    }
    inline unsigned char* GetRSAN() {
        return data;
    }
    inline unsigned char* GetRSAE() {
        return data + GetNLen();
    }
    inline unsigned char* GetData() {
        return data + GetNLen() + GetELen();
    }
    
};

/*
PT=1 CONNECTION RESPONSE
0         1        2        3        4        5     …  10
+--------+--------+--------+--------+--------+--------+---  --+
|     Public Flags(24)     |       Connection ID (64)…        | ->
+--------+--------+--------+--------+--------+--------+---  --+
        11      12       13        14       15        16       17        18
+--------+--------+--------+--------+--------+--------+--------+--------+
|            Session Key            |               CRC(32)             |
+--------+--------+--------+--------+--------+--------+--------+--------+
*/

struct __attribute__((__packed__)) XMDConnResp {
    uint64_t connId;
    unsigned char sessionKey[0];
    
    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    }
    inline void SetSessionKey(unsigned char* key, int len) {
        memcpy(sessionKey, key, len);
    }
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
    inline unsigned char* GetSessionkey() {
        return sessionKey;
    }
};


/*
PT=3  CONNECTION CLOSE
0         1        2        3        4        5     …  10
+--------+--------+--------+--------+--------+--------+---  --+
|     Public Flags(24)     |       Connection ID (64)…        | ->
+--------+--------+--------+--------+--------+--------+---  --+
    11       12       13        14
+--------+--------+--------+--------+
|               CRC(32)             |
+--------+--------+--------+--------+
*/

struct __attribute__((__packed__)) XMDConnClose {
    uint64_t connId;
    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    }
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
};


/*
PT=4 CONNECTION RESET
0         1        2        3        4        5     …  10
+--------+--------+--------+--------+--------+--------+---  --+
|     Public Flags(24)     |       Connection ID (64)…        | ->
+--------+--------+--------+--------+--------+--------+---  --+
    11       12       13        14
+--------+--------+--------+--------+--------+
|ERR TYPE|               CRC(32)             |
+--------+--------+--------+--------+--------+
*/
enum ConnResetType {
    INVALID_TOKEN = 0,
    CONN_NOT_EXIST = 1,
    CONN_ID_CONFLICT =2,
};

struct __attribute__((__packed__)) XMDConnReset {
    uint64_t connId;
    char errType;

    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    }
    inline void SetErrType(ConnResetType type) {
        errType = (char)type;
    }
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
    inline ConnResetType GetErrType() {
        return (ConnResetType)errType;
    }
};

/*
PT=6 CLOSE STREAM
0         1        2        3        4        5     …  10
+--------+--------+--------+--------+--------+--------+---  --+
|     Public Flags(24)     |       Connection ID (64)…        | ->
+--------+--------+--------+--------+--------+--------+---  --+
    11       12       13        14       15      16
+--------+--------+--------+--------+--------+--------+
|  Stream ID(16)  |               CRC(32)             |
+--------+--------+--------+--------+--------+--------+
*/
struct __attribute__((__packed__)) XMDStreamClose {
    uint64_t connId;
    uint16_t streamId;
    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    }
    inline void SetStreamId(uint16_t id) {
        streamId = htons(id);
    }
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
    inline uint16_t GetStreamId() {
        return ntohs(streamId);
    }
};


/*
PT=7 FEC STREAM DATA
0         1        2        3        4        5     …  10
+--------+--------+--------+--------+--------+--------+---  --+
|     Public Flags(24)     |       Connection ID (64)…        | ->
+--------+--------+--------+--------+--------+--------+---  --+
     11       12       13        14        15       16       17      18
+--------+--------+--------+--------+--------+--------+--------+--------+
|                              Packet ID(64)                            | ->
+--------+--------+--------+--------+--------+--------+--------+--------+
     19       20       21        22      23       24       25        26
+--------+--------+--------+--------+--------+--------+--------+--------+
|  Stream ID(16)  |   timeout(16)   |            Group ID(32)           | ->
+--------+--------+--------+--------+--------+--------+--------+--------+
     27       28       29       30       31       32       33       34
+--------+--------+--------+--------+--------+--------+--------+--------+ 
|PSize(8)| PID(8) |  Slice ID(16)   |   FEC O P N(16) |    FEC P N(16)  | ->
+--------+--------+--------+--------+--------+--------+--------+--------+ 
   35         36  …   N    N+1      N+2      N+3      N+4
+--------+-------   ----+--------+--------+--------+--------+
| flags  |  Payload …   |               CRC(32)             |
+--------+-------   ----+--------+--------+--------+--------+
*/

struct __attribute__((__packed__)) XMDFECStreamData {
    uint64_t connId;
    uint64_t packetId;
    uint16_t streamId;
    uint16_t timeout;
    uint32_t groupId;
    uint8_t PSize;
    uint8_t PId;
    uint16_t sliceId;
    uint16_t FECOPN;
    uint16_t FECPN;
    uint8_t flags;
    unsigned char data[0];

    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    }
    inline void SetPacketId(uint64_t id) {
        packetId = xmd_htonll(id);
    }
    inline void SetStreamId(uint16_t id) {
        streamId = htons(id);
    }
    inline void SetGroupId(uint32_t id) {
        groupId = htonl(id);
    }
    inline void SetSliceId(uint16_t id) {
        sliceId = htons(id);
    }
    inline void SetFECOPN(uint16_t n) {
        FECOPN = htons(n);
    }
    inline void SetFECPN(uint16_t n) {
        FECPN = htons(n);
    }
    inline void SetPayload(unsigned char* d, int len) {
        memcpy(data, d, len);
    }
    inline void SetFlags(bool canBeDropped, uint8_t dataType, uint8_t payloadType) {
        flags = 0;
        flags += payloadType;
        flags += (dataType << 4);
        flags += (canBeDropped << 7);
    }
    inline void SetFlags(uint8_t value) {
        flags = value;
    }
    
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
    inline uint64_t GetPacketId() {
        return xmd_ntohll(packetId);
    }
    inline uint16_t GetStreamId() {
        return ntohs(streamId);
    }
    inline uint32_t GetGroupId() {
        return ntohl(groupId);
    }
    inline uint16_t GetSliceId() {
        return ntohs(sliceId);
    }
    inline uint16_t GetFECOPN() {
        return ntohs(FECOPN);
    }
    inline uint16_t GetFECPN() {
        return ntohs(FECPN);
    }
    inline unsigned char* GetData() {
        return data + STREAM_LEN_SIZE;
    }
    inline uint16_t GetPayloadLen() {
        uint16_t* len = (uint16_t*)data;
        return ntohs(*len);
    }
    inline bool GetIsLost() {
        bool result = flags & 0x80;
        return result;
    }
    inline uint8_t getDataType() {
        return ((flags & 0x70) >> 4);
    }
    inline uint8_t getPayloadType() {
        return (flags & 0x0F);
    }
};


/*
STREAM DATA ACK
PT=8
0         1        2        3        4        5     …  10
+--------+--------+--------+--------+--------+--------+---  --+
|     Public Flags(24)     |       Connection ID (64)…        | ->
+--------+--------+--------+--------+--------+--------+---  --+
11       12       13        14      15       16       17       18
+--------+--------+--------+--------+--------+--------+--------+--------+
|                              Packet ID(64)                            | ->
+--------+--------+--------+--------+--------+--------+--------+--------+
19       20       21        22        23       24       25       26
+--------+--------+--------+--------+--------+--------+--------+--------+
|                           Acked Packet ID(64)                         | ->
+--------+--------+--------+--------+--------+--------+--------+--------+
27       28       29       30
+--------+--------+--------+--------+
|               CRC(32)             |
+--------+--------+--------+--------+
*/

struct __attribute__((__packed__)) XMDStreamDataAck {
    uint64_t connId;
    uint64_t packetId;
    uint64_t ackedPacketId;
    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    } 
    inline void SetPacketId(uint64_t id) {
        packetId = xmd_htonll(id);
    }
    inline void SetAckedPacketId(uint64_t id) {
        ackedPacketId = xmd_htonll(id);
    }
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
    inline uint64_t GetPacketId() {
        return xmd_ntohll(packetId);
    }
    inline uint64_t GetAckedPacketId() {
        return xmd_ntohll(ackedPacketId);
    }
};



/**
 * PT=9 PING
 0         1        2        3        4        5     …  10
 +--------+--------+--------+--------+--------+--------+---  --+
 |     Public Flags(24)     |       Connection ID (64)…        | ->
 +--------+--------+--------+--------+--------+--------+---  --+
 11       12       13        14      15       16       17       18
 +--------+--------+--------+--------+--------+--------+--------+--------+
 |                              Packet ID(64)                            | ->
 +--------+--------+--------+--------+--------+--------+--------+--------+
 19       20       21        22       
 +--------+--------+--------+--------+
 |               CRC(32)             |
 +--------+--------+--------+--------+
 */

struct __attribute__((__packed__)) XMDPing {
    uint64_t connId;
    uint64_t packetId;
    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    }  
    inline void SetPacketId(uint64_t id) {
        packetId = xmd_htonll(id);
    }
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
    inline uint64_t GetPacketId() {
        return xmd_ntohll(packetId);
    }
};


/**
 * PT=10 PONG
 0         1        2        3        4        5     …  10
 +--------+--------+--------+--------+--------+--------+---  --+
 |     Public Flags(24)     |       Connection ID (64)…        | ->
 +--------+--------+--------+--------+--------+--------+---  --+
 11       12       13        14      15       16       17       18
 +--------+--------+--------+--------+--------+--------+--------+--------+
 |                              Packet ID(64)                            | ->
 +--------+--------+--------+--------+--------+--------+--------+--------+
 19       20       21        22        23       24       25       26
 +--------+--------+--------+--------+--------+--------+--------+--------+
 |                           Acked Packet ID(64)                         | ->
 +--------+--------+--------+--------+--------+--------+--------+--------+
 29       30       31         32        33       34      35        36
 +--------+--------+--------+--------+--------+--------+--------+--------+
 |          Timestamp1(32)           |          Timestamp2 (32)          | ->
 +--------+--------+--------+--------+--------+--------+--------+--------+
 37       38       39       40         41       42       43        44
 +--------+--------+--------+--------+--------+--------+--------+--------+
 |          total packets(32)        |         recv packets(32)          |
 +--------+--------+--------+--------+--------+--------+--------+--------+
 45        46        47        48
 +--------+--------+--------+--------+
 |               CRC(32)             |
 +--------+--------+--------+--------+

 */

 struct __attribute__((__packed__)) XMDPong {
    uint64_t connId;
    uint64_t packetId;
    uint64_t ackedPacketId;
    uint64_t timeStamp1;
    uint64_t timeStamp2;
    uint32_t totalPackets;
    uint32_t recvPackets;

    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    }  
    inline void SetPacketId(uint64_t id) {
        packetId = xmd_htonll(id);
    }
    inline void SetAckedPacketId(uint64_t id) {
        ackedPacketId = xmd_htonll(id);
    }
    inline void SetTimeStamp1(uint64_t ts) {
        timeStamp1 = xmd_htonll(ts);
    } 
    inline void SetTimeStamp2(uint64_t ts) {
        timeStamp2 = xmd_htonll(ts);
    } 
    inline void SetTotalPackets(uint32_t value) {
        totalPackets = htonl(value);
    }
    inline void SetRecvPackets(uint32_t value) {
        recvPackets = htonl(value);
    }
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
    inline uint64_t GetPacketId() {
        return xmd_ntohll(packetId);
    }
    inline uint64_t GetAckedPacketId() {
        return xmd_ntohll(ackedPacketId);
    }
    inline uint64_t GetTimestamp1() {
        return xmd_ntohll(timeStamp1);
    }
    inline uint64_t GetTimestamp2() {
        return xmd_ntohll(timeStamp2);
    }
    inline uint32_t GetTotalPackets() {
        return ntohl(totalPackets);
    }
    inline uint32_t GetRecvPackets() {
        return ntohl(recvPackets);
    }
 };



/*
PT=11, ACK STREAM DATA
0         1        2        3        4        5     …  10
+--------+--------+--------+--------+--------+--------+---  --+
|     Public Flags(24)     |       Connection ID (64)…        | ->
+--------+--------+--------+--------+--------+--------+---  --+
     11       12        13      … 18    19       20        21       22
+--------+--------+--------+-----…--+--------+--------+--------+--------+--------+--------+
|                 Packet ID(64)…    |  Stream ID(16)  |    timeout(16)  |wait time ms(16) |->
+--------+--------+--------+-----…--+--------+--------+--------+--------+--------+--------+
    23       24       25       26       27      28       29       30
+--------+--------+--------+--------+--------+--------+--------+--------+
|            Group ID(32)           |            GroupSize(32)          |->
+--------+--------+--------+--------+--------+--------+--------+--------+
  31         32        33       34       35       …          N     
+--------+--------+--------+--------+--------+--------+--------+---  ----+
|            Slice ID(32)           |flags(8)|          Payload …        |
+--------+--------+--------+--------+--------+--------+--------+---  ----+
   N+1      N+2     N+3       N+4
+--------+--------+--------+--------+
|               CRC(32)             |
+--------+--------+--------+--------+
*/

struct __attribute__((__packed__)) XMDACKStreamData {
    uint64_t connId;
    uint64_t packetId;
    uint16_t streamId;
    uint16_t timeout;
    uint16_t waitTimeMs;
    uint32_t groupId;
    uint32_t groupSize;
    uint32_t sliceId;
    uint8_t flags;
    unsigned char data[0];
    
    inline void SetConnId(uint64_t id) {
        connId = xmd_htonll(id);
    } 
    inline void SetPacketId(uint64_t id) {
        packetId = xmd_htonll(id);
    }
    inline void SetStreamId(uint16_t id) {
        streamId = htons(id);
    }
    inline void SetWaitTime(uint16_t t) {
        waitTimeMs = htons(t);
    }
    inline void SetGroupId(uint32_t id) {
        groupId = htonl(id);
    }
    inline void SetGroupSize(uint32_t size) {
        groupSize = htonl(size);
    }
    inline void SetSliceId(uint32_t id) {
        sliceId = htonl(id);
    }
    inline void SetFlags(bool canBeDropped, uint8_t dataType, uint8_t payloadType) {
        flags = 0;
        flags += payloadType;
        flags += (dataType << 4);
        flags += (canBeDropped << 7);
    }
    inline void SetFlags(uint8_t value) {
        flags = value;
    }
    inline void SetPayload(unsigned char* d, int len) {
        memcpy(data, d, len);
    }
    inline uint64_t GetConnId() {
        return xmd_ntohll(connId);
    }
    inline uint64_t GetPacketId() {
        return xmd_ntohll(packetId);
    }
    inline uint16_t GetStreamId() {
        return ntohs(streamId);
    }
    inline uint16_t GetWaitTime() {
        return ntohs(waitTimeMs);
    }
    inline uint32_t GetGroupId() {
        return ntohl(groupId);
    }
    inline uint32_t GetGroupSize() {
        return ntohl(groupSize);
    }
    inline uint32_t GetSliceId() {
        return ntohl(sliceId);
    }
    inline bool GetIsLost() {
        bool result = flags & 0x80;
        return result;
    }
    inline uint8_t getDataType() {
        return ((flags & 0x70) >> 4);
    }
    inline uint8_t getPayloadType() {
        return (flags & 0x0F);
    }
    inline unsigned char* GetPayload() {
        return data;
    }
};


class XMDPacketManager {
private:
    XMDPacket* xmdPacket_;
    int len_;

public:
    int buildConn(uint64_t connId, unsigned char* data, int len, int timeout, int nlen, unsigned char* n, int elen, unsigned char* e, bool isEncrypt);
    int buildConnReset(uint64_t connId, ConnResetType type);
    int buildConnResp(uint64_t connId, unsigned char* key, int len);
    int buildConnClose(uint64_t connId);
    int buildDatagram(unsigned char* data, int len);
    int buildStreamClose(uint64_t connId, uint16_t streamId, bool isEncrypt, std::string key);
    int buildFECStreamData(XMDFECStreamData stData, unsigned char* data, int len, bool isEncrypt, std::string key);
    int buildAckStreamData(XMDACKStreamData stData, unsigned char* data, int len, bool isEncrypt, std::string key);
    int buildStreamDataAck(uint64_t connId, uint64_t packetid, uint64_t ackPacketId, bool isEncrypt, std::string key);
    int buildXMDPing(uint64_t connId, bool isEncrypt, std::string key, uint64_t packetid);
    int buildXMDPong(XMDPong pongData, bool isEncrypt, std::string key);
    XMDConnection* decodeNewConn(unsigned char* data, int len);
    XMDConnResp* decodeConnResp(unsigned char* data, int len);
    XMDConnClose* decodeConnClose(unsigned char* data, int len);
    XMDStreamClose* decodeStreamClose(unsigned char* data, int len, bool isEncrypt, std::string key);
    XMDFECStreamData* decodeFECStreamData(unsigned char* data, int len, bool isEncrypt, std::string key);
    XMDACKStreamData* decodeAckStreamData(unsigned char* data, int len, bool isEncrypt, std::string key);
    XMDStreamDataAck* decodeStreamDataAck(unsigned char* data, int len, bool isEncrypt, std::string key);
    XMDConnReset* decodeConnReset(unsigned char* data, int len);
    XMDPing* decodeXMDPing(unsigned char* data, int len, bool isEncrypt, std::string key);
    XMDPong* decodeXMDPong(unsigned char* data, int len, bool isEncrypt, std::string key);
    int encode(XMDPacket* &data, int& len);
    XMDPacket* decode(char* data, int len);
};

#endif //XMDPACKET_H
