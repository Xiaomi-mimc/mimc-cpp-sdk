#include "PacketDecoder.h"
#include "LoggerWrapper.h"
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
        uint64_t* tmpId = (uint64_t*)xmdPacket->data;
        uint64_t connId = ntohll(*tmpId);
    
        ConnInfo connInfo;
        if(commonData_->getConnInfo(connId, connInfo)){
            if ((packetType != CONN_BEGIN) && (ip != connInfo.ip || port != connInfo.port)) {
                LoggerWrapper::instance()->warn("conn(%ld) ip/port changed, origin ip=%d, port=%d, current ip=%d, port=%d",
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
                     LoggerWrapper::instance()->debug("drop repeated conn packet, id=%ld", connId);
                     return;
                 }
            }
        } else {
             if (packetType == FEC_STREAM_DATA || packetType == PING || packetType == PONG 
                 || packetType == ACK_STREAM_DATA || packetType == ACK) {
                 LoggerWrapper::instance()->warn("conn reset type=%d", packetType);
                 sendConnReset(ip, port, connId, CONN_NOT_EXIST);
                 return;
             }
        }
        
        switch (packetType)
        {
            case CONN_BEGIN: {
                handleNewConn(ip, port, xmdPacket->data, packetLen, xmdPacket->getEncryptSign());
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
                handleCloseStream(xmdPacket->data, packetLen);
                break;
            }
            case FEC_STREAM_DATA: {
                handleFECStreamData(connInfo, ip, port, xmdPacket->data, packetLen);
                break;
            }
            case ACK: {
                handleStreamDataAck(connInfo, ip, port, xmdPacket->data, packetLen);
                break;
            }
            case PING: {
                handlePing(connId, connInfo, ip, port, xmdPacket->data, packetLen);
                break;
            }
            case PONG: {
                handlePong(connInfo, ip, port, xmdPacket->data, packetLen);
                break;
            }
            case ACK_STREAM_DATA: {
                handleAckStreamData(connInfo, ip, port, xmdPacket->data, packetLen);
                break;
            }
            
            default: {
                LoggerWrapper::instance()->warn("unknow packet type:%d", packetType);
                break;
            }
        }
    } else {
        LoggerWrapper::instance()->warn("unknow xmdType:%d", xmdType);
    }
}

void PacketDecoder::handleNewConn(uint32_t ip, int port, unsigned char* data, int len, bool isEncrypt) {
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

    const int sessionKeyLen = 4;
    unsigned char keyIn[sessionKeyLen] = {0};
    unsigned char keyOut[SESSION_KEY_LEN];
    uint64_t tmpSessionKey = rand64();
    memcpy(keyIn, (char*)&tmpSessionKey, sessionKeyLen); 
    int encryptedKeyLen = RSA_public_encrypt(sessionKeyLen, (const unsigned char*)keyIn, keyOut, encryptRsa, RSA_PKCS1_PADDING);
    LoggerWrapper::instance()->debug("session key len=%d", encryptedKeyLen);

    
    ConnInfo connInfo;
    connInfo.ip = ip;
    connInfo.port = port;
    connInfo.timeout = conn->GetTimeout();
    connInfo.isConnected = true;
    connInfo.rsa = encryptRsa;
    connInfo.isEncrypt = isEncrypt;
    connInfo.created_stream_id = -1;
    connInfo.max_stream_id = 0;
    std::string tmpStr((char*)keyIn, sessionKeyLen);
    connInfo.sessionKey = tmpStr;
    commonData_->insertConn(conn->GetConnId(), connInfo);
    commonData_->startPing(conn->GetConnId());
    
    
    packetMan.buildConnResp(conn->GetConnId(), keyOut);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);
    LoggerWrapper::instance()->debug("conn id=%ld, %lu", conn->GetConnId(), conn->GetConnId());
    if (!isConnExist) {
        dispatcher_->handleNewConn(conn->GetConnId(), (char*)conn->GetData(), 
                                   len -  sizeof(XMDConnection) - conn->GetNLen() - conn->GetELen());
    }
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
    if (connInfo.isConnected) {
        LoggerWrapper::instance()->debug("conn(%ld) already connected, repeated conn resp", connResp->GetConnId());
        isconnected = true;
    }
    
    connInfo.isConnected = true;

    unsigned char keyIn[SESSION_KEY_LEN] = {0};
    unsigned char keyOut[SESSION_KEY_LEN];
    memcpy(keyIn, connResp->GetSessionkey(), SESSION_KEY_LEN);
    int keyLen = RSA_private_decrypt(SESSION_KEY_LEN, (const unsigned char*)keyIn, keyOut, connInfo.rsa, RSA_PKCS1_PADDING);    

    std::string tmpStr((char*)keyOut, keyLen);
    connInfo.sessionKey = tmpStr;
    LoggerWrapper::instance()->debug("session key len=%d, key=%s", keyLen, connInfo.sessionKey.c_str());
    commonData_->updateConn(connResp->GetConnId(), connInfo);
    commonData_->startPing(connResp->GetConnId());
    if (!isconnected) {
        dispatcher_->handleConnCreateSucc(connResp->GetConnId(), connInfo.ctx);
    }

    std::stringstream ss;
    ss << connResp->GetConnId() << 0;
    std::string connKey = ss.str();
    commonData_->updateResendMap(connKey, true);
}

