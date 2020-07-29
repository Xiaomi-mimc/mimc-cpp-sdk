#include "PacketBuilder.h"
#include "XMDLoggerWrapper.h"
#include "fec.h"
#include <sstream>

PacketBuilder::PacketBuilder(XMDCommonData* data, PacketDispatcher* dispatcher, WorkerCommonData* wData, int workerId) {
    partition_size_ = 0;
    commonData_ = data;
    workerCommonData_ = wData;
    isBigPacket_ = false;
    sendPacketPreMS_ = FLOW_CONTROL_SEND_SPEED;
    sendTime_ = 0;
    dispatcher_ = dispatcher;
    worker_id_ = workerId;
}

PacketBuilder::~PacketBuilder() {
}

void PacketBuilder::build(RTWorkerData* workerData, ConnInfo connInfo, StreamInfo sInfo) {
    if (NULL == workerData) {
        XMDLoggerWrapper::instance()->warn("invalid queue data.");
        return;
    }


    if (sInfo.sType == FEC_STREAM) {
        buildFecStreamPacket(workerData, connInfo, sInfo);
    } else if (sInfo.sType == ACK_STREAM || sInfo.sType == BATCH_ACK_STREAM) {
        buildAckStreamPacket(workerData, connInfo, sInfo);
    } else {
        XMDLoggerWrapper::instance()->warn("PacketBuilder invalid stream type(%d).", sInfo.sType);
    }
    
}

void PacketBuilder::buildFecStreamPacket(RTWorkerData* workerData, ConnInfo connInfo, StreamInfo sInfo) {
    dispatcher_->FECStreamDataSendComplete(workerData->connId, workerData->streamId, workerData->groupId, workerData->ctx);
    if (commonData_->isTimerQueueFull(0, workerData->len)) {
        XMDLoggerWrapper::instance()->warn("timer queue full, drop packet");
        return;
    }

    if (commonData_->isSocketSendQueueFull(workerData->len)) {
        XMDLoggerWrapper::instance()->warn("socket queue full %d drop fec packet", commonData_->getSocketSendQueueUsedSize());
        return;
    }
    
    int total_packet = workerData->len / MAX_PACKET_SIZE;
    if (workerData->len % MAX_PACKET_SIZE != 0) {
        total_packet++;
    }

    partition_size_ = total_packet / MAX_ORIGIN_PACKET_NUM_IN_PARTITION;
    if (total_packet % MAX_ORIGIN_PACKET_NUM_IN_PARTITION != 0) {
        partition_size_++;
    }

    int partition_id = 0;
    int slice_id = 0;
    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t groupId = workerData->groupId;
    XMDLoggerWrapper::instance()->debug("XMDTransceiver packetbuilder len=%d, total packet=%d,groupid=%d,conn=%ld,stream=%d", 
                                      workerData->len, total_packet, groupId, workerData->connId, workerData->streamId);

    uint8_t flags = 0;
    flags += ((workerData->dataPriority & 0x07) << 4);
    flags += (workerData->canBeDropped << 7);

    groupData_.construct(connInfo.ip, connInfo.port, partition_size_, workerData->connId, 
                         workerData->streamId, groupId, sInfo.isEncrypt, connInfo.sessionKey, flags);

    /*sendTime_ = 0;
    if ((int)workerData->len > FLOW_CONTROL_MAX_PACKET_SIZE) {
        isBigPacket_ = true;
    }*/
    
    int fecopn = 0;
    int fecpn = 0;
    if (partition_id < partition_size_ - 1) {
        fecopn = MAX_ORIGIN_PACKET_NUM_IN_PARTITION; 
    } else {
        fecopn = total_packet - partition_id * MAX_ORIGIN_PACKET_NUM_IN_PARTITION;
    }
    netStatus netstatus = workerCommonData_->getNetStatus(workerData->connId);
    fecpn = getRedundancyPacketNum(fecopn, netstatus.packetLossRate);

        
    while(right < workerData->len) {
        XMDFECStreamData streamData;
        streamData.connId = workerData->connId;
        streamData.streamId = workerData->streamId;
        streamData.groupId = groupId;
        streamData.PSize = partition_size_;
        streamData.PId = partition_id;
        streamData.sliceId = slice_id;
        streamData.packetId = workerCommonData_->getPakcetId(workerData->connId);
        streamData.FECOPN = fecopn; 
        streamData.FECPN = fecpn;
        streamData.flags = flags;
        
        if (right + MAX_PACKET_SIZE < workerData->len) {
            right += MAX_PACKET_SIZE;
        } else {
            right = workerData->len;
        }
        XMDPacketManager packetManager;

        uint16_t streamLen = right - left;
        uint16_t tmpLen = htons(streamLen);

        memcpy(groupData_.partitionVec[partition_id].origin_data + slice_id * (MAX_PACKET_SIZE + STREAM_LEN_SIZE), 
               &tmpLen, STREAM_LEN_SIZE);
        memcpy(groupData_.partitionVec[partition_id].origin_data + slice_id * (MAX_PACKET_SIZE + STREAM_LEN_SIZE) + STREAM_LEN_SIZE, 
               workerData->data + left, streamLen);

            
        packetManager.buildFECStreamData(streamData, 
                                         groupData_.partitionVec[partition_id].origin_data + slice_id * (MAX_PACKET_SIZE + STREAM_LEN_SIZE), 
                                         streamLen + STREAM_LEN_SIZE, 
                                         sInfo.isEncrypt,
                                         connInfo.sessionKey);

        XMDPacket *data = NULL;
        int len = 0;
        if (packetManager.encode(data, len) != 0) {
            return;
        }

        SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)data, len);
        commonData_->socketSendQueuePush(sendData); 

        /*if (isBigPacket_ && slice_id % FLOW_CONTROL_SEND_SPEED == 0) {
            sendTime_++;
        }
        
        if (sendTime_ == 0) {
            commonData_->socketSendQueuePush(sendData);  
        } else {
            workerCommonData_->startTimer(workerData->connId, WORKER_DATAGREAM_TIMEOUT, 
                                          sendTime_, (void*)sendData, sizeof(SendQueueData) + len);
        }*/
        
    
        left += MAX_PACKET_SIZE;
        slice_id++;
        if (slice_id >= streamData.FECOPN) {
            groupData_.partitionVec[partition_id].fec_opn = streamData.FECOPN;
            groupData_.partitionVec[partition_id].fec_pn = streamData.FECPN;
            slice_id = 0;
            partition_id++;

            if (partition_id < partition_size_ - 1) {
                fecopn = MAX_ORIGIN_PACKET_NUM_IN_PARTITION; 
            } else {
                fecopn = total_packet - partition_id * MAX_ORIGIN_PACKET_NUM_IN_PARTITION;
            }
            fecpn = getRedundancyPacketNum(fecopn, netstatus.packetLossRate);
        }
    }

    buildRedundancyPacket();
}

