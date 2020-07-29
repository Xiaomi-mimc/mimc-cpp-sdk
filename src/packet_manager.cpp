#include "mimc/packet_manager.h"
#include "mimc/user.h"
#include "mimc/constant.h"
#include "mimc/p2p_callsession.h"
#include "mimc/rts_send_data.h"
#include "mimc/rts_send_signal.h"
#include <string.h>
#include "crypto/rc4_crypto.h"
#include <zlib/zlib.h>
#include <algorithm>
#include "mimc/p2p_processor.h"
#include "mimc/mutex_lock.h"


/* ims::ClientHeader * PacketManager::createClientHeader(const User * user, std::string cmd, int cipher) {
    ims::ClientHeader * header = new ims::ClientHeader();
    header->set_cmd(cmd);
    header->set_server(MIMC_SERVER);
    header->set_uuid(user->getUuid());
    header->set_chid(user->getChid());
    header->set_cipher(cipher);
    header->set_resource(user->getResource());
    header->set_id(createPacketId());
    header->set_dir_flag(ims::ClientHeader::CS_REQ);
    return header;
}
 */
void PacketManager::createClientHeader(ims::ClientHeader& header ,const User * user, std::string cmd, int cipher) {
    //ims::ClientHeader * header = new ims::ClientHeader();
    header.set_cmd(cmd);
    header.set_server(MIMC_SERVER);
    header.set_uuid(user->getUuid());
    header.set_chid(user->getChid());
    header.set_cipher(cipher);
    header.set_resource(user->getResource());
    header.set_id(createPacketId());
    header.set_dir_flag(ims::ClientHeader::CS_REQ);
}

int PacketManager::encodeConnectionPacket(unsigned char * &packet, const Connection * connection) {
    const User * user = connection->getUser();
    ims::ClientHeader  header;
    createClientHeader(header,user, BODY_CLIENTHEADER_CMD_CONN, BODY_CLIENTHEADER_CIPHER_NONE);
    ims::XMMsgConn * payload = new ims::XMMsgConn();
    payload->set_version(BODY_PAYLOAD_CONN_VERSION);
    payload->set_model(connection->getModel());
    payload->set_os(connection->getOs());
    payload->set_udid(connection->getUdid());
    payload->set_sdk(BODY_PAYLOAD_CONN_SDK);
    payload->set_connpt(connection->getConnpt());
    payload->set_locale(connection->getLocale());
    payload->set_andver(connection->getAndver());
    return encodePacket(packet, header, payload);
}

int PacketManager::encodeBindPacket(unsigned char * &packet, const Connection * connection) {
    const User * user = connection->getUser();
    ims::ClientHeader  header;
    createClientHeader(header,user, BODY_CLIENTHEADER_CMD_BIND, BODY_CLIENTHEADER_CIPHER_NONE);
    ims::XMMsgBind * payload = new ims::XMMsgBind();
    payload->set_token(user->getToken());
    payload->set_kick(NO_KICK);
    payload->set_method(METHOD);
    payload->set_client_attrs(user->getClientAttrs());
    payload->set_cloud_attrs(user->getCloudAttrs());
    payload->set_sig(generateSig(header, payload, connection));
    std::string body_key = connection->getBodyKey();
    return encodePacket(packet, header, payload, body_key);
}

int PacketManager::encodeSecMsgPacket(unsigned char * &packet, const Connection * connection, const google::protobuf::MessageLite * message) {
    const User * user = connection->getUser();
    ims::ClientHeader  header;
    createClientHeader(header,user, BODY_CLIENTHEADER_CMD_SECMSG, BODY_CLIENTHEADER_CIPHER_RC4);
    std::string body_key = connection->getBodyKey();
    std::string payload_key = generatePayloadKey(user->getSecurityKey(), header.id());
    return encodePacket(packet, header, message, body_key, payload_key);
}

int PacketManager::encodeUnBindPacket(unsigned char * &packet, const Connection * connection) {
    const User * user = connection->getUser();
    ims::ClientHeader header;
    createClientHeader(header,user, BODY_CLIENTHEADER_CMD_UNBIND, BODY_CLIENTHEADER_CIPHER_NONE);
    std::string body_key = connection->getBodyKey();
    return encodePacket(packet, header, NULL, body_key);
}

int PacketManager::encodePingPacket(unsigned char * &packet, const Connection * connection) {
    ims::ClientHeader header;
    return encodePacket(packet, header, NULL);
}

