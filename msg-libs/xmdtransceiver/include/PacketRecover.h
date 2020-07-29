#ifndef PACKETRECOVER_H
#define PACKETRECOVER_H

#include "XMDCommonData.h"
#include "PacketCallback.h"
#include "XMDPacket.h"

struct SlicePacket {
    unsigned char* data;
    int length;

    SlicePacket(unsigned char* slice_data, int len) {
        length = len;
        data = new unsigned char[len];
        memcpy(data, slice_data, len);
    }
    ~SlicePacket() {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
};

struct PartitionPacket {
    bool isComplete;
    uint16_t FEC_OPN;
    uint16_t FEC_PN;
    uint16_t slice_len;
    int len;
    std::map<uint16_t, SlicePacket*> sliceMap;
};

struct GroupPacket {
    bool isComplete;
    uint8_t partitionSize;
    uint64_t create_time;
    uint64_t connId;
    uint16_t streamId;
    uint64_t groupId;
    int len;
    std::map<uint8_t, PartitionPacket> partitionMap;
};

struct AckStreamSlice {
    int len;
    unsigned char* data;
    AckStreamSlice(unsigned char* d, int l) {
        len = l;
        data = new  unsigned char[len];
        memcpy(data, d, len);
    }
    ~AckStreamSlice() {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
};

struct AckGroupPakcet {
    uint32_t groupSize;
    uint64_t connId;
    uint16_t streamId;
    uint64_t groupId;
    uint64_t create_time;
    int len;
    std::map<uint16_t, AckStreamSlice*> sliceMap;
};


class PacketRecover {
private:
    std::unordered_map<std::string, GroupPacket> groupMap_;
    std::unordered_map<std::string, AckGroupPakcet> ackGroupMap_;
    int last_check_time_;
    XMDCommonData* commonData_;
    WorkerCommonData* workerCommonData_;
    PacketCallback* packetCallback_;
public:
    PacketRecover(XMDCommonData* data, WorkerCommonData* workerCommonData, PacketCallback* packetCallback) {
        last_check_time_ = 0;
        commonData_ = data;
        workerCommonData_ = workerCommonData;
        packetCallback_ = packetCallback;
    }
    ~PacketRecover();
    int insertFecStreamPacket(XMDFECStreamData* packet, int len);
    int insertAckStreamPacket(XMDACKStreamData* packet, int len);
    int getCompletePacket(GroupPacket& gPakcet, unsigned char* &data);

    bool doFecRecover(PartitionPacket& pPacket);
    void deleteBufferData();
    void deleteFromAckGroupMap(std::string key);
    void deleteFromFECGroupMap(std::string key);
};


#endif //PACKETRECOVER_H


