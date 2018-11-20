#include "PacketBuilder.h"
#include "XMDLoggerWrapper.h"
#include "fec.h"
#include <sstream>

PacketBuilder::PacketBuilder(XMDCommonData* data, PacketDispatcher* dispatcher) {
    partition_size_ = 0;
    commonData_ = data;
    isBigPacket_ = false;
    sendPacketPreMS_ = FLOW_CONTROL_SEND_SPEED;
    sendTime_ = 0;
    dispatcher_ = dispatcher;
}

PacketBuilder::~PacketBuilder() {
}

void PacketBuilder::build(StreamQueueData* queueData) {
    if (NULL == queueData) {
        XMDLoggerWrapper::instance()->warn("invalid queue data.");
        return;
    }
    ConnInfo connInfo;
    if(!commonData_->getConnInfo(queueData->connId, connInfo)){
        XMDLoggerWrapper::instance()->warn("PacketBuilder conn(%ld) not exist.", queueData->connId);
        return;
    }
    
    StreamInfo sInfo;
    if(!commonData_->getStreamInfo(queueData->connId, queueData->streamId, sInfo)) {
        XMDLoggerWrapper::instance()->warn("PacketBuilder stream(%ld) not exist.", queueData->connId);
        return;
    }

    if (sInfo.sType == FEC_STREAM) {
        buildFecStreamPacket(queueData, connInfo, sInfo);
    } else if (sInfo.sType == ACK_STREAM) {
        buildAckStreamPacket(queueData, connInfo, sInfo);
    } else {
        XMDLoggerWrapper::instance()->warn("PacketBuilder invalid stream type(%d).", sInfo.sType);
    }
    
}