int PacketManager::encodePacket(unsigned char * &packet, const ims::ClientHeader& header, const google::protobuf::MessageLite * message, const std::string &body_key, const std::string &payload_key) {
    short body_head_size = 0;
    int body_message_size = 0;
    int body_size = 0;
    std::string raw_header;
    std::string final_message;

    if (header.has_cmd()) {
        body_head_size = header.ByteSize();
        raw_header = header.SerializeAsString();

        if (message != NULL) {
            body_message_size = message->ByteSize();

            std::string raw_message;
            raw_message = message->SerializeAsString();
            if (header.cmd() != BODY_CLIENTHEADER_CMD_SECMSG) {
                final_message = raw_message;
            } else {
                std::string raw = raw_message;
                std::string message_cipher;
                if (ccb::CryptoRC4Util::Encrypt(raw, message_cipher, payload_key) != 0) {
                    XMDLoggerWrapper::instance()->error("encodePacket failed, body_message encrypt failed");
                    if(message){
                        delete message;
                        message = NULL;
                    }
                    
                    return -1;
                }
                final_message = message_cipher;
            }
        }
    }


    std::string raw_body(BODY_HEADER_LENGTH,' ');
    std::string final_body;

    if (body_head_size > 0) {
        body_size = BODY_HEADER_LENGTH + body_head_size + body_message_size;
        short2charString(BODY_HEADER_PAYLOADTYPE, raw_body, BODY_HEADER_PAYLOADTYPE_OFFSET);
        short2charString(body_head_size, raw_body, BODY_HEADER_HEADERLEN_OFFSET);
        int2charString(body_message_size, raw_body, BODY_HEADER_PAYLOADLEN_OFFSET);
        raw_body += raw_header;

        if (body_message_size > 0) {
            raw_body += final_message;
        }

        if (header.cmd() == BODY_CLIENTHEADER_CMD_CONN) {
            final_body = raw_body;
        } else {
            std::string raw = raw_body;
            std::string body_cipher;
            if (ccb::CryptoRC4Util::Encrypt(raw, body_cipher, body_key) != 0) {
                XMDLoggerWrapper::instance()->error("encodePacket failed, body encrypt failed");
                if(message){
                    delete message;
                    message = NULL;
                }

                return -1;
            }
            final_body = body_cipher;
        }
    }

    int packet_size = HEADER_LENGTH + body_size + BODY_CRC_LEN;
    packet = new unsigned char[packet_size];
    memset(packet, 0, packet_size);
    short2char(HEADER_MAGIC, packet, HEADER_MAGIC_OFFSET);
    short2char(HEADER_VERSION, packet, HEADER_VERSION_OFFSET);
    int2char(body_size, packet, HEADER_BODYLEN_OFFSET);
    memmove(packet + HEADER_LENGTH, final_body.c_str(), final_body.size());

    uint32_t crc = compute_crc32(packet, packet_size - BODY_CRC_LEN);
    int2char(crc, packet, packet_size - BODY_CRC_LEN);

    delete message;
    message = NULL;

    return packet_size;
}