void PacketBuilder::buildRedundancyPacket() {
    
    for (int i = 0; i < groupData_.partitionSize; i++) {        
        Fec f(groupData_.partitionVec[i].fec_opn, groupData_.partitionVec[i].fec_pn);
        unsigned char fecRedundancyData[groupData_.partitionVec[i].fec_pn * (MAX_PACKET_SIZE + STREAM_LEN_SIZE)];
        f.fec_encode(groupData_.partitionVec[i].origin_data, MAX_PACKET_SIZE + STREAM_LEN_SIZE, fecRedundancyData);
        uint16_t sliceId = groupData_.partitionVec[i].fec_opn;
        for (int j = 0; j < groupData_.partitionVec[i].fec_pn; j++) {
            XMDFECStreamData fecStreamData;
            fecStreamData.connId = groupData_.connId;
            fecStreamData.streamId = groupData_.streamId;
            fecStreamData.groupId = groupData_.groupId;
            fecStreamData.PSize = groupData_.partitionSize;
            fecStreamData.PId = i;
            fecStreamData.packetId = workerCommonData_->getPakcetId(groupData_.connId);
            fecStreamData.sliceId = sliceId++;
            fecStreamData.FECOPN = groupData_.partitionVec[i].fec_opn;
            fecStreamData.FECPN = groupData_.partitionVec[i].fec_pn;
            fecStreamData.flags = groupData_.flags;
            XMDPacketManager fecPacketMan;
            
            fecPacketMan.buildFECStreamData(fecStreamData, fecRedundancyData + j * (MAX_PACKET_SIZE + STREAM_LEN_SIZE), 
                                            MAX_PACKET_SIZE + STREAM_LEN_SIZE, 
                                            groupData_.isEncrypt,
                                            groupData_.sessionKey);
            XMDPacket *data = NULL;
            int len = 0;
            if (fecPacketMan.encode(data, len) != 0) {
                return;
            }
            SendQueueData* sendData = new SendQueueData(groupData_.ip, groupData_.port, (unsigned char*)data, len);
            commonData_->socketSendQueuePush(sendData);  
            /*if (isBigPacket_ && sliceId % FLOW_CONTROL_SEND_SPEED == 0) {
                sendTime_++;
            }
            if (sendTime_ == 0) {
                commonData_->socketSendQueuePush(sendData);  
            } else {
                workerCommonData_->startTimer(groupData_.connId, WORKER_DATAGREAM_TIMEOUT, 
                                              sendTime_, (void*)sendData, sizeof(SendQueueData) + len);
            }*/
        }
    }
}

