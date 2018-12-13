#include "PacketDecoder.h"
#include "XMDLoggerWrapper.h"
#include <sstream>
#include <openssl/rsa.h>
#include <openssl/aes.h>


PacketDecoder::PacketDecoder(PacketDispatcher* dispatcher, XMDCommonData* commonData) {
    dispatcher_ = dispatcher;
    commonData_ = commonData;
}

PacketDecoder::~PacketDecoder() {
    dispatcher_ = NULL;
    commonData_ = NULL;
}


void PacketDecoder::decode(uint32_t ip, int port, char* data, int len) {
    XMDPacketManager packetMan;
    XMDPacket* xmdPacket = packetMan.decode(data, len);
    if (NULL == xmdPacket) {
        return;
    }
    XMDType xmdType = (XMDType)xmdPacket->getType();
    int packetLen = len -  sizeof(XMDPacket) - XMD_CRC_LEN;
    if (xmdType == DATAGRAM) {
        const int IP_STR_LEN = 32;
        char ipStr[IP_STR_LEN];
        memset(ipStr, 0, IP_STR_LEN);
        inet_ntop(AF_INET, &ip, ipStr, IP_STR_LEN);
        dispatcher_->handleRecvDatagram(ipStr, port, (char*)xmdPacket->data, packetLen);
    } else if (xmdType == STREAM) {
        PacketType packetType = (PacketType)xmdPacket->getPacketType();
        uint64_t connId = 0;
        if (packetType == CONN_BEGIN) {
            uint64_t* tmpId = (uint64_t*)(xmdPacket->data + 2);  //VERSION len
            connId = xmd_ntohll(*tmpId);
        } else {
            uint64_t* tmpId = (uint64_t*)xmdPacket->data;
            connId = xmd_ntohll(*tmpId);
        }
        
    
        ConnInfo connInfo;
        if(commonData_->getConnInfo(connId, connInfo)){
            if ((packetType != CONN_BEGIN) && (ip != connInfo.ip || port != connInfo.port)) {
                XMDLoggerWrapper::instance()->warn("conn(%ld) ip/port changed, origin ip=%d, port=%d, current ip=%d, port=%d",
                                             connId, connInfo.ip, connInfo.port, ip, port);
                
                commonData_->updateConnIpInfo(connId, ip, port);

                const int IP_STR_LEN = 32;
                char tmpip[IP_STR_LEN];
                memset(tmpip, 0, IP_STR_LEN);
                inet_ntop(AF_INET, &ip, tmpip, IP_STR_LEN);
                int ipLen = 0;
                for (; ipLen < 16; ipLen++) {
                    if (tmpip[ipLen] == '\0');
                }
                std::string ipStr(tmpip, ipLen);
                dispatcher_->handleConnIpChange(connId, ipStr, port);
            } else {
                 if (packetType == CONN_BEGIN) {
                     XMDLoggerWrapper::instance()->info("drop repeated conn packet, id=%ld", connId);
                     return;
                 }
            }
        } else {
             if (packetType == FEC_STREAM_DATA || packetType == PING || packetType == PONG 
                 || packetType == ACK_STREAM_DATA || packetType == ACK || packetType == STREAM_END) {
                 XMDLoggerWrapper::instance()->warn("conn reset type=%d", packetType);
                 sendConnReset(ip, port, connId, CONN_NOT_EXIST);
                 return;
             }
        }
        
        switch (packetType)
        {
            case CONN_BEGIN: {
                handleNewConn(ip, port, xmdPacket->data, packetLen);
                break;
            }
            case CONN_RESP_SUPPORT: {
                handleConnResp(ip, port, xmdPacket->data, packetLen);
                break;
            }
            case CONN_CLOSE: {
                handleConnClose(xmdPacket->data, packetLen);
                break;
            }
            case CONN_RESET: {
                handleConnReset(xmdPacket->data, packetLen);
                break;
            }
            case STREAM_END: {
                handleCloseStream(connInfo, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case FEC_STREAM_DATA: {
                handleFECStreamData(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case ACK: {
                handleStreamDataAck(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case PING: {
                handlePing(connId, connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case PONG: {
                handlePong(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            case ACK_STREAM_DATA: {
                handleAckStreamData(connInfo, ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
                break;
            }
            
            default: {
                XMDLoggerWrapper::instance()->warn("unknow packet type:%d", packetType);
                break;
            }
        }
    } else {
        XMDLoggerWrapper::instance()->warn("unknow xmdType:%d", xmdType);
    }
}

void PacketDecoder::handleNewConn(uint32_t ip, int port, unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDConnection* conn = packetMan.decodeNewConn(data, len);
    if (NULL == conn) {
        return;
    }

    bool isConnExist = false;
    if (commonData_->isConnExist(conn->GetConnId())) {
        ConnInfo conninfo;
        commonData_->getConnInfo(conn->GetConnId(), conninfo);
        if (!(conninfo.port == port && conninfo.ip == ip)) {
            sendConnReset(ip, port, conn->GetConnId(), CONN_ID_CONFLICT);
            return;
        } else {
            isConnExist = true;
        }
    }


    RSA* encryptRsa = RSA_new();
    encryptRsa->n = BN_bin2bn(conn->GetRSAN(), conn->GetNLen(), encryptRsa->n);
    encryptRsa->e = BN_bin2bn(conn->GetRSAE(), conn->GetELen(), encryptRsa->e);

    std::string sessionKey = std::to_string(rand64());
    unsigned char keyOut[SESSION_KEY_LEN];
    int encryptedKeyLen = RSA_public_encrypt(sessionKey.length(), (const unsigned char*)sessionKey.c_str(), keyOut, encryptRsa, RSA_PKCS1_PADDING);

    
    ConnInfo connInfo;
    connInfo.ip = ip;
    connInfo.port = port;
    connInfo.timeout = conn->GetTimeout();
    connInfo.connState = CONNECTED;
    connInfo.rsa = encryptRsa;
    connInfo.created_stream_id = -1;
    connInfo.max_stream_id = 0;
    connInfo.sessionKey = sessionKey;
    commonData_->insertConn(conn->GetConnId(), connInfo);
    
    
    packetMan.buildConnResp(conn->GetConnId(), keyOut, encryptedKeyLen);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);
    XMDLoggerWrapper::instance()->info("recv new connetion id=%ld, %lu", conn->GetConnId(), conn->GetConnId());
    if (!isConnExist) {
        dispatcher_->handleNewConn(conn->GetConnId(), (char*)conn->GetData(), 
                                   len -  sizeof(XMDConnection) - conn->GetNLen() - conn->GetELen());
    }

    commonData_->insertPacketLossInfoMap(conn->GetConnId());
    return;
}

void PacketDecoder::handleConnResp(uint32_t ip, int port, unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDConnResp* connResp = packetMan.decodeConnResp(data, len);
    if (NULL == connResp) {
        return;
    }

    ConnInfo connInfo;
    if(!commonData_->getConnInfo(connResp->GetConnId(), connInfo)){
        sendConnReset(ip, port, connResp->GetConnId(), CONN_NOT_EXIST);
        return;
    }
    bool isconnected = false;
    if (connInfo.connState == CONNECTED) {
        XMDLoggerWrapper::instance()->debug("conn(%ld) already connected, repeated conn resp", connResp->GetConnId());
        isconnected = true;
    }
    
    connInfo.connState = CONNECTED;

    unsigned char keyOut[SESSION_KEY_LEN];
    int encryptedKeyLen = len - sizeof(XMDConnResp);
    XMDLoggerWrapper::instance()->debug("encrypt session key len=%d", encryptedKeyLen);
    int keyLen = RSA_private_decrypt(encryptedKeyLen, connResp->GetSessionkey(), keyOut, connInfo.rsa, RSA_PKCS1_PADDING);    

    std::string tmpStr((char*)keyOut, keyLen);
    connInfo.sessionKey = tmpStr;
    XMDLoggerWrapper::instance()->debug("session key len=%d, key=%s", keyLen, connInfo.sessionKey.c_str());
    commonData_->updateConn(connResp->GetConnId(), connInfo);
    if (!isconnected) {
        dispatcher_->handleConnCreateSucc(connResp->GetConnId(), connInfo.ctx);
    }

    std::stringstream ss;
    ss << connResp->GetConnId() << 0;
    std::string connKey = ss.str();
    commonData_->updateIsPacketRecvAckMap(connKey, true);

    commonData_->insertPacketLossInfoMap(connResp->GetConnId());
}

void PacketDecoder::handleConnClose(unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDConnClose* connClose = packetMan.decodeConnClose(data, len);
    if (NULL == connClose) {
        return;
    }

    if (commonData_->deleteConn(connClose->GetConnId()) != 0) {
        XMDLoggerWrapper::instance()->warn("Close conn invalid conn id:%ld", connClose->GetConnId());
        return;
    }
    dispatcher_->handleCloseConn(connClose->GetConnId(), CLOSE_NORMAL);
}

void PacketDecoder::handleFECStreamData(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    XMDPacketManager packetMan;
    XMDFECStreamData* streamData = packetMan.decodeFECStreamData(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == streamData) {
        return;
    }

    XMDLoggerWrapper::instance()->debug("recv fec stream packet,connid=%ld,packetid=%ld,streamid=%d,groupid=%d,pid=%d,sliceid=%d", 
                                         streamData->GetConnId(), streamData->GetPacketId(), streamData->GetStreamId(),
                                         streamData->GetGroupId(), streamData->PId, streamData->GetSliceId());

    uint64_t timestamp = current_ms();
    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamData->GetStreamId());
    
    if (it == connInfo.streamMap.end()) {
        dispatcher_->handleNewStream(streamData->GetConnId(), streamData->GetStreamId());
        StreamInfo streamInfo;
        streamInfo.sType =  FEC_STREAM;
        streamInfo.callbackWaitTimeout = 0;
        streamInfo.isEncrypt = isEncrypt;
        commonData_->insertStream(streamData->GetConnId(), streamData->GetStreamId(), streamInfo);
    } 

    XMDLoggerWrapper::instance()->info("stream timeout=%d", streamData->GetTimeout());

    commonData_->updatePacketLossInfoMap(streamData->GetConnId(), streamData->GetPacketId());
    
    std::stringstream ss_conn;
    ss_conn << streamData->GetConnId();
    std::string connKey = ss_conn.str();
    commonData_->updateLastPacketTime(connKey, timestamp);

    //thread调度方式待定
    int threadId = (streamData->GetConnId() + streamData->GetStreamId()) % commonData_->getDecodeThreadSize();
    StreamData* sData = new StreamData(FEC_STREAM, len, data);
    commonData_->packetRecoverQueuePush(sData, threadId);

    return;
}


void PacketDecoder::handleCloseStream(ConnInfo connInfo, unsigned char* data, int len, bool isEncrypt) {    
    XMDPacketManager packetMan;
    XMDStreamClose* streamClose = packetMan.decodeStreamClose(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == streamClose) {
        return;
    }

    if (!commonData_->isStreamExist(streamClose->GetConnId(), streamClose->GetStreamId())) {
        XMDLoggerWrapper::instance()->warn("close stream packet invalid,stream does not exist." 
                                        "conn id:%ld, stream id:%d", 
                                    streamClose->GetConnId(), streamClose->GetStreamId());
        return;
    }
    dispatcher_->handleCloseStream(streamClose->GetConnId(), streamClose->GetStreamId());
}


void PacketDecoder::handleConnReset(unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDConnReset* connReset = packetMan.decodeConnReset(data, len);
    if (NULL == connReset) {
        return;
    }
    if (commonData_->deleteConn(connReset->GetConnId()) != 0) {
        XMDLoggerWrapper::instance()->warn("reset conn invalid conn id:%ld", connReset->GetConnId());
        return;
    }
    
    dispatcher_->handleCloseConn(connReset->GetConnId(), CLOSE_CONN_RESET);

    std::stringstream ss;
    ss << connReset->GetConnId() << 0;
    std::string connKey = ss.str();
    commonData_->updateIsPacketRecvAckMap(connKey, true);
}

void PacketDecoder::handleStreamDataAck(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    XMDPacketManager packetMan;
    XMDStreamDataAck* ack = packetMan.decodeStreamDataAck(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == ack) {
        return;
    }
    XMDLoggerWrapper::instance()->debug("recv ack packet,packetid=%ld, acked packet id=%ld,", ack->GetPacketId(), ack->GetAckedPacketId());

    commonData_->updatePacketLossInfoMap(ack->GetConnId(), ack->GetPacketId());

    std::stringstream ss_ack;
    ss_ack << ack->GetConnId() << ack->GetAckedPacketId();
    std::string ackpacketKey = ss_ack.str();
    
    commonData_->updateIsPacketRecvAckMap(ackpacketKey, true);

    PacketCallbackInfo packetCallbackInfo;
    if (commonData_->getDeletePacketCallbackInfo(ackpacketKey, packetCallbackInfo)) {
        std::stringstream ss_callback;
        ss_callback << packetCallbackInfo.connId << packetCallbackInfo.streamId << packetCallbackInfo.groupId;
        std::string callbackKey = ss_callback.str();
        if (commonData_->updateSendCallbackMap(callbackKey, packetCallbackInfo.sliceId) == 1) {
            dispatcher_->streamDataSendSucc(packetCallbackInfo.connId, packetCallbackInfo.streamId, packetCallbackInfo.groupId, packetCallbackInfo.ctx);
        }
    }
}


void PacketDecoder::handlePing(uint64_t connId, ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    uint64_t timestamp = current_ms();
    XMDPacketManager packetMan;
    XMDPing* ping = packetMan.decodeXMDPing(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == ping) {
        return;
    }

    PongThreadData pongthreadData;
    pongthreadData.connId = connId;
    pongthreadData.packetId = ping->GetPacketId();
    pongthreadData.ts = timestamp;

    commonData_->pongThreadQueuePush(pongthreadData);
    commonData_->updatePacketLossInfoMap(connId, ping->GetPacketId());
    commonData_->startPacketLossCalucate(connId);

    std::stringstream ss_conn;
    ss_conn << connId;
    std::string connKey = ss_conn.str();
    commonData_->updateLastPacketTime(connKey, timestamp);
}

void PacketDecoder::handlePong(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    uint64_t currentTime = current_ms();
    XMDPacketManager packetMan;
    XMDPong* pong = packetMan.decodeXMDPong(data, len, isEncrypt, connInfo.sessionKey);
    if (NULL == pong) {
        return;
    }

    commonData_->updatePacketLossInfoMap(pong->GetConnId(), pong->GetPacketId());

    netStatus status;
    if (pong->GetTotalPackets() != 0) {
        status.packetLossRate = 1 - float(pong->GetRecvPackets()) / float(pong->GetTotalPackets());
    } else {
        status.packetLossRate = 0;
    }
    PingPacket pingPakcet;
    if (commonData_->getPingPacket(pong->GetConnId(), pingPakcet)) {
        status.ttl = (currentTime - pingPakcet.sendTime - (pong->GetTimestamp2() - pong->GetTimestamp1())) / 2;
    }
    
    commonData_->updateNetStatus(pong->GetConnId(), status);

    XMDLoggerWrapper::instance()->debug("connection(%ld) recv pong, packet loss rate:%f, ttl:%d, total:%d,recved:%d,ts0:%ld,ts1:%ld,ts2:%ld,ts3:%ld", 
                                                         pong->GetConnId(), status.packetLossRate, status.ttl,
                                                         pong->GetTotalPackets(),pong->GetRecvPackets(),
                                                         pingPakcet.sendTime,pong->GetTimestamp1(),pong->GetTimestamp2(),currentTime);

    std::stringstream ss_conn;
    ss_conn << pong->GetConnId();
    std::string connKey = ss_conn.str();
    commonData_->updateLastPacketTime(connKey, currentTime);
}

void PacketDecoder::handleAckStreamData(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
    XMDPacketManager packetMan;
    XMDACKStreamData* streamData = packetMan.decodeAckStreamData(data, len, isEncrypt, connInfo.sessionKey);
    if (streamData == NULL) {
        return;
    }
    XMDLoggerWrapper::instance()->debug("recv ack stream packet,connid=%ld,packetid=%ld", 
                                         streamData->GetConnId(), streamData->GetPacketId());
                                         
    uint64_t timestamp = current_ms();
    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamData->GetStreamId());
    
    if (it == connInfo.streamMap.end()) {
        dispatcher_->handleNewStream(streamData->GetConnId(), streamData->GetStreamId());
        StreamInfo streamInfo;
        streamInfo.sType = ACK_STREAM;
        streamInfo.callbackWaitTimeout = streamData->GetWaitTime();
        streamInfo.isEncrypt = isEncrypt;
        commonData_->insertStream(streamData->GetConnId(), streamData->GetStreamId(), streamInfo);
    }

    commonData_->updatePacketLossInfoMap(streamData->GetConnId(), streamData->GetPacketId());

    std::stringstream ss_conn;
    ss_conn << streamData->GetConnId();
    std::string connKey = ss_conn.str();
    commonData_->updateLastPacketTime(connKey, timestamp);


    int threadId = (streamData->GetConnId() + streamData->GetStreamId()) % commonData_->getDecodeThreadSize();
    StreamData* sData = new StreamData(ACK_STREAM, len, data);
    commonData_->packetRecoverQueuePush(sData, threadId);

    uint64_t packetId = commonData_->getPakcetId(streamData->GetConnId());
    packetMan.buildStreamDataAck(streamData->GetConnId(), packetId, streamData->GetPacketId(), 
                                 isEncrypt, connInfo.sessionKey);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return;
    }
    SendQueueData* sendData = new SendQueueData(ip, port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);
}



void PacketDecoder::sendConnReset(uint32_t ip, int port, uint64_t conn_id, ConnResetType type) {
    XMDPacketManager packetMan;
    packetMan.buildConnReset(conn_id, type);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return;
    }
    SendQueueData* sendData = new SendQueueData(ip, port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);
    XMDLoggerWrapper::instance()->warn("send conn(%ld) reset.type=%d", conn_id, type);
}