int PacketManager::decodePacketAndHandle(unsigned char * packet, Connection * connection) {
    int16_t magic = char2short(packet, HEADER_MAGIC_OFFSET);
    int16_t version = char2short(packet, HEADER_VERSION_OFFSET);
    if (magic != HEADER_MAGIC || version != HEADER_VERSION) {
        return -1;
    }
    int body_size = char2int(packet, HEADER_BODYLEN_OFFSET);
    int crc_offset = HEADER_LENGTH + body_size;
    uint32_t compute_crc = compute_crc32(packet, crc_offset);
    uint32_t crc = char2int(packet, crc_offset);

    if (compute_crc != crc) {
        XMDLoggerWrapper::instance()->error("decodePacket failed, compute_crc != crc");
        return -1;
    }

    if (body_size == 0) {
        XMDLoggerWrapper::instance()->debug("body_size == 0(ping-ack from fe)");
        return 0;
    }

    if (connection->getBodyKey() != "") {
        std::string raw((char *)(packet + HEADER_LENGTH), body_size);
        std::string body_plain;
        if (ccb::CryptoRC4Util::Decrypt(raw, body_plain, connection->getBodyKey()) != 0) {
            XMDLoggerWrapper::instance()->error("decodePacket failed, body decrypt failed");
            return -1;
        }
        memmove(packet + HEADER_LENGTH, body_plain.c_str(), body_size);
    }

    short payload_type = char2short(packet, HEADER_LENGTH + BODY_HEADER_PAYLOADTYPE_OFFSET);
    short body_head_size = char2short(packet, HEADER_LENGTH + BODY_HEADER_HEADERLEN_OFFSET);
    int body_message_size = char2int(packet, HEADER_LENGTH + BODY_HEADER_PAYLOADLEN_OFFSET);

    ims::ClientHeader header;
    if (!header.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH, body_head_size)) {
        XMDLoggerWrapper::instance()->error("decodePacket failed, header parse failed");
        return -1;
    }

    User * user = connection->getUser();
    std::string cmd = header.cmd();

    XMDLoggerWrapper::instance()->debug("cmd = %s",cmd.c_str());
    if (cmd == BODY_CLIENTHEADER_CMD_CONN) {
        ims::XMMsgConnResp resp;
        if (!resp.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_message_size)) {
            XMDLoggerWrapper::instance()->error("XMMsgConnResp parse failed");
            return -1;
        }
        std::string challenge = resp.challenge();
        connection->setChallengeAndBodyKey(challenge);
        connection->setState(HANDSHAKE_CONNECTED);
        int64_t ts = Utils::currentTimeMillis();
        XMDLoggerWrapper::instance()->info("fe conn timecost end time=%llu", ts);
        XMDLoggerWrapper::instance()->info("connresp receive succeed, connection build succeed, user is %s", user->getAppAccount().c_str());
    }
    else if (cmd == BODY_CLIENTHEADER_CMD_BIND) {
        ims::XMMsgBindResp resp;
        if (!resp.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_message_size)) {
            XMDLoggerWrapper::instance()->error("XMMsgBindResp parse failed");
            return -1;
        }
        OnlineStatus onlineStatus = resp.result() ? Online : Offline;
        XMDLoggerWrapper::instance()->info("bindresp receive succeed, onlineStatus is %d, user is %s, uuid is %lld", onlineStatus, user->getAppAccount().c_str(), user->getUuid());

        int64_t ts = Utils::currentTimeMillis();
        XMDLoggerWrapper::instance()->info("fe bind timecost end time=%llu", ts);
        user->setOnlineStatus(onlineStatus);
        if (user->getStatusHandler() != NULL) {
            user->getStatusHandler()->statusChange(onlineStatus, resp.error_type(), resp.error_reason(), resp.error_desc());
        }
        if (resp.error_type() == "token-expired" || resp.error_reason() == "invalid-token") {
            XMDLoggerWrapper::instance()->warn("bindresp(kick) receive succeed, error_type is token-expired, user is %s", user->getAppAccount().c_str());
            user->setTokenInvalid(true);
        }
    }
    else if (cmd == BODY_CLIENTHEADER_CMD_KICK) {
        ims::XMMsgBindResp resp;
        if (!resp.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_message_size)) {
            XMDLoggerWrapper::instance()->error("KICK XMMsgBindResp parse failed");
            return -1;
        }
        OnlineStatus onlineStatus = resp.result() ? Online : Offline;
        XMDLoggerWrapper::instance()->info("bindresp(kick) receive succeed, onlineStatus is %d, user is %s, uuid is %lld", onlineStatus, user->getAppAccount().c_str(), user->getUuid());

        user->setOnlineStatus(onlineStatus);
        if (user->getStatusHandler() != NULL) {
            user->getStatusHandler()->statusChange(onlineStatus, resp.error_type(), resp.error_reason(), resp.error_desc());
        }
        if (resp.error_type() == "token-expired" || resp.error_reason() == "invalid-token") {
            XMDLoggerWrapper::instance()->warn("bindresp(kick) receive succeed, error_type is token-expired, user is %s", user->getAppAccount().c_str());
            user->setTokenInvalid(true);
        }
    }
    else if (cmd == BODY_CLIENTHEADER_CMD_SECMSG) {
        if(header.chid() != MIMC_CHID || header.uuid() != user->getUuid()){
             XMDLoggerWrapper::instance()->error("cmd = SECMSG,head chid = %d header.uuid() is %lld",header.chid(),header.uuid());
        }
        
        std::string raw((char *)(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size), body_message_size);
        std::string body_plain;
        std::string payload_key = generatePayloadKey(user->getSecurityKey(), header.id());
        if (ccb::CryptoRC4Util::Decrypt(raw, body_plain, payload_key) != 0) {
            XMDLoggerWrapper::instance()->error("decodePacket failed, body_message decrypt failed");
            return -1;
        }
        memmove(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_plain.c_str(), body_message_size);
        mimc::MIMCPacket mimcPacket;
        if (!mimcPacket.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_message_size)) {
            XMDLoggerWrapper::instance()->error("decodePacket failed, mimcPacket parse failed");
            return -1;
        }

        XMDLoggerWrapper::instance()->debug("mimcPacket type is %d",mimcPacket.type());
        if (mimcPacket.type() == mimc::PACKET_ACK) {
            mimc::MIMCPacketAck mimcPacketAck;
            if (!mimcPacketAck.ParseFromString(mimcPacket.payload())) {
                XMDLoggerWrapper::instance()->error("decodePacket failed, mimcPacketAck parse failed");
                return -1;
            }
            if (user->getMessageHandler() != NULL) {
                MIMCServerAck serverAck(mimcPacketAck.packetid(), mimcPacketAck.sequence(), mimcPacketAck.timestamp(),
                    mimcPacketAck.code(), mimcPacketAck.errormsg(), mimcPacketAck.convindex());
                user->getMessageHandler()->handleServerAck(serverAck);
            }
            MIMCMutexLock mutexLock(&packetsTimeoutMutex);
            (this->packetsWaitToTimeout).erase(mimcPacketAck.packetid());
            (this->groupPacketWaitToTimeout).erase(mimcPacketAck.packetid());

        } else if(mimcPacket.type() == mimc::ONLINE_MESSAGE){
            XMDLoggerWrapper::instance()->info("In ONLINE_MESSAGE, user is %s", user->getAppAccount().c_str());
            
            mimc::MIMCP2PMessage p2pMessage;
            if(!p2pMessage.ParseFromString(mimcPacket.payload())){
                XMDLoggerWrapper::instance()->error("decodePacket failed, MIMCP2PMessage parse failed");
                return -1;
            }

            MIMCMessage mimcMessage(mimcPacket.packetid(),mimcPacket.sequence(),p2pMessage.from().appaccount(),p2pMessage.from().resource(),
            p2pMessage.to().appaccount(),p2pMessage.to().resource(),p2pMessage.payload(),p2pMessage.biztype(),mimcPacket.timestamp(), mimcPacket.convindex());

            if (user->getMessageHandler() != NULL)
            {
                user->getMessageHandler()->handleOnlineMessage(mimcMessage);
            }
        }
        else if (mimcPacket.type() == mimc::COMPOUND) {
            mimc::MIMCPacketList mimcPacketList;
            if (!mimcPacketList.ParseFromString(mimcPacket.payload()))
            {
                XMDLoggerWrapper::instance()->error("decodePacket failed, mimcPacketList parse failed");
                return -1;
            }

            XMDLoggerWrapper::instance()->info("In COMPOUND, user is %s", user->getAppAccount().c_str());
            bool isSendCompoundSequenceAck = true;
            if (mimcPacketList.status() == mimc::STATUS_EXCEED_COUNT_LIMIT)
            {
                XMDLoggerWrapper::instance()->debug("decodePacketAndHandle, status:STATUS_EXCEED_COUNT_LIMIT");
                if (isSerialPullNotification) {
                    sendCompoundSequenceAck(mimcPacketList, user);
                    XMDLoggerWrapper::instance()->warn("decodePacketAndHandle, ignore serial pull notification maxSequence:%ld", mimcPacketList.maxsequence());
                    return 0;
                }

                if (user->getMessageHandler() != NULL) {
                    if (!user->getMessageHandler()->onPullNotification()) {
                        XMDLoggerWrapper::instance()->warn("decodePacketAndHandle, onPullNotification callback return false");
                        isSendCompoundSequenceAck = false;
                    }
                }

                if (isSendCompoundSequenceAck) {
                    sendCompoundSequenceAck(mimcPacketList, user);
                    isSerialPullNotification = true;
                    XMDLoggerWrapper::instance()->debug("decodePacketAndHandle, sendCompoundSequenceAck maxSequence:%ld", mimcPacketList.maxsequence());
                }
            } else if (mimcPacketList.status() == mimc::STATUS_NORMAL) {
                isSerialPullNotification = false;
                int packetNum = mimcPacketList.packets_size();
                std::vector<MIMCMessage> p2pMimcMessages;
                std::vector<MIMCGroupMessage> p2tMimcMessages;
                std::map<int64_t, time_t> p2pSequences;
                std::map<int64_t, time_t> p2tSequences;
                for (int i = 0; i < packetNum; i++)
                {
                    mimc::MIMCPacket mimcMessagePacket = mimcPacketList.packets(i);
                    {
                        MIMCMutexLock mutexLock(&sequencesMutex);
                        if ((this->sequencesReceived).count(mimcMessagePacket.sequence()) != 0)
                        {
                            continue;
                        }
                    }

                    XMDLoggerWrapper::instance()->info("PacketManager::decodePacketAndHandle, mimcMessagePacket.type:%d", mimcMessagePacket.type());
                    if (mimcMessagePacket.type() == mimc::P2P_MESSAGE)
                    {
                        mimc::MIMCP2PMessage p2pMessage;
                        if (!p2pMessage.ParseFromString(mimcMessagePacket.payload()))
                        {
                            continue;
                        }
                        if (p2pMessage.to().resource() != "" && p2pMessage.to().resource() != user->getResource())
                        {
                            continue;
                        }

                        XMDLoggerWrapper::instance()->info("PacketManager::decodePacketAndHandle, p2pMessage.biztype:%s", p2pMessage.biztype().c_str());
                        // @TODO split p2p addr info msg
                        if (p2pMessage.biztype() == MP_MSG_NET_INFO)
                        {

                            XMDLoggerWrapper::instance()->info("received MP_MSG_NET_INFO msg for user %s, uuid %lld",
                                                               user->getAppAccount().c_str(), user->getUuid());
                            mimc::UserInfo userInfo;
                            if (!userInfo.ParseFromString(p2pMessage.payload()))
                            {
                                XMDLoggerWrapper::instance()->error("get addr info failed: %s, %s, %s, %s, %s, %s",
                                                                    p2pMessage.from().appaccount().c_str(),
                                                                    p2pMessage.from().resource().c_str(),
                                                                    p2pMessage.to().appaccount().c_str(),
                                                                    p2pMessage.to().resource().c_str(),
                                                                    p2pMessage.payload().c_str(),
                                                                    p2pMessage.biztype().c_str());
                                continue;
                            }

                            P2PProcessor::handleNetInfo(user, &userInfo);
                        }
                        else
                        {
                            p2pMimcMessages.push_back(MIMCMessage(mimcMessagePacket.packetid(), mimcMessagePacket.sequence(), p2pMessage.from().appaccount(), p2pMessage.from().resource(), p2pMessage.to().appaccount(), p2pMessage.to().resource(), p2pMessage.payload(), p2pMessage.biztype(), mimcMessagePacket.timestamp(), mimcMessagePacket.convindex()));
                            p2pSequences.insert(std::make_pair(mimcMessagePacket.sequence(), time(NULL)));
                        }
                    }
                    else if (mimcMessagePacket.type() == mimc::P2T_MESSAGE)
                    {
                        mimc::MIMCP2TMessage p2tMessage;
                        if (!p2tMessage.ParseFromString(mimcMessagePacket.payload()))
                        {
                            continue;
                        }
                        p2tMimcMessages.push_back(MIMCGroupMessage(mimcMessagePacket.packetid(), mimcMessagePacket.sequence(), p2tMessage.from().appaccount(), p2tMessage.from().resource(), p2tMessage.to().topicid(), p2tMessage.payload(), p2tMessage.biztype(), mimcMessagePacket.timestamp(), mimcMessagePacket.convindex()));
                        p2tSequences.insert(std::make_pair(mimcMessagePacket.sequence(), time(NULL)));
                    }
                }

                if (p2pMimcMessages.size() > 0)
                {
                    std::sort(p2pMimcMessages.begin(), p2pMimcMessages.end(), MIMCMessage::sortBySequence);
                    if (user->getMessageHandler() != NULL)
                    {
                        if (!user->getMessageHandler()->handleMessage(p2pMimcMessages)) {
                            isSendCompoundSequenceAck = false;
                            XMDLoggerWrapper::instance()->error("decodePacketAndHandle, handleMessage callback return false, pull message");
                        } else {
                            addCompoundSequenceAndCleanOld(p2pSequences);
                        }
                    }
                }

                if (p2tMimcMessages.size() > 0)
                {
                    std::sort(p2tMimcMessages.begin(), p2tMimcMessages.end(), MIMCGroupMessage::sortBySequence);
                    if (user->getMessageHandler() != NULL)
                    {
                        if(!user->getMessageHandler()->handleGroupMessage(p2tMimcMessages)) {
                            isSendCompoundSequenceAck = false;
                            XMDLoggerWrapper::instance()->error("decodePacketAndHandle, handleGroupMessage callback return false, pull message");
                        } else {
                            addCompoundSequenceAndCleanOld(p2tSequences);
                        }
                        
                    }
                }
                if (isSendCompoundSequenceAck) {
                    sendCompoundSequenceAck(mimcPacketList, user);
                } else {
                    // 拉取消息
                    user->pull();
                }
            }
        }
        else if (mimcPacket.type() == mimc::RTS_SIGNAL) {
            mimc::RTSMessage rtsMessage;
            if (!rtsMessage.ParseFromString(mimcPacket.payload())) {
                XMDLoggerWrapper::instance()->error("decodePacket failed, rtsMessage parse failed");
                return -1;
            }
            const uint64_t& callId = rtsMessage.callid();


            switch (rtsMessage.type()) {
                XMDLoggerWrapper::instance()->debug("rtsMessage type is %d",rtsMessage.type());
                case mimc::INVITE_REQUEST:
                {
                    int64_t ts = Utils::currentTimeMillis();
                    XMDLoggerWrapper::instance()->info("recv invite request timecost begin time=%llu", ts);
                    XMDLoggerWrapper::instance()->info("In INVITE_REQUEST, callId is %llu, user is %s", callId, user->getAppAccount().c_str());
                    mimc::InviteRequest inviteRequest;
                    if (!inviteRequest.ParseFromString(rtsMessage.payload())) {
                        XMDLoggerWrapper::instance()->error("In INVITE_REQUEST, ERROR: RTS_PAYLOAD_PARSE_ERROR");
                        RtsSendSignal::sendInviteResponse(user, callId, rtsMessage.calltype(), mimc::INTERNAL_ERROR1, "RTS_PAYLOAD_PARSE_ERROR");
                        return -1;
                    }
                    if (!inviteRequest.has_streamtype() || inviteRequest.members_size() == 0) {
                        XMDLoggerWrapper::instance()->error("In INVITE_REQUEST, ERROR: INVALID_PARAM");
                        RtsSendSignal::sendInviteResponse(user, callId, rtsMessage.calltype(), mimc::PARAMETER_ERROR, "INVALID_PARAM");
                        return 0;
                    }

                    mimc::UserInfo from;
                    bool uuid_in_members = false;
                    for (int i = 0; i < inviteRequest.members_size(); i++) {
                        const mimc::UserInfo& member = inviteRequest.members(i);
                        if (member.uuid() == rtsMessage.uuid()) {
                            uuid_in_members = true;
                            from = member;
                            break;
                        }
                    }
                    if (!uuid_in_members) {
                        XMDLoggerWrapper::instance()->error("In INVITE_REQUEST, ERROR: MEMBERS_NOT_CONTAIN_SENDER");
                        RtsSendSignal::sendInviteResponse(user, callId, rtsMessage.calltype(), mimc::PARAMETER_ERROR, "MEMBERS_NOT_CONTAIN_SENDER");
                        return 0;
                    }

                    if (user->currentCallsMapSize() == user->getMaxCallNum()) {
                        XMDLoggerWrapper::instance()->warn("In INVITE_REQUEST, current Calls size has reached maxCallNum %d!", user->getMaxCallNum());
                        RtsSendSignal::sendInviteResponse(user, callId, rtsMessage.calltype(), mimc::PEER_REFUSE, "USER_BUSY");
                        return 0;
                    }

                    if (user->getRelayLinkState() == NOT_CREATED) {

                        user->checkToRunXmdTranseiver();
                        uint64_t connId = RtsSendData::createRelayConn(user);
                        XMDLoggerWrapper::instance()->info("In INVITE_REQUEST, relayLinkState is NOT_CREATED, relayConnId is %llu", connId);
                        if (connId == 0) {
                            XMDLoggerWrapper::instance()->error("In INVITE_REQUEST, ERROR: RELAY CAN NOT BE CONNECTED");
                            RtsSendSignal::sendInviteResponse(user, callId, rtsMessage.calltype(), mimc::PEER_OFFLINE, "RELAY CAN NOT BE CONNECTED");
                            return 0;
                        }
                        user->insertIntoCurrentCallsMap(callId, P2PCallSession(callId, from, rtsMessage.calltype(), WAIT_CALL_ONLAUNCHED, time(NULL), false, inviteRequest.appcontent()));

                    } else if (user->getRelayLinkState() == BEING_CREATED) {
                        XMDLoggerWrapper::instance()->info("In INVITE_REQUEST, relayLinkState is BEING_CREATED");
                        user->insertIntoCurrentCallsMap(callId, P2PCallSession(callId, from, rtsMessage.calltype(), WAIT_CALL_ONLAUNCHED, time(NULL), false, inviteRequest.appcontent()));

                    } else {
                        XMDLoggerWrapper::instance()->info("In INVITE_REQUEST, relayLinkState is SUCC_CREATED");
                        user->insertIntoCurrentCallsMap(callId, P2PCallSession(callId, from, rtsMessage.calltype(), WAIT_INVITEE_RESPONSE, time(NULL), false, inviteRequest.appcontent()));

                        struct onLaunchedParam* param = new struct onLaunchedParam();
                        param->user = user;
                        param->callId = callId;
                        pthread_t onLaunchedThread;
                        pthread_attr_t attr;
                        pthread_attr_init(&attr);
                        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                        pthread_create (&onLaunchedThread, &attr, User::onLaunched, (void *)param);
                        user->insertLaunchCall(callId, onLaunchedThread);
                        pthread_attr_destroy(&attr);
                    }
                }
                    break;
                case mimc::CREATE_RESPONSE:
                {
                    int64_t ts = Utils::currentTimeMillis();
                    XMDLoggerWrapper::instance()->info("create response timecost end time=%llu", ts);
                    XMDLoggerWrapper::instance()->info("In CREATE_RESPONSE, callId is %llu, user is %s", callId, user->getAppAccount().c_str());
                    if (user->countCurrentCallsMap(callId) == 0) {
                        return 0;
                    }
                    mimc::CreateResponse createResponse;
                    if (!createResponse.ParseFromString(rtsMessage.payload())) {
                        XMDLoggerWrapper::instance()->error("CreateResponse parse error");
                        return -1;
                    }

                    if (!createResponse.has_result() || !createResponse.has_errmsg() || createResponse.members_size() == 0) {
                        XMDLoggerWrapper::instance()->error("In CREATE_RESPONSE,createResponse has error");
                        user->deleteFromCurrentCallsMap(callId);
                        RtsSendData::closeRelayConnWhenNoCall(user);
                        if(createResponse.has_result()) {
                             XMDLoggerWrapper::instance()->error("In CREATE_RESPONSE,result is %d",createResponse.result());
                        }
                        if(createResponse.has_errmsg()) {
                            XMDLoggerWrapper::instance()->error("In CREATE_RESPONSE,errmsg is %s",createResponse.errmsg().c_str());
                            user->getRTSCallEventHandler()->onAnswered(callId, false, createResponse.errmsg());
                        } else {
                            user->getRTSCallEventHandler()->onAnswered(callId, false, "param is abnormal");
                        }
                        
                        return 0;
                    }
                    
                    if (rtsMessage.calltype() == mimc::SINGLE_CALL && createResponse.members_size() != 2) {
                        XMDLoggerWrapper::instance()->error("In CREATE_RESPONSE,SINGLE_CALL MEMBER SIZE IS NOT 2");
                        user->deleteFromCurrentCallsMap(callId);
                        RtsSendData::closeRelayConnWhenNoCall(user);
                        user->getRTSCallEventHandler()->onAnswered(callId, false, "SINGLE_CALL MEMBER SIZE IS NOT 2");
                        return 0;
                    }

                    bool accepted = false;
                    if (createResponse.result() == mimc::SUCC) {
                        accepted = true;
                    } else {
                        XMDLoggerWrapper::instance()->error("createResponse.result is not success,res is %d,errmsg:%s",createResponse.result(),createResponse.errmsg().c_str());
                    }
                    if (accepted) {
                        mimc::UserInfo toUser;
                        for (int i = 0; i < createResponse.members_size(); i++) {
                            const mimc::UserInfo& member = createResponse.members(i);
                            if (member.uuid() == rtsMessage.uuid()) {
                                toUser = member;
                                break;
                            }
                        }
                        RWMutexLock rwlock(&user->getCallsRwlock());
                        rwlock.wlock();
                        P2PCallSession& callSession = user->getCurrentCalls()->at(callId);
                        callSession.setCallState(RUNNING);
                        callSession.setLatestLegalCallStateTs(time(NULL));

                        //会话接通，开始P2P相关流程
                        //added by huanghua on 2019-04-01
                        if(user->isP2PEnabled()) {
                            callSession.setPeerUser(toUser);
                            P2PProcessor::startP2P(user, callId);
                        }
                    } else {
                        XMDLoggerWrapper::instance()->warn("In CREATE_RESPONSE, accepted is false");
                        user->deleteFromCurrentCallsMap(callId);
                        RtsSendData::closeRelayConnWhenNoCall(user);
                    }
                    user->getRTSCallEventHandler()->onAnswered(callId, accepted, createResponse.errmsg());
                }
                    break;
                case mimc::BYE_REQUEST:
                {
                    XMDLoggerWrapper::instance()->info("In BYE_REQUEST, callId is %llu, user is %s, resource is %s", callId, user->getAppAccount().c_str(), user->getResource().c_str());
                    if (user->countCurrentCallsMap(callId) == 0) {
                        return 0;
                    }
                    mimc::ByeRequest byeRequest;
                    P2PCallSession callSession;
                    if (!user->getCallSession(callId, callSession)) {
                        XMDLoggerWrapper::instance()->error("sendByeResponse callid not exist %llu", callId);
                        return 0;
                    }
                    if (!byeRequest.ParseFromString(rtsMessage.payload())) {
                        RtsSendSignal::sendByeResponse(user, callId, mimc::INTERNAL_ERROR1,callSession);
                        return -1;
                    }
                    XMDLoggerWrapper::instance()->info("In BYE_REQUEST, byeReason is %s", byeRequest.reason().c_str());
                    pthread_t onlaunchCallThread;
                    if (user->getLaunchCallThread(callId, onlaunchCallThread)) {
#ifndef __ANDROID__
                        pthread_cancel(onlaunchCallThread);
#else
                        if(pthread_kill(onlaunchCallThread, 0) != ESRCH)
                        {
                            pthread_kill(onlaunchCallThread, SIGTERM);
                        }
#endif
                        user->deleteLaunchCall(callId);
                    }
                    RtsSendSignal::sendByeResponse(user, callId, mimc::SUCC,callSession);

                    {
                        RWMutexLock rwlock(&user->getCallsRwlock());
                        rwlock.wlock();
                        P2PProcessor::closeP2PConn(user, callId);
                        user->deleteFromCurrentCallsMapNoSafe(callId);    
                    }
                    RtsSendData::closeRelayConnWhenNoCall(user);
                    user->getRTSCallEventHandler()->onClosed(callId, byeRequest.reason());
                }
                    break;
                case mimc::BYE_RESPONSE:
                {
                    XMDLoggerWrapper::instance()->info("In BYE_RESPONSE, callId is %llu, user is %s, resource is %s", callId, user->getAppAccount().c_str(), user->getResource().c_str());
                    if (user->countCurrentCallsMap(callId) == 0) {
                        return 0;
                    }
                    mimc::ByeResponse byeResponse;
                    if (!byeResponse.ParseFromString(rtsMessage.payload())) {
                        return -1;
                    }
                    XMDLoggerWrapper::instance()->info("In BYE_RESPONSE, byeReason is %s", byeResponse.reason().c_str());
                    if (!byeResponse.has_result()) {
                        return 0;
                    }

                    {
                        RWMutexLock rwlock(&user->getCallsRwlock());
                        rwlock.wlock();
                        P2PProcessor::closeP2PConn(user, callId);
                        user->deleteFromCurrentCallsMapNoSafe(callId);    
                    }
                    RtsSendData::closeRelayConnWhenNoCall(user);
                    user->getRTSCallEventHandler()->onClosed(callId, byeResponse.reason());
                }
                    break;
                case mimc::UPDATE_REQUEST:
                {
                    if (user->countCurrentCallsMap(callId) == 0) {
                        return 0;
                    }
                    mimc::UpdateRequest updateRequest;
                    if (!updateRequest.ParseFromString(rtsMessage.payload())) {
                        RtsSendSignal::sendUpdateResponse(user, callId, mimc::INTERNAL_ERROR1);
                        return -1;
                    }
                    if (!updateRequest.has_user()) {

                        RtsSendSignal::sendUpdateResponse(user, callId, mimc::PARAMETER_ERROR);
                        return 0;
                    }
                    const mimc::UserInfo& toUser = updateRequest.user();
                    if (!toUser.has_internetip() || !toUser.has_internetport() || !toUser.has_intranetip() || !toUser.has_intranetport()) {
                        RtsSendSignal::sendUpdateResponse(user, callId, mimc::PARAMETER_ERROR);
                        return 0;
                    }
                    {
                        RWMutexLock rwlock(&user->getCallsRwlock());
                        rwlock.wlock();
                        P2PCallSession& callSession = user->getCurrentCalls()->at(callId);
                        callSession.setPeerUser(toUser);
                    }
                    RtsSendSignal::sendUpdateResponse(user, callId, mimc::SUCC);
                }
                    break;
                case mimc::UPDATE_RESPONSE:
                {
                    if (user->countCurrentCallsMap(callId) == 0) {
                        return 0;
                    }
                    mimc::UpdateResponse updateResponse;
                    if (!updateResponse.ParseFromString(rtsMessage.payload())) {
                        return -1;
                    }
                    if (!updateResponse.has_result()) {
                        return 0;
                    }
                    RWMutexLock rwlock(&user->getCallsRwlock());
                    rwlock.wlock();
                    P2PCallSession& callSession = user->getCurrentCalls()->at(callId);
                    callSession.setCallState(RUNNING);
                    callSession.setLatestLegalCallStateTs(time(NULL));
                }
                    break;
            }
        }


    }

    return 0;
}

