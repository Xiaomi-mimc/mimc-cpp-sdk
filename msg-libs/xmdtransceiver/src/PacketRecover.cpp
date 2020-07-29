#ifdef _WIN32
#else
#include <unistd.h>
#endif
#include <sstream> 
#include "PacketRecover.h"
#include "fec.h"

PacketRecover::~PacketRecover() {
    deleteBufferData();
}


int PacketRecover::insertFecStreamPacket(XMDFECStreamData* packet, int len) { 
    SlicePacket* slicePacket = new SlicePacket(packet->data, len - sizeof(XMDFECStreamData));
    std::stringstream ss;
    ss << packet->GetConnId() << packet->GetStreamId() << packet->GetGroupId();
    std::string tmpKey = ss.str();
    std::unordered_map<std::string, GroupPacket>::iterator it = groupMap_.find(tmpKey);
    if (it  == groupMap_.end()) {
        //XMDLoggerWrapper::instance()->debug("new group id=%d,pid=%d,slice id=%d",
        //                                  packet->GetGroupId(),packet->PId,packet->GetSliceId());
        PartitionPacket parPacket;
        parPacket.FEC_OPN = packet->GetFECOPN();
        parPacket.FEC_PN = packet->GetFECPN();
        //parPacket.slice_len = len - sizeof(XMDFECStreamData);
        parPacket.sliceMap[packet->GetSliceId()] = slicePacket;
        if (packet->GetFECOPN() == 1) {
            parPacket.isComplete = true;
        } else {
            parPacket.isComplete = false;
        }
    
        GroupPacket gPacket;
        gPacket.partitionSize = packet->PSize;
        gPacket.connId = packet->GetConnId();
        gPacket.streamId = packet->GetStreamId();
        gPacket.groupId = packet->GetGroupId();
        gPacket.isComplete = false;

        if (packet->GetSliceId() < packet->GetFECOPN()) {
            parPacket.len = packet->GetPayloadLen();
            gPacket.len = packet->GetPayloadLen();
        }
        gPacket.create_time = current_ms();
        gPacket.partitionMap[packet->PId] = parPacket;
        groupMap_[tmpKey] = gPacket;

        DeleteGroupTimeout* deleteGroupTimeout = new DeleteGroupTimeout();
        deleteGroupTimeout->key = tmpKey;
        workerCommonData_->startTimer(packet->GetConnId(), WORKER_DELETE_FECGROUP_TIMEOUT, FEC_GROUP_DELETE_INTERVAL, (void*)deleteGroupTimeout, sizeof(DeleteGroupTimeout));
    } else {
        GroupPacket &gPacket = it->second;
        if (gPacket.isComplete) {
            //XMDLoggerWrapper::instance()->debug("group(%d) already completed, drop this packet.", it->second.groupId);
            delete slicePacket;
            return 1;
        }
        std::map<uint8_t, PartitionPacket>::iterator iter = gPacket.partitionMap.find(packet->PId);
        if (iter == gPacket.partitionMap.end()) {
            //XMDLoggerWrapper::instance()->debug("group exist id=%d, new pid=%d,slice id=%d",
            //                                  packet->GetGroupId(),packet->PId,packet->GetSliceId());
        
            PartitionPacket parPacket;
            parPacket.FEC_OPN = packet->GetFECOPN();
            parPacket.FEC_PN = packet->GetFECPN();
            //parPacket.slice_len = len - sizeof(XMDFECStreamData);
            parPacket.sliceMap[packet->GetSliceId()] = slicePacket;
            if (packet->GetSliceId() < packet->GetFECOPN()) {
                parPacket.len = packet->GetPayloadLen();
            }
            if (packet->GetFECOPN() == 1) {
                parPacket.isComplete = true;
            } else {
                parPacket.isComplete = false;
            }
            gPacket.partitionMap[packet->PId] = parPacket;
        } else {
            //XMDLoggerWrapper::instance()->debug("group&p exist id=%d,pid=%d,slice id=%d",
             //                                 packet->GetGroupId(),packet->PId,packet->GetSliceId());
        
            PartitionPacket &pPacket = iter->second;
            if (pPacket.isComplete) {
                //XMDLoggerWrapper::instance()->debug("partition(%d) already completed, drop this packet.", iter->first);
                delete slicePacket;
                return 1;
            }
            std::map<uint16_t, SlicePacket*>::iterator sliceIt = pPacket.sliceMap.find(packet->GetSliceId());
            if (sliceIt != pPacket.sliceMap.end()) {
                XMDLoggerWrapper::instance()->debug("drop repeated packet, partition(%d),slice(%d).", 
                                                  iter->first, packet->GetSliceId());
                delete slicePacket;
                return 1;
            }

            if (MAX_PACKET_SIZE + STREAM_LEN_SIZE < len - sizeof(XMDFECStreamData)) {
                XMDLoggerWrapper::instance()->error("slice packet len(%d) not same with partition len(%d)",
                                                     len - sizeof(XMDFECStreamData), MAX_PACKET_SIZE + STREAM_LEN_SIZE);
                delete slicePacket;
                return -1;
            }

            pPacket.sliceMap[packet->GetSliceId()] = slicePacket;
            if (packet->GetSliceId() < packet->GetFECOPN()) {
                pPacket.len += packet->GetPayloadLen();
            }
            if (pPacket.FEC_OPN <= pPacket.sliceMap.size()) {
                doFecRecover(pPacket);
            }
        }
    }

    bool isGroupComplete = true;
    int groupLen = 0;
    std::unordered_map<std::string, GroupPacket>::iterator iter = groupMap_.find(tmpKey);
    if (iter == groupMap_.end()) {
        return 0;
    }
    
    if (iter->second.partitionSize != iter->second.partitionMap.size()) {
        isGroupComplete = false;
    } else {
        std::map<uint8_t, PartitionPacket>::iterator it2 = iter->second.partitionMap.begin();
        for (; it2 != iter->second.partitionMap.end(); it2++) {
            if (it2->second.isComplete) {
                groupLen += it2->second.len;
            } else {
                isGroupComplete = false;
                break;
            }
        }
    }

    if (!isGroupComplete) {
        return 0;
    }

    iter->second.len = groupLen;
    iter->second.isComplete = true;
                
    unsigned char* groupData = NULL;
    if (getCompletePacket(iter->second, groupData) == 0) {
        CallbackQueueData* callbackData = new CallbackQueueData(iter->second.connId, iter->second.streamId, 
                                           iter->second.groupId, FEC_STREAM, groupLen, current_ms(), groupData);
                                                           
        XMDLoggerWrapper::instance()->debug("conn=%ld, stream=%d, group id = %d, packet len =%d", 
                                              iter->second.connId, iter->second.streamId, iter->second.groupId, callbackData->len);
        packetCallback_->CallbackFECStreamData(callbackData);
    }

    deleteFromFECGroupMap(tmpKey);

    return 0;
}

