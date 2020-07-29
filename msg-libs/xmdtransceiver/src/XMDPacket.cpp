#include "XMDPacket.h"
#include "XMDLoggerWrapper.h"
#include "rc4_crypto.h"



int XMDPacketManager::buildConn(uint64_t connId, unsigned char* data, int len, int timeout, int nlen, unsigned char* n, int elen, unsigned char* e, bool isEncrypt) {
    int packetLen = sizeof(XMDPacket) + sizeof(XMDConnection) + len + XMD_CRC_LEN + nlen + elen;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, CONN_BEGIN, isEncrypt);
    
    XMDConnection* xmdConnection = (XMDConnection*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    xmdConnection->SetVersion(XMD_VERSION);
    xmdConnection->SetConnId(connId);
    xmdConnection->SetTimeout(timeout);
    xmdConnection->SetPublicKey(nlen, n, elen, e);
    xmdConnection->SetPayload(len, data);


    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}

int XMDPacketManager::buildConnReset(uint64_t connId, ConnResetType type) {
    int packetLen = sizeof(XMDPacket) + sizeof(XMDConnReset) + XMD_CRC_LEN;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, CONN_RESET, 0);

    XMDConnReset* connReset = (XMDConnReset*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    connReset->SetConnId(connId);
    connReset->SetErrType(type);
    
    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}

int XMDPacketManager::buildConnResp(uint64_t connId, unsigned char* key, int len) {
    int packetLen = sizeof(XMDPacket) + sizeof(XMDConnResp) + XMD_CRC_LEN + len;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, CONN_RESP_SUPPORT, 0);

    XMDConnResp* connResp = (XMDConnResp*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    connResp->SetConnId(connId);
    connResp->SetSessionKey(key, len);
    
    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}

int XMDPacketManager::buildConnClose(uint64_t connId) {
    int packetLen = sizeof(XMDPacket) + sizeof(XMDConnClose) + XMD_CRC_LEN;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, CONN_CLOSE, 0);

    XMDConnClose* connClose = (XMDConnClose*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    connClose->SetConnId(connId);    

    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}


int XMDPacketManager::buildDatagram(unsigned char* data, int len, unsigned char packetType) {
    len_ = sizeof(XMDPacket) + len + XMD_CRC_LEN;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(len_);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(DATAGRAM, packetType, 0);
    xmdPakcet_t->SetPayload(len, data);
    
    char* crc32 = (char*)xmdPakcet_t + len_ - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, len_ - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;

    return 0;
}