void PacketManager::sendCompoundSequenceAck(mimc::MIMCPacketList &mimcPacketList, User *user)
{
    mimc::MIMCSequenceAck mimcSequenceAck;
    mimcSequenceAck.set_uuid(mimcPacketList.uuid());
    mimcSequenceAck.set_resource(mimcPacketList.resource());
    mimcSequenceAck.set_sequence(mimcPacketList.maxsequence());
    mimcSequenceAck.set_appid(user->getAppId());

    int message_size = mimcSequenceAck.ByteSize();
    char * messageBytes = new char[message_size];
    memset(messageBytes, 0, message_size);
    mimcSequenceAck.SerializeToArray(messageBytes, message_size);
    std::string messageBytesStr(messageBytes, message_size);

    mimc::MIMCPacket * ackPacket = new mimc::MIMCPacket();
    ackPacket->set_packetid(createPacketId());
    ackPacket->set_package(user->getAppPackage());
    ackPacket->set_type(mimc::SEQUENCE_ACK);
    ackPacket->set_payload(messageBytesStr);
    ackPacket->set_timestamp(time(NULL));

    delete[] messageBytes;
    messageBytes = NULL;

    struct waitToSendContent mimc_obj;
    mimc_obj.cmd = BODY_CLIENTHEADER_CMD_SECMSG;
    mimc_obj.type = C2S_SINGLE_DIRECTION;
    mimc_obj.message = ackPacket;

    (this->packetsWaitToSend).Push(mimc_obj);
}