int PacketRecover::insertAckStreamPacket(XMDACKStreamData* packet, int len) {
    std::stringstream ss_stream;
    ss_stream << packet->GetConnId() << packet->GetStreamId();
    std::string streamKey = ss_stream.str();
    uint32_t lastCallbackGroupId = 0;
    if (workerCommonData_->getLastCallbackGroupId(streamKey, lastCallbackGroupId) && lastCallbackGroupId >= packet->GetGroupId()) {
        XMDLoggerWrapper::instance()->debug("conn(%ld), group(%d),already received, drop packet.", 
                                          packet->GetConnId(), packet->GetGroupId());
        return 0;
    }
    
    AckStreamSlice* slice = new AckStreamSlice(packet->GetPayload(), len - sizeof(XMDACKStreamData));
    std::stringstream ss;
    ss << packet->GetConnId() << packet->GetStreamId() << packet->GetGroupId();
    std::string tmpKey = ss.str();
    std::unordered_map<std::string, AckGroupPakcet>::iterator it = ackGroupMap_.find(tmpKey);
    if (it == ackGroupMap_.end()) {
        if (packet->GetGroupSize() == 1) {
            int cbLen = len - sizeof(XMDACKStreamData);
            unsigned char* cbData = new unsigned char[cbLen];
            memcpy(cbData, packet->GetPayload(), cbLen);
            CallbackQueueData* callbackData = new CallbackQueueData(packet->GetConnId(), packet->GetStreamId(), 
                                              packet->GetGroupId(), ACK_STREAM, 
                                              cbLen, current_ms(), cbData);
            XMDLoggerWrapper::instance()->debug("packet recover succ, connid(%ld),streamid(%d),groupid(%d)",
                                                 packet->GetConnId(), packet->GetStreamId(), packet->GetGroupId());
            packetCallback_->CallbackAckStreamData(callbackData);
            delete slice;
        } else {
            AckGroupPakcet ackGroup;
            ackGroup.connId = packet->GetConnId();
            ackGroup.create_time = current_ms();
            ackGroup.streamId = packet->GetStreamId();
            ackGroup.groupId = packet->GetGroupId();
            ackGroup.groupSize = packet->GetGroupSize();
            ackGroup.len = len - sizeof(XMDACKStreamData);
            ackGroup.sliceMap[packet->GetSliceId()] = slice;
            ackGroupMap_[tmpKey] = ackGroup;

            DeleteGroupTimeout* deleteGroupTimeout = new DeleteGroupTimeout();
            deleteGroupTimeout->key = tmpKey;
            workerCommonData_->startTimer(packet->GetConnId(), WORKER_DELETE_ACKGROUP_TIMEOUT, ACK_GROUP_DELETE_INTERVAL, (void*)deleteGroupTimeout, sizeof(DeleteGroupTimeout));
        }
        
    } else {
        AckGroupPakcet &ackGroup = it->second;
        std::map<uint16_t, AckStreamSlice*>::iterator sliceIt = ackGroup.sliceMap.find(packet->GetSliceId());
        if (sliceIt != ackGroup.sliceMap.end()) {
            XMDLoggerWrapper::instance()->debug("conn(%ld), drop repeated packet, packetid(%ld),slice(%d).", 
                                              packet->GetConnId(), packet->GetPacketId(), packet->GetSliceId());
            delete slice;
            return 0;
        }
        if (packet->GetSliceId() > ackGroup.groupSize) {
            XMDLoggerWrapper::instance()->debug("conn(%ld), invalid slice id, packetid(%ld),slice(%d).", 
                                                  packet->GetConnId(), packet->GetPacketId(), packet->GetSliceId());
            delete slice;
            return 0;
        }
        ackGroup.len += (len - sizeof(XMDACKStreamData));
        std::map<uint16_t, AckStreamSlice*>::iterator sliceIt2 = ackGroup.sliceMap.find(packet->GetSliceId());
        if (sliceIt2 == ackGroup.sliceMap.end()) {
            ackGroup.sliceMap[packet->GetSliceId()] = slice;
        } else {
            delete slice;
            return 0;
        }
        if (ackGroup.sliceMap.size() >=  ackGroup.groupSize) {
            unsigned char* gData = new unsigned char[ackGroup.len];
            int pos = 0;
            for (unsigned int i = 0; i < ackGroup.groupSize; i++) {
                if (pos + ackGroup.sliceMap[i]->len > ackGroup.len) {
                    XMDLoggerWrapper::instance()->debug("conn(%ld), invalid slice len,slice(%d) pos=%d len=%d, group len=%d", 
                                                      packet->GetConnId(), i, pos, ackGroup.sliceMap[i]->len, ackGroup.len);
                    delete slice;
                    return -1;
                }
                memcpy(gData + pos, ackGroup.sliceMap[i]->data, ackGroup.sliceMap[i]->len);
                pos += ackGroup.sliceMap[i]->len;
            }
            XMDLoggerWrapper::instance()->debug("packet recover succ, connid(%ld),streamid(%d),groupid(%d)",
                                                 packet->GetConnId(), packet->GetStreamId(), packet->GetGroupId());
    
            CallbackQueueData* callbackData = new CallbackQueueData(packet->GetConnId(), packet->GetStreamId(), 
                                                  packet->GetGroupId(), ACK_STREAM, ackGroup.len, current_ms(), gData);
            packetCallback_->CallbackAckStreamData(callbackData);

            
            for (sliceIt = ackGroup.sliceMap.begin(); sliceIt != ackGroup.sliceMap.end(); sliceIt++){
                delete sliceIt->second;
            }
            ackGroupMap_.erase(it);
        }
    }

    return 0;
}