void PacketBuilder::buildAckStreamPacket(RTWorkerData* workerData, ConnInfo connInfo, StreamInfo sInfo) {
    if (workerData->resendCount != -1 && commonData_->isTimerQueueFull(0, workerData->len)) {
        XMDLoggerWrapper::instance()->warn("timer queue full drop ack packet");
        dispatcher_->streamDataSendFail(workerData->connId, workerData->streamId, workerData->groupId, workerData->ctx);
        return;
    }
    if (workerData->resendCount != -1 && commonData_->isSocketSendQueueFull(workerData->len)) {
        XMDLoggerWrapper::instance()->warn("socket queue full %d drop ack packet", commonData_->getSocketSendQueueUsedSize());
        dispatcher_->streamDataSendFail(workerData->connId, workerData->streamId, workerData->groupId, workerData->ctx);
        return;
    }
    /*float resendQueueUsage = commonData_->getResendQueueUsageRate();
    bool isOverUsage = ((resendQueueUsage > QUEUE_USAGERAGE_80) && (workerData->dataPriority == P2)) || 
                ((resendQueueUsage > QUEUE_USAGERAGE_90) && (workerData->dataPriority == P1));
    bool canBeDropped = (isOverUsage && workerData->canBeDropped) || (resendQueueUsage > QUEUE_USAGERAGE_FULL);
    if (canBeDropped) {
        dispatcher_->streamDataSendFail(workerData->connId, workerData->streamId, workerData->groupId, workerData->ctx);
    }*/
            
    int groupSize = workerData->len / MAX_PACKET_SIZE;
    if (workerData->len % MAX_PACKET_SIZE != 0) {
        groupSize++;
    }

    if ((workerData->resendCount >= 0 || workerData->resendCount == -1)) {
        std::stringstream ss;
        ss << workerData->connId << workerData->streamId << workerData->groupId;
        std::string callbackKey = ss.str();
        workerCommonData_->insertSendCallbackMap(callbackKey, groupSize);
    }

    int slice_id = 0;
    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t groupId = workerData->groupId;
    XMDLoggerWrapper::instance()->debug("XMDTransceiver packetbuilder len=%d, groupSize=%d,conn=%ld,stream=%d,groupid=%d", 
                                      workerData->len, groupSize, workerData->connId, workerData->streamId, groupId);

    uint8_t flags = 0;
    flags += ((workerData->dataPriority & 0x07) << 4);
    flags += (workerData->canBeDropped << 7);
    

    /*sendTime_ = 0;
    if ((int)workerData->len > FLOW_CONTROL_MAX_PACKET_SIZE) {
        isBigPacket_ = true;
    }*/
    
    while (right < workerData->len) {
        if (right + MAX_PACKET_SIZE < workerData->len) {
            right += MAX_PACKET_SIZE;
        } else {
            right = workerData->len;
        }

        XMDACKStreamData streamData;
        streamData.connId = workerData->connId;
        streamData.streamId = workerData->streamId;
        streamData.packetId = workerCommonData_->getPakcetId(workerData->connId);
        streamData.waitTimeMs = sInfo.callbackWaitTimeout;
        streamData.groupId = groupId;
        streamData.groupSize = groupSize;
        streamData.sliceId = slice_id;
        streamData.flags = flags;

        XMDPacketManager packetManager;
        packetManager.buildAckStreamData(streamData, workerData->data + left, right - left, 
                                         sInfo.isEncrypt, connInfo.sessionKey, (PacketType)sInfo.sType);
                                         
        XMDPacket *data = NULL;
        int len = 0;
        if (packetManager.encode(data, len) != 0) {
            return;
        }

        SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)data, len, 
                                                    streamData.connId, streamData.packetId, workerData->resendCount, 
                                                    workerCommonData_->getResendInterval(streamData.connId));

        /*if (isBigPacket_ && slice_id % FLOW_CONTROL_SEND_SPEED == 0) {
            sendTime_++;
        }*/
        
        slice_id++;
        left = right;

        commonData_->socketSendQueuePush(sendData); 

        /*if (sendTime_ == 0) {
            commonData_->socketSendQueuePush(sendData);  
        } else {
            workerCommonData_->startTimer(workerData->connId, WORKER_DATAGREAM_TIMEOUT, 
                                          sendTime_, (void*)sendData, sizeof(SendQueueData) + len);
        }*/

    
        std::stringstream ss_ack;
        ss_ack << streamData.connId << streamData.packetId;
        std::string ackpacketKey = ss_ack.str();
        workerCommonData_->insertIsPacketRecvAckMap(ackpacketKey, false);
        
    
        PacketCallbackInfo packetInfo;
        packetInfo.connId = streamData.connId;
        packetInfo.streamId = streamData.streamId;
        packetInfo.packetId = streamData.packetId;
        packetInfo.groupId = streamData.groupId;
        packetInfo.sliceId = streamData.sliceId;
        packetInfo.ctx = workerData->ctx;
        workerCommonData_->insertPacketCallbackInfoMap(ackpacketKey, packetInfo);

    }
}


int PacketBuilder::getRedundancyPacketNum(int fecopn, double packetLossRate) {
    return fecopn * 0.25;
    if (packetLossRate < 0.001) {
        if (fecopn < 10) {
            return 0;
        } else {
            return 1;
        }
    } 

    int result = 0;
    if (fecopn < 10) {
        result = fecopn * packetLossRate * 5;
        result++;
    } else if (fecopn < 20) {
        result = fecopn * packetLossRate * 4;
    } else if (fecopn < 30) {
        result = fecopn * packetLossRate * 3;
    } else {
        result = fecopn * packetLossRate * 2.5;
    }

    if (result > fecopn) {
        result = fecopn;
    } else if (result == 0) {
        result = 1;
    }
    
    return  result;
}