void PacketManager::addCompoundSequenceAndCleanOld(std::map<int64_t, time_t> &sequencesReceived)
{
    MIMCMutexLock mutexLock(&sequencesMutex);
    sequencesReceived.insert(sequencesReceived.begin(), sequencesReceived.end());
}

std::string PacketManager::createPacketId() {
    MIMCMutexLock mutexLock(&packetIdMutex);
    std::string packetId = packetIdPrefix + "_" + Utils::int2str(packetIdSeq++);
    return packetId;
}

void PacketManager::checkMessageSendTimeout(const User * user) {
    MIMCMutexLock mutexLock(&packetsTimeoutMutex);
    std::vector<std::string> packetsWaitToDelete;
    std::map<std::string, MIMCMessage>::iterator iter;
    for (iter = (this->packetsWaitToTimeout).begin(); iter != (this->packetsWaitToTimeout).end(); iter++) {
        MIMCMessage mimcMessage = iter->second;
        if (time(NULL) - mimcMessage.getTimeStamp() < SEND_TIMEOUT) {
            continue;
        }

        if (user->getMessageHandler() != NULL) {
            user->getMessageHandler()->handleSendMsgTimeout(mimcMessage);
        }

        packetsWaitToDelete.push_back(mimcMessage.getPacketId());
    }

    int toDeleteSize = packetsWaitToDelete.size();
    for (int i = 0; i < toDeleteSize; i++) {
        (this->packetsWaitToTimeout).erase(packetsWaitToDelete[i]);
    }

}

