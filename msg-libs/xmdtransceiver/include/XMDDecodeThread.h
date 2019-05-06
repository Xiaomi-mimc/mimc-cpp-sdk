#ifndef ENCODETHREAD_H
#define ENCODETHREAD_H

#include "xmd_thread.h"
#include "PacketDecoder.h"

const int GROUPMAP_CHECK_INTERVAL = 1;  //1ms
const int GROUP_DELETE_INTERVAL = 10000;  //10s

struct SlicePacket {
    unsigned char* data;

    SlicePacket(unsigned char* slice_data, int len) {
        data = new unsigned char[MAX_PACKET_SIZE + STREAM_LEN_SIZE];
        memset(data, 0, MAX_PACKET_SIZE + STREAM_LEN_SIZE);
        memcpy(data, slice_data, len);
        //data = slice_data;
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


class GroupManager {
private:
    std::map<std::string, GroupPacket> groupMap_;
    int last_check_time_;
    XMDCommonData* commonData_;
public:
    GroupManager(XMDCommonData* data) {
        last_check_time_ = 0;
        commonData_ = data;
    }
    bool isComplete(uint32_t id);
    int insertPacket(XMDFECStreamData* packet, int len);
    int getCompletePacket(GroupPacket& gPakcet, unsigned char* &data, int& len);
    int deleteGroup(uint32_t id);
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

#endif //ENCODETHREAD_H

