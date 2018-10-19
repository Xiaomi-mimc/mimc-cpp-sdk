#ifndef PACKETBUILDER_H
#define PACKETBUILDER_H

#include <vector>
#include <map>
#include <memory>
#include "XMDCommonData.h"
#include "XMDPacket.h"
#include <iostream>


struct partitionData {
    uint16_t fec_opn;
    uint16_t fec_pn;
    unsigned char origin_data[MAX_ORIGIN_PACKET_NUM_IN_PARTITION * (MAX_PACKET_SIZE + STREAM_LEN_SIZE)];
};

struct groupData {
    uint32_t ip;
    int port;
    int partitionSize;
    uint64_t connId;
    uint32_t streamId;
    uint32_t groupId;
    int timeout;
    bool isEncrypt;
    std::string sessionKey;
    uint8_t flags;
    partitionData* partitionVec;

    groupData() {
        partitionVec = NULL;
        partitionSize = 0;
    }

    void construct(uint32_t i, int p, int size, uint64_t cId, uint32_t sId, uint32_t gId, int t, bool e, std::string key, uint8_t flag) {
        ip = i;
        port = p;
        partitionSize = size;
        connId = cId;
        streamId = sId;
        groupId = gId;
        timeout = t;
        isEncrypt = e;
        sessionKey = key;
        flags = flag;
        partitionVec = new partitionData[partitionSize];
    }
    ~groupData() {
        if (partitionVec) {
            delete[] partitionVec;
        }
    }
};


class PacketBuilder {
public:
    ~PacketBuilder();
    PacketBuilder(XMDCommonData* data);
    void build(StreamQueueData* queueData);
    void buildFecStreamPacket(StreamQueueData* queueData, ConnInfo connInfo, StreamInfo sInfo);
    void buildAckStreamPacket(StreamQueueData* queueData, ConnInfo connInfo, StreamInfo sInfo);
    void buildRedundancyPacket();
    int getRedundancyPacketNum(int fecopn, double packetLossRate);

private:
    XMDCommonData* commonData_;
    //PacketDispatcher* dispatcher_;
    unsigned char fecRedundancyData_[MAX_ORIGIN_PACKET_NUM_IN_PARTITION * (MAX_PACKET_SIZE + STREAM_LEN_SIZE)];
    int partition_size_;
    groupData groupData_;
    bool isBigPacket_;
    int sendPacketPreMS_;
    uint64_t sendTime_;
    
};


#endif // PACKETBUILDER_H