void PacketManager::checkReceiveMessageSequence(){
    if(sequencesReceived.empty()){
        return;
    }
    MIMCMutexLock mutexLock(&sequencesMutex);
    std::map<int64_t,time_t>::iterator iter;
    for(iter = sequencesReceived.begin();iter != sequencesReceived.end();){
        if(time(NULL) - iter->second > SEQUENCE_TIMEOUT){
            iter = sequencesReceived.erase(iter);
        } else {
            iter++;
        }
    }

}

void PacketManager::short2char(int16_t data, unsigned char* result, int index) {
    unsigned char lowByte = data & 0XFF;
    unsigned char highByte = (data >> 8) & 0xFF;
    result[index] = highByte;
    result[index + 1] = lowByte;
}

void PacketManager::int2char(int32_t data, unsigned char* result, int index) {
    unsigned char firstByte = data & (0xff);
    unsigned char secondByte = (data >> 8) & (0xff);
    unsigned char thirdByte = (data >> 16) & (0xff);
    unsigned char fourthByte = (data >> 24) & (0xff);
    result[index] = fourthByte;
    result[index + 1] = thirdByte;
    result[index + 2] = secondByte;
    result[index + 3] = firstByte;
}


void PacketManager::short2charString(int16_t data, std::string& result, int index) {
    if(index + 1 > result.size() - 1) {
        XMDLoggerWrapper::instance()->error("beyond size");
    }
    unsigned char lowByte = data & 0XFF;
    unsigned char highByte = (data >> 8) & 0xFF;
    result[index] = highByte;
    result[index + 1] = lowByte;
}

