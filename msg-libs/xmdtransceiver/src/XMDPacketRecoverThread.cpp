#include "XMDPacketRecoverThread.h"
#include <unistd.h>
#include <sstream> 

XMDPacketRecoverThread::XMDPacketRecoverThread(int id, XMDCommonData* commonData) {
    commonData_ = commonData;
    thread_id_ = id;
    stopFlag_ = false;
    groupManager_ = new GroupManager(commonData_);
}

XMDPacketRecoverThread::~XMDPacketRecoverThread() {
    commonData_ = NULL;
    if (groupManager_) {
        delete groupManager_;
        groupManager_ = NULL;
    }
}

void XMDPacketRecoverThread::stop() {
    stopFlag_ = true;
}

void* XMDPacketRecoverThread::process() {
    while (!stopFlag_) {
        groupManager_->checkGroupMap();
        StreamData* streamData = commonData_->packetRecoverQueuePop(thread_id_);
        if (streamData == NULL) {
            usleep(100);
            continue;
        }


        XMDPacketManager packetMan;
        if (streamData->type == FEC_STREAM) {
            XMDFECStreamData* fecStreamData = packetMan.decodeFECStreamData(streamData->data, streamData->len, false, "");
            groupManager_->insertFecStreamPacket(fecStreamData, streamData->len);
        } else if (streamData->type == ACK_STREAM) {
            XMDACKStreamData* ackStreamData = packetMan.decodeAckStreamData(streamData->data, streamData->len, false, "");
            groupManager_->insertAckStreamPacket(ackStreamData, streamData->len);
        }
        delete streamData;
    }
    
    return NULL;
}