void PacketDecoder::handleConnClose(unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDConnClose* connClose = packetMan.decodeConnClose(data, len);
    if (NULL == connClose) {
        return;
    }

    if (commonData_->deleteConn(connClose->GetConnId()) != 0) {
        LoggerWrapper::instance()->warn("Close conn invalid conn id:%ld", connClose->GetConnId());
        return;
    }
    dispatcher_->handleCloseConn(connClose->GetConnId(), CLOSE_NORMAL);
}

void PacketDecoder::handleFECStreamData(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDFECStreamData* streamData = packetMan.decodeFECStreamData(data, len, connInfo.isEncrypt, connInfo.sessionKey);
    if (NULL == streamData) {
        return;
    }

    uint64_t timestamp = current_ms();
    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamData->GetStreamId());
    
    if (it == connInfo.streamMap.end()) {
        dispatcher_->handleNewStream(streamData->GetConnId(), streamData->GetStreamId());
        StreamInfo streamInfo;
        streamInfo.timeout = streamData->GetTimeout();
        streamInfo.sType =  FEC_STREAM;
        //streamInfo.lastPacketTime = timestamp;
        commonData_->insertStream(streamData->GetConnId(), streamData->GetStreamId(), streamInfo);
    } 
    
    std::stringstream ss_conn, ss_conn_stream;
    ss_conn << streamData->GetConnId();
    std::string connKey = ss_conn.str();
    ss_conn_stream << streamData->GetConnId() << streamData->GetStreamId();
    std::string streamKey = ss_conn_stream.str();
    commonData_->updateLastPacketTime(connKey, timestamp);
    commonData_->updateLastPacketTime(streamKey, timestamp);
    

    //thread调度方式待定
    int threadId = (streamData->GetConnId() + streamData->GetStreamId()) % commonData_->getDecodeThreadSize();
    StreamData* sData = new StreamData(FEC_STREAM, len, data);
    commonData_->packetRecoverQueuePush(sData, threadId);

    return;
}