int PacketRecover::getCompletePacket(GroupPacket& gPakcet, unsigned char* &data) {
    int len = gPakcet.len;
    data = new unsigned char[len];
    int pos = 0;

    for (int i = 0; i < gPakcet.partitionSize; i++) {
        for (int j = 0; j < gPakcet.partitionMap[i].FEC_OPN; j++) {
            //uint16_t* tmpLen = (uint16_t*)gPakcet.partitionMap[i].sliceMap[j]->data;
            uint16_t tmpLen = 0;
            trans_uint16_t(tmpLen, (char*)gPakcet.partitionMap[i].sliceMap[j]->data);
            uint16_t sliceLen = ntohs(tmpLen);
            if (sliceLen > MAX_PACKET_SIZE + STREAM_LEN_SIZE) {
                XMDLoggerWrapper::instance()->debug("invalid slice len=%d", sliceLen);
                return -1;
            }
            if (pos + sliceLen > len) {
                XMDLoggerWrapper::instance()->debug("invalid slice len=%d", sliceLen);
                return -1;
            }
            XMDLoggerWrapper::instance()->debug("group(%d), partition(%d), slice(%d) len=%d", gPakcet.groupId, i, j, sliceLen);
            memcpy(data + pos, gPakcet.partitionMap[i].sliceMap[j]->data + STREAM_LEN_SIZE, sliceLen);
            pos += sliceLen;
        }
    }

    return 0;
}