int GroupManager::insertFecStreamPacket(XMDFECStreamData* packet, int len) { 
    SlicePacket slicePacket(packet->data, len - sizeof(XMDFECStreamData));
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
    } else {
        GroupPacket &gPacket = it->second;
        if (gPacket.isComplete) {
            //XMDLoggerWrapper::instance()->debug("group(%d) already completed, drop this packet.", it->second.groupId);
            return 1;
        }
        std::map<uint8_t, PartitionPacket>::iterator iter = gPacket.partitionMap.find(packet->PId);
        if (iter == gPacket.partitionMap.end()) {
            //XMDLoggerWrapper::instance()->debug("group exist id=%d, new pid=%d,slice id=%d",
            //                                  packet->GetGroupId(),packet->PId,packet->GetSliceId());
        
            PartitionPacket parPacket;
            parPacket.FEC_OPN = packet->GetFECOPN();
            parPacket.FEC_PN = packet->GetFECPN();
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
                return 1;
            }
            std::map<uint16_t, SlicePacket>::iterator sliceIt = pPacket.sliceMap.find(packet->GetSliceId());
            if (sliceIt != pPacket.sliceMap.end()) {
                XMDLoggerWrapper::instance()->debug("drop repeated packet, partition(%d),slice(%d).", 
                                                  iter->first, packet->GetSliceId());
                return 1;
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

    return 0;
}

int GroupManager::insertAckStreamPacket(XMDACKStreamData* packet, int len) {
    StreamInfo streamInfo;
    if (! commonData_->getStreamInfo(packet->GetConnId(), packet->GetStreamId(), streamInfo)) {
        return -1;
    }

    std::stringstream ss_stream;
    ss_stream << packet->GetConnId() << packet->GetStreamId();
    std::string streamKey = ss_stream.str();
    uint32_t lastCallbackGroupId = 0;
    if (commonData_->getLastCallbackGroupId(streamKey, lastCallbackGroupId) && lastCallbackGroupId >= packet->GetGroupId()) {
        XMDLoggerWrapper::instance()->debug("conn(%ld), group(%d),already received, drop packet.", 
                                          packet->GetConnId(), packet->GetGroupId());
        return 0;
    }
    
    AckStreamSlice slice(packet->GetPayload(), len - sizeof(XMDACKStreamData));
    std::stringstream ss;
    ss << packet->GetConnId() << packet->GetStreamId() << packet->GetGroupId();
    std::string tmpKey = ss.str();
    std::unordered_map<std::string, AckGroupPakcet>::iterator it = ackGroupMap_.find(tmpKey);
    if (it == ackGroupMap_.end()) {
        AckGroupPakcet ackGroup;
        ackGroup.connId = packet->GetConnId();
        ackGroup.create_time = current_ms();
        ackGroup.streamId = packet->GetStreamId();
        ackGroup.groupId = packet->GetGroupId();
        ackGroup.groupSize = packet->GetGroupSize();
        ackGroup.len = len - sizeof(XMDACKStreamData);
        if (ackGroup.groupSize == 1) {
            int cbLen = len - sizeof(XMDACKStreamData);
            unsigned char* cbData = new unsigned char[cbLen];
            memcpy(cbData, packet->GetPayload(), cbLen);
            CallbackQueueData* callbackData = new CallbackQueueData(packet->GetConnId(), packet->GetStreamId(), 
                                              packet->GetGroupId(), ACK_STREAM, 
                                              cbLen, current_ms(), cbData);
            XMDLoggerWrapper::instance()->debug("packet recover succ, connid(%ld),streamid(%d),groupid(%d)",
                                                 packet->GetConnId(), packet->GetStreamId(), packet->GetGroupId());
            commonData_->callbackQueuePush(callbackData);
            //commonData_->updateLastRecvGroupId(streamKey, packet->GetGroupId());
        } else {
            ackGroup.sliceMap[packet->GetSliceId()] = slice;
            ackGroupMap_[tmpKey] = ackGroup;
        }
        
    } else {
        AckGroupPakcet &ackGroup = it->second;
        std::map<uint16_t, AckStreamSlice>::iterator sliceIt = ackGroup.sliceMap.find(packet->GetSliceId());
        if (sliceIt != ackGroup.sliceMap.end()) {
            XMDLoggerWrapper::instance()->debug("conn(%ld), drop repeated packet, packetid(%ld),slice(%d).", 
                                              packet->GetConnId(), packet->GetPacketId(), packet->GetSliceId());
            return 0;
        }
        if (packet->GetSliceId() > ackGroup.groupSize) {
            XMDLoggerWrapper::instance()->debug("conn(%ld), invalid slice id, packetid(%ld),slice(%d).", 
                                                  packet->GetConnId(), packet->GetPacketId(), packet->GetSliceId());
            return 0;
        }
        ackGroup.len += (len - sizeof(XMDACKStreamData));
        ackGroup.sliceMap[packet->GetSliceId()] = slice;
        if (ackGroup.sliceMap.size() >=  ackGroup.groupSize) {
            unsigned char* gData = new unsigned char[ackGroup.len];
            int pos = 0;
            for (unsigned int i = 0; i < ackGroup.groupSize; i++) {
                if (pos + ackGroup.sliceMap[i].len > ackGroup.len) {
                    XMDLoggerWrapper::instance()->debug("conn(%ld), invalid slice len,slice(%d) pos=%d len=%d, group len=%d", 
                                                      packet->GetConnId(), i, pos, ackGroup.sliceMap[i].len, ackGroup.len);
                    return -1;
                }
                memcpy(gData + pos, ackGroup.sliceMap[i].data, ackGroup.sliceMap[i].len);
                pos += ackGroup.sliceMap[i].len;
            }
    
            CallbackQueueData* callbackData = new CallbackQueueData(packet->GetConnId(), packet->GetStreamId(), 
                                                  packet->GetGroupId(), ACK_STREAM, ackGroup.len, current_ms(), gData);
            XMDLoggerWrapper::instance()->debug("packet recover succ, connid(%ld),streamid(%d),groupid(%d)",
                                                 packet->GetConnId(), packet->GetStreamId(), packet->GetGroupId());
            commonData_->callbackQueuePush(callbackData);
            ackGroupMap_.erase(it);
            //commonData_->updateLastRecvGroupId(streamKey, packet->GetGroupId());
        }
    }

    return 0;
}


int GroupManager::getCompletePacket(GroupPacket& gPakcet, unsigned char* &data, int& len) {
    len = gPakcet.len;
    data = new unsigned char[len];
    int pos = 0;

    for (int i = 0; i < gPakcet.partitionSize; i++) {
        for (int j = 0; j < gPakcet.partitionMap[i].FEC_OPN; j++) {
            uint16_t* tmpLen = (uint16_t*)gPakcet.partitionMap[i].sliceMap[j].data;
            uint16_t sliceLen = ntohs(*tmpLen);
            if (sliceLen > MAX_PACKET_SIZE) {
                XMDLoggerWrapper::instance()->debug("invalid slice len=%d", sliceLen);
                return -1;
            }
            if (pos + sliceLen > len) {
                XMDLoggerWrapper::instance()->debug("invalid slice len=%d", sliceLen);
                return -1;
            }
            XMDLoggerWrapper::instance()->debug("group(%d), partition(%d), slice(%d) len=%d", gPakcet.groupId, i, j, sliceLen);
            memcpy(data + pos, gPakcet.partitionMap[i].sliceMap[j].data + STREAM_LEN_SIZE, sliceLen);
            pos += sliceLen;
        }
    }

    return 0;
}

void GroupManager::checkGroupMap() {
    uint64_t currentTime = current_ms();
    if (currentTime - last_check_time_ < GROUPMAP_CHECK_INTERVAL) {
        return;
    }

    std::unordered_map<std::string, GroupPacket>::iterator it = groupMap_.begin();
    for(; it != groupMap_.end(); ) {
        if (currentTime - it->second.create_time > FEC_GROUP_DELETE_INTERVAL) {
            if (!it->second.isComplete) {
                XMDLoggerWrapper::instance()->warn("fec group is not completed when deleting, conn(%ld) stream(%d) group(%d)", 
                                                it->second.connId, it->second.streamId, it->second.groupId);
            }
            groupMap_.erase(it++);
        } else {
            if (it->second.isComplete) {
                it++;
                continue;
            }
            
            bool isGroupComplete = true;
            int len = 0;
            if (it->second.partitionSize != it->second.partitionMap.size()) {
                isGroupComplete = false;
            } else {
                std::map<uint8_t, PartitionPacket>::iterator it2 = it->second.partitionMap.begin();
                for (; it2 != it->second.partitionMap.end(); it2++) {
                    if (it2->second.isComplete) {
                        len += it2->second.len;
                    } else {
                        isGroupComplete = false;
                    }
                }
            }

            
            if (isGroupComplete) {
                it->second.len = len;
                it->second.isComplete = true;
                
                unsigned char* groupData = NULL;
                int groupLen = 0;
                if (getCompletePacket(it->second, groupData, groupLen) == 0) {
                    CallbackQueueData* queueData = new CallbackQueueData(it->second.connId, it->second.streamId, 
                                                       it->second.groupId, FEC_STREAM, groupLen, current_ms(), groupData);
                                                           
                    XMDLoggerWrapper::instance()->debug("conn=%ld, stream=%d, group id = %d, packet len =%d", 
                                                      it->second.connId, it->second.streamId, it->second.groupId, queueData->len);
                    commonData_->callbackQueuePush(queueData);
                }
            } 
            
            it++;
            
        }
    }

    std::unordered_map<std::string, AckGroupPakcet>::iterator iter = ackGroupMap_.begin();
    for (; iter != ackGroupMap_.end(); ) {
        if (currentTime - iter->second.create_time > ACK_GROUP_DELETE_INTERVAL) {
            XMDLoggerWrapper::instance()->warn("ack stream group is not completed, conn(%ld) stream(%d) group(%d)", 
                                             iter->second.connId, iter->second.streamId, iter->second.groupId);
            ackGroupMap_.erase(iter++);
        } else {
            iter++;
        }
    }
    last_check_time_ = currentTime;
}

bool GroupManager::doFecRecover(PartitionPacket& pPacket) {
    if (pPacket.isComplete) {
        return true;
    }
    pPacket.isComplete = true;
    
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
    unsigned char* input = new unsigned char[n * (MAX_PACKET_SIZE + STREAM_LEN_SIZE)];
    memset(input, 0, n * (MAX_PACKET_SIZE + STREAM_LEN_SIZE));
    unsigned char* output = new unsigned char[n * (MAX_PACKET_SIZE + STREAM_LEN_SIZE)];
    memset(output, 0, n * (MAX_PACKET_SIZE + STREAM_LEN_SIZE));
    

    Fec fec(pPacket.FEC_OPN, pPacket.FEC_PN);
    int* origin_matrix = fec.get_matrix();

    int index = 0;
    std::map<uint16_t, SlicePacket>::iterator it = pPacket.sliceMap.begin();
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
        memcpy(input + index * (MAX_PACKET_SIZE + STREAM_LEN_SIZE), it->second.data, MAX_PACKET_SIZE + STREAM_LEN_SIZE);
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
    fec.fec_decode(input, MAX_PACKET_SIZE + STREAM_LEN_SIZE, output);
    
    for (int i = 0; i < n; i++) {        
        if (isSliceExist[i]) {
            memcpy(pPacket.sliceMap[i].data, output + i * (MAX_PACKET_SIZE + STREAM_LEN_SIZE), 
                   MAX_PACKET_SIZE + STREAM_LEN_SIZE);
        } else {
            unsigned char* tmpChar = new unsigned char[MAX_PACKET_SIZE + STREAM_LEN_SIZE];
            memcpy(tmpChar, output + i * (MAX_PACKET_SIZE + STREAM_LEN_SIZE), MAX_PACKET_SIZE + STREAM_LEN_SIZE);
            SlicePacket slicePacket(tmpChar, MAX_PACKET_SIZE + STREAM_LEN_SIZE);
            pPacket.sliceMap[i] = slicePacket;
            uint16_t* tmpLen = (uint16_t*)(output + i * (MAX_PACKET_SIZE + STREAM_LEN_SIZE));
            uint16_t sliceLen = ntohs(*tmpLen);
            XMDLoggerWrapper::instance()->debug("do fec recover %d len =%d.", i, sliceLen);
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