void PacketBuilder::buildFecStreamPacket(StreamQueueData* queueData, ConnInfo connInfo, StreamInfo sInfo) {
    dispatcher_->FECStreamDataSendComplete(queueData->connId, queueData->streamId, queueData->groupId, queueData->ctx);
    
    int total_packet = queueData->len / MAX_PACKET_SIZE;
    if (queueData->len % MAX_PACKET_SIZE != 0) {
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
    uint32_t groupId = queueData->groupId;
    XMDLoggerWrapper::instance()->debug("XMDTransceiver packetbuilder len=%d, total packet=%d,groupid=%d,conn=%ld,stream=%d", 
                                      queueData->len, total_packet, groupId, queueData->connId, queueData->streamId);

    uint8_t flags = 0;
    flags += ((queueData->dataPriority & 0x07) << 4);
    flags += (queueData->canBeDropped << 7);

    groupData_.construct(connInfo.ip, connInfo.port, partition_size_, queueData->connId, 
                         queueData->streamId, groupId, sInfo.timeout, sInfo.isEncrypt, connInfo.sessionKey, flags);

    int sendCount = 0;
    sendTime_ = current_ms() + 1;
    if (queueData->len > FLOW_CONTROL_MAX_PACKET_SIZE) {
        isBigPacket_ = true;
    }
    
    int fecopn = 0;
    int fecpn = 0;
    if ((total_packet - partition_id * MAX_ORIGIN_PACKET_NUM_IN_PARTITION) < MAX_ORIGIN_PACKET_NUM_IN_PARTITION) {
        fecopn = total_packet - partition_id * MAX_ORIGIN_PACKET_NUM_IN_PARTITION; 
    } else {
        fecopn = MAX_ORIGIN_PACKET_NUM_IN_PARTITION; 
    }
    netStatus netstatus = commonData_->getNetStatus(queueData->connId);
    fecpn = getRedundancyPacketNum(fecopn, netstatus.packetLossRate);

        
    while(right < queueData->len) {
        XMDFECStreamData streamData;
        streamData.connId = queueData->connId;
        streamData.streamId = queueData->streamId;
        streamData.groupId = groupId;
        streamData.PSize = partition_size_;
        streamData.PId = partition_id;
        streamData.sliceId = slice_id;
        streamData.timeout = sInfo.timeout;
        streamData.packetId = commonData_->getPakcetId(queueData->connId);
        streamData.FECOPN = fecopn; 
        streamData.FECPN = fecpn;
        streamData.flags = flags;
        
        if (right + MAX_PACKET_SIZE < queueData->len) {
            right += MAX_PACKET_SIZE;
        } else {
            right = queueData->len;
        }
        XMDPacketManager packetManager;

        uint16_t streamLen = right - left;
        uint16_t tmpLen = htons(streamLen);

        memcpy(groupData_.partitionVec[partition_id].origin_data + slice_id * (MAX_PACKET_SIZE + STREAM_LEN_SIZE), 
               &tmpLen, STREAM_LEN_SIZE);
        memcpy(groupData_.partitionVec[partition_id].origin_data + slice_id * (MAX_PACKET_SIZE + STREAM_LEN_SIZE) + STREAM_LEN_SIZE, 
               queueData->data + left, streamLen);

            
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
        if (isBigPacket_) {
            sendCount++;
            sendData->sendTime = sendTime_;
            if (sendCount >= sendPacketPreMS_) {
                sendCount = 0;
                sendTime_++;
            }
            commonData_->datagramQueuePush(sendData);
        } else {
            commonData_->socketSendQueuePush(sendData);
        }
        
        
    
        left += MAX_PACKET_SIZE;
        slice_id++;
        if (slice_id >= streamData.FECOPN) {
            groupData_.partitionVec[partition_id].fec_opn = streamData.FECOPN;
            groupData_.partitionVec[partition_id].fec_pn = streamData.FECPN;
            slice_id = 0;
            partition_id++;
            if ((total_packet - partition_id * MAX_ORIGIN_PACKET_NUM_IN_PARTITION) < MAX_ORIGIN_PACKET_NUM_IN_PARTITION) {
                fecopn = total_packet - partition_id * MAX_ORIGIN_PACKET_NUM_IN_PARTITION; 
            } else {
                fecopn = MAX_ORIGIN_PACKET_NUM_IN_PARTITION; 
            }
            fecpn = getRedundancyPacketNum(fecopn, netstatus.packetLossRate);
        }
    }

    buildRedundancyPacket();
}

void PacketBuilder::buildRedundancyPacket() {
    int sendCount = 0;
    
    for (int i = 0; i < groupData_.partitionSize; i++) {        
        Fec f(groupData_.partitionVec[i].fec_opn, groupData_.partitionVec[i].fec_pn);
        f.fec_encode(groupData_.partitionVec[i].origin_data, MAX_PACKET_SIZE + STREAM_LEN_SIZE, fecRedundancyData_);
        uint16_t sliceId = groupData_.partitionVec[i].fec_opn;
        uint64_t currtenTime = current_ms();
        if (sendTime_ < currtenTime) {
            sendTime_ = currtenTime;
        }
        for (int j = 0; j < groupData_.partitionVec[i].fec_pn; j++) {
            XMDFECStreamData fecStreamData;
            fecStreamData.connId = groupData_.connId;
            fecStreamData.streamId = groupData_.streamId;
            fecStreamData.groupId = groupData_.groupId;
            fecStreamData.PSize = groupData_.partitionSize;
            fecStreamData.PId = i;
            fecStreamData.timeout = groupData_.timeout;
            fecStreamData.packetId = commonData_->getPakcetId(groupData_.connId);
            fecStreamData.sliceId = sliceId++;
            fecStreamData.FECOPN = groupData_.partitionVec[i].fec_opn;
            fecStreamData.FECPN = groupData_.partitionVec[i].fec_pn;
            fecStreamData.flags = groupData_.flags;
            XMDPacketManager fecPacketMan;
            
            fecPacketMan.buildFECStreamData(fecStreamData, fecRedundancyData_ + j * (MAX_PACKET_SIZE + STREAM_LEN_SIZE), 
                                            MAX_PACKET_SIZE + STREAM_LEN_SIZE, 
                                            groupData_.isEncrypt,
                                            groupData_.sessionKey);
            XMDPacket *data = NULL;
            int len = 0;
            if (fecPacketMan.encode(data, len) != 0) {
                return;
            }
            SendQueueData* sendData = new SendQueueData(groupData_.ip, groupData_.port, (unsigned char*)data, len);
            if (isBigPacket_) {
                sendCount++;
                sendData->sendTime = sendTime_;
                if (sendCount >= sendPacketPreMS_) {
                    sendCount = 0;
                    sendTime_++;
                }
                
                //std::cout<<"fec sendtime="<<sendData->sendTime<<std::endl;
                commonData_->datagramQueuePush(sendData);
            } else {
                commonData_->socketSendQueuePush(sendData);
            }
            
        }
    }
}

void PacketBuilder::buildAckStreamPacket(StreamQueueData* queueData, ConnInfo connInfo, StreamInfo sInfo) {
    float resendQueueUsage = commonData_->getResendQueueUsageRate();
    bool isOverUsage = ((resendQueueUsage > QUEUE_USAGERAGE_80) && (queueData->dataPriority == P2)) || 
                ((resendQueueUsage > QUEUE_USAGERAGE_90) && (queueData->dataPriority == P1));
    bool canBeDropped = (isOverUsage && queueData->canBeDropped) || (resendQueueUsage > QUEUE_USAGERAGE_FULL);
    if (canBeDropped) {
        dispatcher_->streamDataSendFail(queueData->connId, queueData->streamId, queueData->groupId, queueData->ctx);
    }
            
    int groupSize = queueData->len / MAX_PACKET_SIZE;
    if (queueData->len % MAX_PACKET_SIZE != 0) {
        groupSize++;
    }

    if ((queueData->resendCount >= 0 || queueData->resendCount == -1) && !canBeDropped) {
        std::stringstream ss;
        ss << queueData->connId << queueData->streamId << queueData->groupId;
        std::string callbackKey = ss.str();
        commonData_->insertSendCallbackMap(callbackKey, groupSize);
    }

    int slice_id = 0;
    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t groupId = queueData->groupId;
    XMDLoggerWrapper::instance()->debug("XMDTransceiver packetbuilder len=%d, groupSize=%d,conn=%ld,stream=%d,groupid=%d", 
                                      queueData->len, groupSize, queueData->connId, queueData->streamId, groupId);

    uint8_t flags = 0;
    flags += ((queueData->dataPriority & 0x07) << 4);
    flags += (queueData->canBeDropped << 7);
    
    int sendCount = 0;
    sendTime_ = current_ms() + 1;
    if (queueData->len > FLOW_CONTROL_MAX_PACKET_SIZE) {
        isBigPacket_ = true;
    }
    
    while (right < queueData->len) {
        if (right + MAX_PACKET_SIZE < queueData->len) {
            right += MAX_PACKET_SIZE;
        } else {
            right = queueData->len;
        }

        XMDACKStreamData streamData;
        streamData.connId = queueData->connId;
        streamData.streamId = queueData->streamId;
        streamData.packetId = commonData_->getPakcetId(queueData->connId);
        streamData.timeout = sInfo.timeout;
        streamData.waitTimeMs = sInfo.callbackWaitTimeout;
        streamData.groupId = groupId;
        streamData.groupSize = groupSize;
        streamData.sliceId = slice_id;
        streamData.flags = flags;

        XMDPacketManager packetManager;
        packetManager.buildAckStreamData(streamData, queueData->data + left, right - left, 
                                         sInfo.isEncrypt, connInfo.sessionKey);
                                         
        XMDPacket *data = NULL;
        int len = 0;
        if (packetManager.encode(data, len) != 0) {
            return;
        }

        SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)data, len);
        if (isBigPacket_) {
            sendCount++;
            sendData->sendTime = sendTime_;
            if (sendCount >= sendPacketPreMS_) {
                sendCount = 0;
                sendTime_++;
            }
            commonData_->datagramQueuePush(sendData);
        } else {
            commonData_->socketSendQueuePush(sendData);
            sendData->sendTime = current_ms();
        }
        
        slice_id++;
        left = right;


        if (canBeDropped) {
            XMDLoggerWrapper::instance()->debug("resend queue full, do not resend this packet.conn:%ld,packetid%ld",
                                                 streamData.connId, streamData.packetId);
            return;
        }
        ResendData* resendData = new ResendData((unsigned char*)data, len);
        resendData->connId = streamData.connId;
        resendData->packetId = streamData.packetId;
        resendData->ip = connInfo.ip;
        resendData->port = connInfo.port;
        resendData->reSendTime = sendData->sendTime + getResendTimeInterval(1);
        resendData->reSendCount= queueData->resendCount;
        commonData_->resendQueuePush(resendData);
    
        std::stringstream ss_ack;
        ss_ack << streamData.connId << streamData.packetId;
        std::string ackpacketKey = ss_ack.str();
        commonData_->insertIsPacketRecvAckMap(ackpacketKey, false);
        
    
        PacketCallbackInfo packetInfo;
        packetInfo.connId = streamData.connId;
        packetInfo.streamId = streamData.streamId;
        packetInfo.packetId = streamData.packetId;
        packetInfo.groupId = streamData.groupId;
        packetInfo.sliceId = streamData.sliceId;
        packetInfo.ctx = queueData->ctx;
        commonData_->insertPacketCallbackInfoMap(ackpacketKey, packetInfo);

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