void PacketRecover::deleteBufferData() {
    std::unordered_map<std::string, GroupPacket>::iterator it = groupMap_.begin();
    for(; it != groupMap_.end(); ) {
        std::map<uint8_t, PartitionPacket>::iterator it2 = it->second.partitionMap.begin();
        for (; it2 != it->second.partitionMap.end(); it2++) {
            std::map<uint16_t, SlicePacket*>::iterator it3 = it2->second.sliceMap.begin();
            for (; it3 != it2->second.sliceMap.end(); it3++) {
                if (it3->second) {
                    delete it3->second;
                }
            }
        }
        groupMap_.erase(it++);
    }

    std::unordered_map<std::string, AckGroupPakcet>::iterator iter = ackGroupMap_.begin();
    for (; iter != ackGroupMap_.end(); ) {
        std::map<uint16_t, AckStreamSlice*>::iterator sliceIt = iter->second.sliceMap.begin();
        for (; sliceIt != iter->second.sliceMap.end(); sliceIt++) {
            delete sliceIt->second;
        }
        ackGroupMap_.erase(iter++);
    }
}

void PacketRecover::deleteFromAckGroupMap(std::string key) {
    XMDLoggerWrapper::instance()->debug("delete from ack group map key=%s", key.c_str());
    std::unordered_map<std::string, AckGroupPakcet>::iterator iter = ackGroupMap_.find(key);
    if (iter != ackGroupMap_.end()) {
        std::map<uint16_t, AckStreamSlice*>::iterator sliceIt = iter->second.sliceMap.begin();
        for (; sliceIt != iter->second.sliceMap.end(); sliceIt++){
            delete sliceIt->second;
        }
        ackGroupMap_.erase(iter);
    }
}