void PacketDecoder::handleCloseStream(unsigned char* data, int len) {
    uint64_t* tmpId = (uint64_t*)data;
    uint64_t connId = ntohll(*tmpId);

    ConnInfo connInfo;
    if(!commonData_->getConnInfo(connId, connInfo)){
        LoggerWrapper::instance()->warn("data invalid conn(%ld) not exist.", connId);
        return;
    }
    
    XMDPacketManager packetMan;
    XMDStreamClose* streamClose = packetMan.decodeStreamClose(data, len, connInfo.isEncrypt, connInfo.sessionKey);
    if (NULL == streamClose) {
        return;
    }

    if (!commonData_->isStreamExist(streamClose->GetConnId(), streamClose->GetStreamId())) {
        LoggerWrapper::instance()->warn("close stream packet invalid,stream does not exist." 
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
        LoggerWrapper::instance()->warn("reset conn invalid conn id:%ld", connReset->GetConnId());
        return;
    }
    
    dispatcher_->handleCloseConn(connReset->GetConnId(), CLOSE_CONN_RESET);

    std::stringstream ss;
    ss << connReset->GetConnId() << 0;
    std::string connKey = ss.str();
    commonData_->updateResendMap(connKey, true);
}

void PacketDecoder::handleStreamDataAck(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDStreamDataAck* ack = packetMan.decodeStreamDataAck(data, len, connInfo.isEncrypt, connInfo.sessionKey);
    if (NULL == ack) {
        return;
    }

    std::stringstream ss_ack;
    ss_ack << ack->GetConnId() << ack->GetAckedPacketId();
    std::string ackpacketKey = ss_ack.str();
    
    commonData_->updateResendMap(ackpacketKey, true);

    ackPacketInfo ackPacket;
    if (commonData_->getDeleteAckPacketInfo(ackpacketKey, ackPacket)) {
        std::stringstream ss_callback;
        ss_callback << ackPacket.connId << ackPacket.streamId << ackPacket.groupId;
        std::string callbackKey = ss_callback.str();
        if (commonData_->updateSendCallbackMap(callbackKey, ackPacket.sliceId) == 1) {
            dispatcher_->streamDataSendSucc(ackPacket.connId, ackPacket.streamId, ackPacket.groupId, ackPacket.ctx);
        }
    }
}


void PacketDecoder::handlePing(uint64_t connId, ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len) {
    uint64_t timestamp = current_ms();
    XMDPacketManager packetMan;
    XMDPing* ping = packetMan.decodeXMDPing(data, len, connInfo.isEncrypt, connInfo.sessionKey);
    if (NULL == ping) {
        return;
    }

    packetMan.buildXMDPong(connId, commonData_->getPakcetId(connId), ping->GetPacketId(), 
                           timestamp, connInfo.isEncrypt, connInfo.sessionKey);
    XMDPacket *xmddata = NULL;
    int packetLen = 0;
    if (packetMan.encode(xmddata, packetLen) != 0) {
        return;
    }

    SendQueueData* sendData = new SendQueueData(connInfo.ip, connInfo.port, (unsigned char*)xmddata, packetLen);
    commonData_->socketSendQueuePush(sendData);
}

void PacketDecoder::handlePong(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len) {
    uint64_t currentTime = current_ms();
    XMDPacketManager packetMan;
    XMDPong* pong = packetMan.decodeXMDPong(data, len, connInfo.isEncrypt, connInfo.sessionKey);
    if (NULL == pong) {
        return;
    }

    uint64_t ts = pong->timeStamp2 - pong->timeStamp1;

    commonData_->insertPong(pong->GetConnId(), pong->GetAckedPacketId(), currentTime, ts);
}

void PacketDecoder::handleAckStreamData(ConnInfo connInfo, uint32_t ip, int port, unsigned char* data, int len) {
    XMDPacketManager packetMan;
    XMDACKStreamData* streamData = packetMan.decodeAckStreamData(data, len, connInfo.isEncrypt, connInfo.sessionKey);
    if (streamData == NULL) {
        return;
    }
    uint64_t timestamp = current_ms();
    std::unordered_map<uint16_t, StreamInfo>::iterator it = connInfo.streamMap.find(streamData->GetStreamId());
    
    if (it == connInfo.streamMap.end()) {
        dispatcher_->handleNewStream(streamData->GetConnId(), streamData->GetStreamId());
        StreamInfo streamInfo;
        streamInfo.timeout = streamData->GetTimeout();
        streamInfo.sType = ACK_STREAM;
        //streamInfo.lastPacketTime = timestamp;
        commonData_->insertStream(streamData->GetConnId(), streamData->GetStreamId(), streamInfo);
    }

    std::stringstream ss_conn, ss_conn_stream;
    ss_conn << streamData->GetConnId();
    std::string connKey = ss_conn.str();
    ss_conn_stream << streamData->GetConnId() << streamData->GetStreamId();
    std::string streamKey = ss_conn_stream.str();
    commonData_->updateLastPacketTime(connKey, timestamp);
    commonData_->updateLastPacketTime(streamKey, timestamp);


    int threadId = (streamData->GetConnId() + streamData->GetStreamId()) % commonData_->getDecodeThreadSize();
    StreamData* sData = new StreamData(ACK_STREAM, len, data);
    commonData_->packetRecoverQueuePush(sData, threadId);

    uint64_t packetId = commonData_->getPakcetId(streamData->GetConnId());
    packetMan.buildStreamDataAck(streamData->GetConnId(), packetId, streamData->GetPacketId(), 
                                 connInfo.isEncrypt, connInfo.sessionKey);
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
    LoggerWrapper::instance()->warn("send conn(%ld) reset.type=%d", conn_id, type);
}

