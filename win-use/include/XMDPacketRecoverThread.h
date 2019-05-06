#ifndef RECOVERTHREAD_H
#define RECOVERTHREAD_H

#include "xmd_thread.h"
#include "PacketDecoder.h"

const int GROUPMAP_CHECK_INTERVAL = 1;  //1ms
const int FEC_GROUP_DELETE_INTERVAL = 10000;  //10s
const int ACK_GROUP_DELETE_INTERVAL = 100000;  //100s


struct SlicePacket {
    unsigned char data[MAX_PACKET_SIZE + STREAM_LEN_SIZE];

    SlicePacket(unsigned char* slice_data, int len) {
        memset(data, 0, MAX_PACKET_SIZE + STREAM_LEN_SIZE);
        memcpy(data, slice_data, len);
    }
    SlicePacket() {
        memset(data, 0, MAX_PACKET_SIZE + STREAM_LEN_SIZE);
    }
};

struct PartitionPacket {
    bool isComplete;
    uint16_t FEC_OPN;
    uint16_t FEC_PN;
    int len;
    std::map<uint16_t, SlicePacket> sliceMap;
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
    unsigned char data[MAX_PACKET_SIZE];
    AckStreamSlice(unsigned char* d, int l) {
        len = l;
        memset(data, 0, MAX_PACKET_SIZE);
        memcpy(data, d, len);
    }
    AckStreamSlice() {
        len = 0;
        memset(data, 0, MAX_PACKET_SIZE);
    }
};

struct AckGroupPakcet {
    uint32_t groupSize;
    uint64_t connId;
    uint16_t streamId;
    uint64_t groupId;
    uint64_t create_time;
    int len;
    std::map<uint16_t, AckStreamSlice> sliceMap;
};


class GroupManager {
private:
    std::unordered_map<std::string, GroupPacket> groupMap_;
    std::unordered_map<std::string, AckGroupPakcet> ackGroupMap_;
    int last_check_time_;
    XMDCommonData* commonData_;
public:
    GroupManager(XMDCommonData* data) {
        last_check_time_ = 0;
        commonData_ = data;
    }
    int insertFecStreamPacket(XMDFECStreamData* packet, int len);
    int insertAckStreamPacket(XMDACKStreamData* packet, int len);
    int getCompletePacket(GroupPacket& gPakcet, unsigned char* &data, int& len);
    void checkGroupMap();
    bool doFecRecover(PartitionPacket& pPacket);
};


class XMDPacketRecoverThread : public XMDThread {
public:
    virtual void* process();
    XMDPacketRecoverThread(int id, XMDCommonData* commonData);
    ~XMDPacketRecoverThread();

    void stop();

private:
    bool stopFlag_;
    int thread_id_;
    XMDCommonData* commonData_;
    GroupManager* groupManager_;
};

#endif //RECOVERTHREAD_H