void PacketManager::int2charString(int32_t data, std::string& result, int index) {
    if(index + 3 > result.size() - 1) {
        XMDLoggerWrapper::instance()->error("beyond size");
    }
    unsigned char firstByte = data & (0xff);
    unsigned char secondByte = (data >> 8) & (0xff);
    unsigned char thirdByte = (data >> 16) & (0xff);
    unsigned char fourthByte = (data >> 24) & (0xff);
    result[index] = fourthByte;
    result[index + 1] = thirdByte;
    result[index + 2] = secondByte;
    result[index + 3] = firstByte;
}

int16_t PacketManager::char2short(const unsigned char* result, int index) {
    unsigned char highByte = result[index];
    unsigned char lowByte = result[index + 1];
    int16_t ret = (highByte << 8) | lowByte;
    return ret;
}

int32_t PacketManager::char2int(const unsigned char* result, int index) {
    unsigned char fourthByte = result[index];
    unsigned char thirdByte = result[index + 1];
    unsigned char secondByte = result[index + 2];
    unsigned char firstByte = result[index + 3];
    int32_t ret = (fourthByte << 24) | (thirdByte << 16) | (secondByte << 8) | firstByte;
    return ret;
}

uint32_t PacketManager::compute_crc32(const unsigned char *data, size_t len)
{
    return adler32(1L, data, len);
}