void PacketRecover::deleteFromFECGroupMap(std::string key) {
    XMDLoggerWrapper::instance()->debug("delete from fec group map key=%s", key.c_str());
    std::unordered_map<std::string, GroupPacket>::iterator it = groupMap_.find(key);
    if (it != groupMap_.end()) {
        std::map<uint8_t, PartitionPacket>::iterator it2 = it->second.partitionMap.begin();
        for (; it2 != it->second.partitionMap.end(); it2++) {
            std::map<uint16_t, SlicePacket*>::iterator it3 = it2->second.sliceMap.begin();
            for (; it3 != it2->second.sliceMap.end(); it3++) {
                if (it3->second) {
                    delete it3->second;
                }
            }
        }
        groupMap_.erase(it);
    }
}



bool PacketRecover::doFecRecover(PartitionPacket& pPacket) {
    if (pPacket.isComplete) {
        return true;
    }

    if (pPacket.FEC_PN == 0) {
        if (pPacket.FEC_OPN == pPacket.sliceMap.size()) {
            pPacket.isComplete = true;
            return true;
        } else {
            return false;
        }
    }

    pPacket.isComplete = true;
    int fec_len = MAX_PACKET_SIZE + STREAM_LEN_SIZE;
    
    bool result = true;
    bool isNeedFec = false;
    int n = pPacket.FEC_OPN;
    bool isSliceExist[n];
    for (int i = 0; i < n; i++) {
        isSliceExist[i] = false;
    }

    int* matrix = new int [n * n];
    for (int i = 0; i < n * n; i++) {
        matrix[i] = 0;
    }
    unsigned char* input = new unsigned char[n * fec_len];
    memset(input, 0, n * fec_len);
    unsigned char* output = new unsigned char[n * fec_len];
    memset(output, 0, n * fec_len);
    

    Fec fec(pPacket.FEC_OPN, pPacket.FEC_PN);
    int* origin_matrix = fec.get_matrix();

    int index = 0;
    std::map<uint16_t, SlicePacket*>::iterator it = pPacket.sliceMap.begin();
    for (; it != pPacket.sliceMap.end(); it++) {
        if (index >= n) {
            break;
        }
        
        int sliceId = it->first;
        if (index != sliceId) {
            isNeedFec = true;
        }
        if (sliceId < n) {
            isSliceExist[sliceId] = true;
            matrix[index * n + sliceId] = 1;
        } else {
            isNeedFec = true;
            for (int k = 0; k < n; k++) {
                matrix[index * n + k] = origin_matrix[(sliceId - pPacket.FEC_OPN) * n + k];
            }
        }
        if (it->second->length < fec_len) {
            memcpy(input + index * fec_len, it->second->data, it->second->length);
        } else {
            memcpy(input + index * fec_len, it->second->data, fec_len);
        }
        index++;
    }


    if (!isNeedFec) {
        XMDLoggerWrapper::instance()->debug("no need to do fec.");
        goto fecend;
    }

    result = fec.reverse_matrix(matrix);
    if (!result) {
        pPacket.isComplete = false;
        goto fecend;
    }
    fec.fec_decode(input, fec_len, output);
    
    for (int i = 0; i < n; i++) {        
        if (!isSliceExist[i]) {
            SlicePacket *slicePacket = new SlicePacket(output + i * fec_len, fec_len);
            pPacket.sliceMap[i] = slicePacket;
            uint16_t tmpLen = 0;
            trans_uint16_t(tmpLen, (char*)(output + i * fec_len));
            uint16_t sliceLen = ntohs(tmpLen);
            XMDLoggerWrapper::instance()->info("do fec recover %d len =%d.", i, sliceLen);
            pPacket.len += sliceLen;
        }
    }

fecend:
    if (matrix) {
        delete[] matrix;
        matrix = NULL;
    }

    if (input) {
        delete[] input;
        input = NULL;
    }

    if (output) {
        delete[] output;
        output = NULL;
    }

    return result;
}