int XMDPacketManager::buildStreamClose(uint64_t connId, uint16_t streamId, bool isEncrypt, std::string key) {
    int packetLen = sizeof(XMDPacket) + sizeof(XMDStreamClose) + XMD_CRC_LEN;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, STREAM_END, isEncrypt);

    XMDStreamClose* streamClose = (XMDStreamClose*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    streamClose->SetConnId(connId);
    streamClose->SetStreamId(streamId);

    if (isEncrypt) {
        std::string tmpMSg((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
                           packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    
        std::string encryptedData;
        if (CryptoRC4Util::Encrypt(tmpMSg, encryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("buildStreamClose rc4 encrypt failed.");
            return -1;
        }
    
        memcpy((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
               encryptedData.c_str(), 
               packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    }

    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}

int XMDPacketManager::buildFECStreamData(XMDFECStreamData stData, unsigned char* data, int len, bool isEncrypt, std::string key) {
    int packetLen = sizeof(XMDPacket) + sizeof(XMDFECStreamData) + XMD_CRC_LEN + len;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, FEC_STREAM_DATA, isEncrypt);

    XMDFECStreamData* streamData = (XMDFECStreamData*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    streamData->SetConnId(stData.connId);
    streamData->SetPacketId(stData.packetId);
    streamData->SetStreamId(stData.streamId);
    streamData->SetTimeout(0x7FFF);
    streamData->SetGroupId(stData.groupId);
    streamData->PSize = stData.PSize; 
    streamData->PId = stData.PId;
    streamData->SetSliceId(stData.sliceId);
    streamData->SetFECOPN(stData.FECOPN);
    streamData->SetFECPN(stData.FECPN);
    streamData->SetFlags(stData.flags);
    streamData->SetPayload(data, len);

    if (isEncrypt) {
        std::string tmpMSg((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
                           packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    
        std::string encryptedData;
        if (CryptoRC4Util::Encrypt(tmpMSg, encryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("buildFECStreamData rc4 encrypt failed.");
            return -1;
        }
    
        memcpy((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
               encryptedData.c_str(), 
               packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    }

    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}

int XMDPacketManager::buildAckStreamData(XMDACKStreamData stData, unsigned char* data, int len, bool isEncrypt, std::string key, PacketType type) {
    int packetLen = sizeof(XMDPacket) + sizeof(XMDACKStreamData) + XMD_CRC_LEN + len;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, type, isEncrypt);

    XMDACKStreamData* streamData = (XMDACKStreamData*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    streamData->SetConnId(stData.connId);
    streamData->SetPacketId(stData.packetId);
    streamData->SetStreamId(stData.streamId);
    streamData->SetTimeout(0x7FFF);
    streamData->SetWaitTime(stData.waitTimeMs);
    streamData->SetGroupId(stData.groupId);
    streamData->SetGroupSize(stData.groupSize);
    streamData->SetSliceId(stData.sliceId);
    streamData->SetFlags(stData.flags);
    streamData->SetPayload(data, len);

    if (isEncrypt) {
        std::string tmpMSg((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
                           packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    
        std::string encryptedData;
        if (CryptoRC4Util::Encrypt(tmpMSg, encryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("buildAckStreamData rc4 encrypt failed.");
            return -1;
        }
    
        memcpy((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
               encryptedData.c_str(), 
               packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    }

    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}

int XMDPacketManager::buildStreamDataAck(uint64_t connId, uint64_t packetid, uint64_t ackPacketId, bool isEncrypt, std::string key) {
    int packetLen = sizeof(XMDPacket) + sizeof(XMDStreamDataAck) + XMD_CRC_LEN;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, ACK, isEncrypt);

    XMDStreamDataAck* ackPacket = (XMDStreamDataAck*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    ackPacket->SetConnId(connId);
    ackPacket->SetPacketId(packetid);
    ackPacket->SetAckedPacketId(ackPacketId);

    if (isEncrypt) {
        std::string tmpMSg((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
                           packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    
        std::string encryptedData;
        if (CryptoRC4Util::Encrypt(tmpMSg, encryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("buildAckStreamData rc4 encrypt failed.");
            return -1;
        }
    
        memcpy((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
               encryptedData.c_str(), 
               packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    }

    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}



int XMDPacketManager::buildXMDPing(uint64_t connId, bool isEncrypt, std::string key, uint64_t packetid) { 
    int packetLen = sizeof(XMDPacket) + sizeof(XMDPing) + XMD_CRC_LEN;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, PING, isEncrypt);

    XMDPing* ping = (XMDPing*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    ping->SetConnId(connId);
    //packetid = PACKET_ID;
    ping->SetPacketId(packetid);

    if (isEncrypt) {
        std::string tmpMSg((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
                           packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    
        std::string encryptedData;
        if (CryptoRC4Util::Encrypt(tmpMSg, encryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("buildXMDPing rc4 encrypt failed.");
            return -1;
        }
    
        memcpy((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
               encryptedData.c_str(), 
               packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    }

    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}

int XMDPacketManager::buildXMDTestRttPacket(uint64_t connId, bool isEncrypt, std::string key, uint64_t packetid) { 
    int packetLen = sizeof(XMDPacket) + sizeof(XMDPing) + XMD_CRC_LEN;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, TEST_RTT_PACKET, isEncrypt);

    XMDPing* ping = (XMDPing*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    ping->SetConnId(connId);
    //packetid = PACKET_ID;
    ping->SetPacketId(packetid);

    if (isEncrypt) {
        std::string tmpMSg((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
                           packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    
        std::string encryptedData;
        if (CryptoRC4Util::Encrypt(tmpMSg, encryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("buildXMDPing rc4 encrypt failed.");
            return -1;
        }
    
        memcpy((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
               encryptedData.c_str(), 
               packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    }

    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}


int XMDPacketManager::buildXMDPong(XMDPong pongData, bool isEncrypt, std::string key) {
    int packetLen = sizeof(XMDPacket) + sizeof(XMDPong) + XMD_CRC_LEN;
    XMDPacket* xmdPakcet_t = (XMDPacket*) ::operator new(packetLen);
    xmdPakcet_t->SetMagic();
    xmdPakcet_t->SetSign(STREAM, PONG, isEncrypt);

    XMDPong* pong = (XMDPong*)((char*)xmdPakcet_t + sizeof(XMDPacket));
    pong->SetConnId(pongData.connId);
    pong->SetPacketId(pongData.packetId);
    pong->SetAckedPacketId(pongData.ackedPacketId);
    pong->SetTimeStamp1(pongData.timeStamp1);
    pong->SetTimeStamp2(current_ms());
    pong->SetTotalPackets(pongData.totalPackets);
    pong->SetRecvPackets(pongData.recvPackets);

    if (isEncrypt) {
        std::string tmpMSg((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
                           packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    
        std::string encryptedData;
        if (CryptoRC4Util::Encrypt(tmpMSg, encryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("buildXMDPong rc4 encrypt failed.");
            return -1;
        }
    
        memcpy((char*)xmdPakcet_t + sizeof(XMDPacket) + CONN_LEN, 
               encryptedData.c_str(), 
               packetLen - sizeof(XMDPacket) - CONN_LEN - XMD_CRC_LEN);
    }

    char* crc32 = (char*)xmdPakcet_t + packetLen - XMD_CRC_LEN;
    uint32_t crc32_val = adler32(1L, (unsigned char*)xmdPakcet_t, packetLen - XMD_CRC_LEN);
    uint32_t real_crc32 = htonl(crc32_val);
    char* tmp_char_crc = (char*)&real_crc32;
    for (int i = 0; i < XMD_CRC_LEN; i++) {
        crc32[i] = tmp_char_crc[i];
    }

    xmdPacket_ = xmdPakcet_t;
    len_ = packetLen;
    
    return 0;
}




XMDConnection* XMDPacketManager::decodeNewConn(unsigned char* data, int len) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len < int(sizeof(XMDConnection))) {
        XMDLoggerWrapper::instance()->warn("connection packet decode faild,len=%d", len);
        return NULL;
    }
    XMDConnection* xmdConn = (XMDConnection*)data;
    return xmdConn;
}

XMDConnResp* XMDPacketManager::decodeConnResp(unsigned char* data, int len) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len < int(sizeof(XMDConnResp))) {
        XMDLoggerWrapper::instance()->warn("XMDConnResp packet decode faild,len=%d", len);
        return NULL;
    }

    XMDConnResp* connResp = (XMDConnResp*)data;
    return connResp;
}

XMDConnClose* XMDPacketManager::decodeConnClose(unsigned char* data, int len) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len != int(sizeof(XMDConnClose))) {
        XMDLoggerWrapper::instance()->warn("XMDConnClose packet decode faild,len=%d", len);
        return NULL;
    }

    XMDConnClose* connClose = (XMDConnClose*)data;
    return connClose;
}

XMDStreamClose* XMDPacketManager::decodeStreamClose(unsigned char* data, int len, bool isEncrypt, std::string key) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len != int(sizeof(XMDStreamClose))) {
        XMDLoggerWrapper::instance()->warn("XMDStreamClose packet decode faild,len=%d", len);
        return NULL;
    } 

    if (isEncrypt) {
        std::string tmpStr((char*)data + CONN_LEN, len - CONN_LEN);
        std::string decryptedData;
        if (CryptoRC4Util::Decrypt(tmpStr, decryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("decodeStreamClose RC4 Decrypt failed.");
            return NULL;
        }
        
        memcpy(data + CONN_LEN, decryptedData.c_str(), len - CONN_LEN);
    }

    XMDStreamClose* streamClose = (XMDStreamClose*)data;
    return streamClose;
}

XMDFECStreamData* XMDPacketManager::decodeFECStreamData(unsigned char* data, int len, bool isEncrypt, std::string key) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len < int(sizeof(XMDFECStreamData))) {
        XMDLoggerWrapper::instance()->warn("XMDFECStreamData packet decode faild,len=%d", len);
        return NULL;
    } 

    if (isEncrypt) {
        std::string tmpStr((char*)data + CONN_LEN, len - CONN_LEN);
        std::string decryptedData;
        if (CryptoRC4Util::Decrypt(tmpStr, decryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("decodeFECStreamData RC4 Decrypt failed.");
            return NULL;
        }
        memcpy(data + CONN_LEN, decryptedData.c_str(), len - CONN_LEN);
    }

    XMDFECStreamData* streamData = (XMDFECStreamData*)data;
    return streamData;
}

XMDACKStreamData* XMDPacketManager::decodeAckStreamData(unsigned char* data, int len, bool isEncrypt, std::string key) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len < int(sizeof(XMDACKStreamData))) {
        XMDLoggerWrapper::instance()->warn("XMDACKStreamData packet decode faild,len=%d", len);
        return NULL;
    } 

    if (isEncrypt) {
        std::string tmpStr((char*)data + CONN_LEN, len - CONN_LEN);
        std::string decryptedData;
        if (CryptoRC4Util::Decrypt(tmpStr, decryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("decodeAckStreamdata RC4 Decrypt failed.");
            return NULL;
        }
        memcpy(data + CONN_LEN, decryptedData.c_str(), len - CONN_LEN);
    }

    XMDACKStreamData* streamData = (XMDACKStreamData*)data;
    return streamData;
}

XMDStreamDataAck* XMDPacketManager::decodeStreamDataAck(unsigned char* data, int len, bool isEncrypt, std::string key) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len < int(sizeof(XMDStreamDataAck))) {
        XMDLoggerWrapper::instance()->warn("XMDStreamDataAck packet decode faild,len=%d", len);
        return NULL;
    } 

    if (isEncrypt) {
        std::string tmpStr((char*)data + CONN_LEN, len - CONN_LEN);
        std::string decryptedData;
        if (CryptoRC4Util::Decrypt(tmpStr, decryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("decodeStreamdataAck RC4 Decrypt failed.");
            return NULL;
        }
        memcpy(data + CONN_LEN, decryptedData.c_str(), len - CONN_LEN);
    }

    XMDStreamDataAck* ackPacket = (XMDStreamDataAck*)data;
    return ackPacket;
}



XMDConnReset* XMDPacketManager::decodeConnReset(unsigned char* data, int len) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len < int(sizeof(XMDConnReset))) {
        XMDLoggerWrapper::instance()->warn("XMDConnReset packet decode faild,len=%d", len);
        return NULL;
    } 

    XMDConnReset* connReset = (XMDConnReset*)data;
    return connReset;
}

XMDPing* XMDPacketManager::decodeXMDPing(unsigned char* data, int len, bool isEncrypt, std::string key) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len < int(sizeof(XMDPing))) {
        XMDLoggerWrapper::instance()->warn("XMDPing packet decode faild,len=%d", len);
        return NULL;
    } 

    if (isEncrypt) {
        std::string tmpStr((char*)data + CONN_LEN, len - CONN_LEN);
        std::string decryptedData;
        if (CryptoRC4Util::Decrypt(tmpStr, decryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("decodeXMDPing RC4 Decrypt failed.");
            return NULL;
        }
        memcpy(data + CONN_LEN, decryptedData.c_str(), len - CONN_LEN);
    }

    XMDPing* ping = (XMDPing*)data;
    return ping;
}

XMDPong* XMDPacketManager::decodeXMDPong(unsigned char* data, int len, bool isEncrypt, std::string key) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len < int(sizeof(XMDPong))) {
        XMDLoggerWrapper::instance()->warn("XMDPong packet decode faild,len=%d", len);
        return NULL;
    } 

    if (isEncrypt) {
        std::string tmpStr((char*)data + CONN_LEN, len - CONN_LEN);
        std::string decryptedData;
        if (CryptoRC4Util::Decrypt(tmpStr, decryptedData, key) != 0) {
            XMDLoggerWrapper::instance()->warn("decodeXMDPong RC4 Decrypt failed.");
            return NULL;
        }
        memcpy(data + CONN_LEN, decryptedData.c_str(), len - CONN_LEN);
    }

    XMDPong* pong = (XMDPong*)data;
    return pong;
}



XMDPacket* XMDPacketManager::decode(char* data, int len) {
    if (NULL == data) {
        XMDLoggerWrapper::instance()->warn("data invalid.");
        return NULL;
    }
    
    if (len < (int(sizeof(XMDPacket)) + XMD_CRC_LEN)) {
        XMDLoggerWrapper::instance()->warn("recevie packet err, len=%d", len);
        return NULL;
    }

    //uint32_t* crc32 = (uint32_t*)(data + len - XMD_CRC_LEN);
    uint32_t crc32 = 0;
    trans_uint32_t(crc32, (char*)(data + len - XMD_CRC_LEN));
    uint32_t origin_crc = ntohl(crc32);
    uint32_t current_crc = adler32(1L, (unsigned char*)data, len - XMD_CRC_LEN);

    if (origin_crc != current_crc) {
        XMDLoggerWrapper::instance()->warn("crc check faild, origin crc=%d, current crc=%d, len=%d", origin_crc, current_crc, len);
        return NULL;
    }

    return (XMDPacket*)data;
}


int XMDPacketManager::encode(XMDPacket* &data, int& len) {
    if (! xmdPacket_) {
        XMDLoggerWrapper::instance()->warn("decode failed ,xmdPacket_ is invalid.");
        return -1;
    }

    data = xmdPacket_;
    len = len_;
    return 0;
}