std::string PacketManager::generateSig(const ims::ClientHeader& header, const ims::XMMsgBind * bindmsg, const Connection * connection) {
    std::map<std::string, std::string> params;
    params.insert(std::pair<std::string, std::string>("challenge", connection->getChallenge()));
    params.insert(std::pair<std::string, std::string>("token", bindmsg->token()));
    params.insert(std::pair<std::string, std::string>("chid", Utils::int2str(header.chid())));
    params.insert(std::pair<std::string, std::string>("from", Utils::int2str(header.uuid()).append("@xiaomi.com/").append(header.resource())));
    params.insert(std::pair<std::string, std::string>("id", header.id()));
    params.insert(std::pair<std::string, std::string>("to", MIMC_SERVER));
    params.insert(std::pair<std::string, std::string>("kick", bindmsg->kick()));
    params.insert(std::pair<std::string, std::string>("client_attrs", bindmsg->client_attrs()));
    params.insert(std::pair<std::string, std::string>("cloud_attrs", bindmsg->cloud_attrs()));

    std::ostringstream oss;
    std::string requestMethodUpper;
    const std::string& requestMethod = bindmsg->method();
    std::transform(requestMethod.begin(), requestMethod.end(), std::back_inserter(requestMethodUpper), ::toupper);
    oss << requestMethodUpper << "&";
    for (std::map<std::string, std::string>::const_iterator iter = params.begin(); iter != params.end(); iter++) {
        const std::string& paramKey = iter->first;
        const std::string& paramValue = iter->second;
        oss << paramKey << "=" << paramValue << "&";
    }
    oss << connection->getUser()->getSecurityKey();
    return Utils::hash4SHA1AndBase64(oss.str());
}

std::string PacketManager::generatePayloadKey(const std::string &securityKey, const std::string &headerId) {
    std::string keyBytes;
    ccb::Base64Util::Decode(securityKey, keyBytes);

    int keyBytesLen = keyBytes.length();
    int headerIdLen = headerId.length();

    char *payloadKey = new char[keyBytesLen + 1 + headerIdLen];
    memset(payloadKey, 0, keyBytesLen + 1 + headerIdLen);
    memmove(payloadKey, keyBytes.c_str(), keyBytesLen);
    payloadKey[keyBytesLen] = '_';
    memmove(payloadKey + keyBytesLen + 1, headerId.c_str(), headerIdLen);
    std::string result(payloadKey, keyBytesLen + 1 + headerIdLen);
    delete[] payloadKey;
    payloadKey = NULL;
    return result;
}