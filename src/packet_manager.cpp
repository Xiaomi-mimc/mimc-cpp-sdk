#include <mimc/packet_manager.h>
#include <mimc/user.h>
#include <mimc/constant.h>
#include <string.h>
#include <crypto/rc4_crypto.h>
#include <zlib.h>
#include <iostream>
#include <algorithm>

ims::ClientHeader * PacketManager::createClientHeader(const User * user, std::string cmd, int cipher) {
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

int PacketManager::encodeConnectionPacket(unsigned char * &packet, const Connection * connection) {
	const User * user = connection->getUser();
	ims::ClientHeader * header = createClientHeader(user, BODY_CLIENTHEADER_CMD_CONN, BODY_CLIENTHEADER_CIPHER_NONE);
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
	ims::ClientHeader * header = createClientHeader(user, BODY_CLIENTHEADER_CMD_BIND, BODY_CLIENTHEADER_CIPHER_NONE);
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

int PacketManager::encodeSecMsgPacket(unsigned char * &packet, const Connection * connection, const google::protobuf::Message * message) {
	const User * user = connection->getUser();
	ims::ClientHeader * header = createClientHeader(user, BODY_CLIENTHEADER_CMD_SECMSG, BODY_CLIENTHEADER_CIPHER_RC4);
	std::string body_key = connection->getBodyKey();
	std::string payload_key = generatePayloadKey(user->getSecurityKey(), header->id());
	return encodePacket(packet, header, message, body_key, payload_key);
}

int PacketManager::encodeUnBindPacket(unsigned char * &packet, const Connection * connection) {
	const User * user = connection->getUser();
	ims::ClientHeader * header = createClientHeader(user, BODY_CLIENTHEADER_CMD_UNBIND, BODY_CLIENTHEADER_CIPHER_NONE);
	std::string body_key = connection->getBodyKey();
	return encodePacket(packet, header, NULL, body_key);
}

int PacketManager::encodePingPacket(unsigned char * &packet, const Connection * connection) {
	return encodePacket(packet, NULL, NULL);
}

int PacketManager::encodePacket(unsigned char * &packet, const ims::ClientHeader * header, const google::protobuf::Message * message, const std::string &body_key, const std::string &payload_key) {
	short body_head_size = 0;
	int body_message_size = 0;
	int body_size = 0;
	char * raw_header = NULL;
	char * final_message = NULL;

	if (header != NULL) {
		body_head_size = header->ByteSize();
		raw_header = new char[body_head_size];
		memset(raw_header, 0, body_head_size);
		header->SerializeToArray(raw_header, body_head_size);

		if (message != NULL) {
			body_message_size = message->ByteSize();
			char * raw_message = new char[body_message_size];
			memset(raw_message, 0, body_message_size);
			message->SerializeToArray(raw_message, body_message_size);
			final_message = new char[body_message_size];
			memset(final_message, 0, body_message_size);
			if (header->cmd() != BODY_CLIENTHEADER_CMD_SECMSG) {
				memmove(final_message, raw_message, body_message_size);
			} else {
				std::string raw((char *)raw_message, body_message_size);
				std::string message_cipher;
				if (ccb::CryptoRC4Util::Encrypt(raw, message_cipher, payload_key) != 0) {
					LOG4CPLUS_ERROR(LOGGER, "message encrypt error");
					return -1;
				}
				memmove(final_message, message_cipher.c_str(), body_message_size);
			}
			delete[] raw_message;
			raw_message = NULL;
		}
	}

	unsigned char * raw_body = NULL;
	char * final_body = NULL;

	if (body_head_size > 0) {
		body_size = BODY_HEADER_LENGTH + body_head_size + body_message_size;
		raw_body = new unsigned char[body_size];
		memset(raw_body, 0, body_size);
		short2char(BODY_HEADER_PAYLOADTYPE, raw_body, BODY_HEADER_PAYLOADTYPE_OFFSET);
		short2char(body_head_size, raw_body, BODY_HEADER_HEADERLEN_OFFSET);
		int2char(body_message_size, raw_body, BODY_HEADER_PAYLOADLEN_OFFSET);
		memmove(raw_body + BODY_HEADER_LENGTH, raw_header, body_head_size);

		if (body_message_size > 0) {
			memmove(raw_body + BODY_HEADER_LENGTH + body_head_size, final_message, body_message_size);
		}

		final_body = new char[body_size];
		memset(final_body, 0, body_size);
		if (header->cmd() == BODY_CLIENTHEADER_CMD_CONN) {
			memmove(final_body, raw_body, body_size);
		} else {
			std::string raw((char *)raw_body, body_size);
			std::string body_cipher;
			if (ccb::CryptoRC4Util::Encrypt(raw, body_cipher, body_key) != 0) {
				LOG4CPLUS_ERROR(LOGGER, "body encrypt error");
				return -1;
			}
			memmove(final_body, body_cipher.c_str(), body_size);
		}
	}

	delete[] raw_header;
	raw_header = NULL;

	delete[] final_message;
	final_message = NULL;

	delete[] raw_body;
	raw_body = NULL;
	int packet_size = HEADER_LENGTH + body_size + BODY_CRC_LEN;
	packet = new unsigned char[packet_size];
	memset(packet, 0, packet_size);
	short2char(HEADER_MAGIC, packet, HEADER_MAGIC_OFFSET);
	short2char(HEADER_VERSION, packet, HEADER_VERSION_OFFSET);
	int2char(body_size, packet, HEADER_BODYLEN_OFFSET);
	memmove(packet + HEADER_LENGTH, final_body, body_size);

	uint32_t crc = compute_crc32(packet, packet_size - BODY_CRC_LEN);
	int2char(crc, packet, packet_size - BODY_CRC_LEN);

	delete[] final_body;
	final_body = NULL;

	delete header;
	header = NULL;

	delete message;
	message = NULL;

	return packet_size;
}

int PacketManager::decodePacketAndHandle(unsigned char * packet, Connection * connection) {
	short magic = char2short(packet, HEADER_MAGIC_OFFSET);
	short version = char2short(packet, HEADER_VERSION_OFFSET);
	if (magic != HEADER_MAGIC || version != HEADER_VERSION) {
		return -1;
	}
	int body_size = char2int(packet, HEADER_BODYLEN_OFFSET);
	int crc_offset = HEADER_LENGTH + body_size;
	uint32_t compute_crc = compute_crc32(packet, crc_offset);
	uint32_t crc = char2int(packet, crc_offset);
	LOG4CPLUS_INFO(LOGGER, "compute_crc is " << compute_crc << ", raw crc is " << crc);
	if (compute_crc != crc) {
		LOG4CPLUS_ERROR(LOGGER, "packet has been modified");
		return -1;
	}

	if (body_size == 0) {
		LOG4CPLUS_INFO(LOGGER, "PING_PACKET or PONG_PACTET");
		return 0;
	}

	if (connection->getBodyKey() != "") {
		std::string raw((char *)(packet + HEADER_LENGTH), body_size);
		std::string body_plain;
		if (ccb::CryptoRC4Util::Decrypt(raw, body_plain, connection->getBodyKey()) != 0) {
			LOG4CPLUS_ERROR(LOGGER, "body decrypt error");
			return -1;
		}
		memmove(packet + HEADER_LENGTH, body_plain.c_str(), body_size);
	}

	short payload_type = char2short(packet, HEADER_LENGTH + BODY_HEADER_PAYLOADTYPE_OFFSET);
	short body_head_size = char2short(packet, HEADER_LENGTH + BODY_HEADER_HEADERLEN_OFFSET);
	int body_message_size = char2int(packet, HEADER_LENGTH + BODY_HEADER_PAYLOADLEN_OFFSET);

	ims::ClientHeader header;
	if (!header.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH, body_head_size)) {
		LOG4CPLUS_ERROR(LOGGER, "ClientHeader parse failed");
		return -1;
	}

	User * user = connection->getUser();
	std::string cmd = header.cmd();
	LOG4CPLUS_INFO(LOGGER, "receive cmd is " << cmd);
	if (cmd == BODY_CLIENTHEADER_CMD_CONN) {
		ims::XMMsgConnResp resp;
		if (!resp.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_message_size)) {
			LOG4CPLUS_ERROR(LOGGER, "XMMsgConnResp parse failed");
			return -1;
		}
		connection->setState(HANDSHAKE_CONNECTED);
		LOG4CPLUS_INFO(LOGGER, "HANDSHAKE_CONNECTED");
		std::string challenge = resp.challenge();
		connection->setChallengeAndBodyKey(challenge);
	}
	else if (cmd == BODY_CLIENTHEADER_CMD_BIND) {
		ims::XMMsgBindResp resp;
		if (!resp.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_message_size)) {
			LOG4CPLUS_ERROR(LOGGER, "XMMsgBindResp parse failed");
			return -1;
		}
		OnlineStatus onlineStatus = resp.result() ? Online : Offline;
		if (onlineStatus == Online) {
			LOG4CPLUS_INFO(LOGGER, "user bind succeed");
		} else {
			LOG4CPLUS_WARN(LOGGER, "user bind failed");
		}

		user->setOnlineStatus(onlineStatus);
		if (user->getStatusHandler() != NULL) {
			user->getStatusHandler()->statusChange(onlineStatus, resp.error_type(), resp.error_reason(), resp.error_desc());
		}
		if (resp.error_type() == "token-expired") {
			user->login();
		}
	}
	else if (cmd == BODY_CLIENTHEADER_CMD_KICK) {
		ims::XMMsgBindResp resp;
		if (!resp.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_message_size)) {
			LOG4CPLUS_ERROR(LOGGER, "XMMsgBindResp parse failed");
			return -1;
		}
		OnlineStatus onlineStatus = resp.result() ? Online : Offline;
		if (onlineStatus == Offline) {
			LOG4CPLUS_INFO(LOGGER, "user unBind succeed");
		}
		user->setOnlineStatus(onlineStatus);
		if (user->getStatusHandler() != NULL) {
			user->getStatusHandler()->statusChange(onlineStatus, resp.error_type(), resp.error_reason(), resp.error_desc());
		}
	}
	else if (cmd == BODY_CLIENTHEADER_CMD_SECMSG) {
		if (header.chid() == MIMC_CHID && header.uuid() == user->getUuid()) {
			std::string raw((char *)(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size), body_message_size);
			std::string body_plain;
			std::string payload_key = generatePayloadKey(user->getSecurityKey(), header.id());
			if (ccb::CryptoRC4Util::Decrypt(raw, body_plain, payload_key) != 0) {
				LOG4CPLUS_ERROR(LOGGER, "message decrypt error");
				return -1;
			}
			memmove(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_plain.c_str(), body_message_size);
			mimc::MIMCPacket mimcPacket;
			if (!mimcPacket.ParseFromArray(packet + HEADER_LENGTH + BODY_HEADER_LENGTH + body_head_size, body_message_size)) {
				LOG4CPLUS_ERROR(LOGGER, "XMMsgMIMCPacket parse failed");
				return -1;
			}
			LOG4CPLUS_INFO(LOGGER, "mimcPacket.type is " << mimcPacket.type());
			if (mimcPacket.type() == mimc::PACKET_ACK) {
				mimc::MIMCPacketAck mimcPacketAck;
				if (!mimcPacketAck.ParseFromString(mimcPacket.payload())) {
					LOG4CPLUS_ERROR(LOGGER, "mimcPacketAck parse failed");
					return -1;
				}
				if (user->getMessageHandler() != NULL) {
					user->getMessageHandler()->handleServerAck(mimcPacketAck.packetid(), mimcPacketAck.sequence(), mimcPacketAck.timestamp(), mimcPacketAck.errormsg());
				}
				pthread_mutex_lock(&packetsTimeoutMutex);
				(this->packetsWaitToTimeout).erase(mimcPacketAck.packetid());
				pthread_mutex_unlock(&packetsTimeoutMutex);
				LOG4CPLUS_INFO(LOGGER, "mimcPacketAck process succeed");
			}
			else if (mimcPacket.type() == mimc::COMPOUND) {
				mimc::MIMCPacketList mimcPacketList;
				if (!mimcPacketList.ParseFromString(mimcPacket.payload()) || mimcPacketList.packets_size() == 0) {
					LOG4CPLUS_ERROR(LOGGER, "mimcPacketList parse failed");
					return -1;
				}

				mimc::MIMCSequenceAck mimcSequenceAck;
				mimcSequenceAck.set_uuid(mimcPacketList.uuid());
				mimcSequenceAck.set_resource(mimcPacketList.resource());
				mimcSequenceAck.set_sequence(mimcPacketList.maxsequence());

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

				delete[] messageBytes;
				messageBytes = NULL;

				struct waitToSendContent mimc_obj;
				mimc_obj.cmd = BODY_CLIENTHEADER_CMD_SECMSG;
				mimc_obj.type = C2S_SINGLE_DIRECTION;
				mimc_obj.message = ackPacket;
				(this->packetsWaitToSend).push(mimc_obj);

				int packetNum = mimcPacketList.packets_size();
				LOG4CPLUS_INFO(LOGGER, "packetNum is " << packetNum);
				std::vector<MIMCMessage> p2pMimcMessages;
				for (int i = 0; i < packetNum; i++) {
					mimc::MIMCPacket mimcMessagePacket = mimcPacketList.packets(i);
					if ((this->sequencesReceived).count(mimcMessagePacket.sequence())) {
						LOG4CPLUS_INFO(LOGGER, "RECV_PACKET, IGNORE, RECEIVED_AGAGIN, sequence = " << mimcMessagePacket.sequence());
						continue;
					}
					(this->sequencesReceived).insert(mimcMessagePacket.sequence());
					if (mimcMessagePacket.type() == mimc::P2P_MESSAGE) {
						mimc::MIMCP2PMessage p2pMessage;
						if (!p2pMessage.ParseFromString(mimcMessagePacket.payload())) {
							LOG4CPLUS_ERROR(LOGGER, "p2pMessage parse failed");
							continue;
						}
						if (p2pMessage.to().resource() != "" && p2pMessage.to().resource() != user->getResource()) {
							LOG4CPLUS_ERROR(LOGGER, "RECV_PACKET, PACKET, RESOURCE_NOT_MATCH, " << user->getResource() << "!=" << p2pMessage.to().resource());
							continue;
						}
						p2pMimcMessages.push_back(MIMCMessage(mimcMessagePacket.packetid(), mimcMessagePacket.sequence(), p2pMessage.from().appaccount(), p2pMessage.from().resource(), p2pMessage.payload(), mimcMessagePacket.timestamp()));
					}
				}
				if (p2pMimcMessages.size() > 0) {
					std::sort(p2pMimcMessages.begin(), p2pMimcMessages.end(), MIMCMessage::sortBySequence);
					if (user->getMessageHandler() != NULL) {
						user->getMessageHandler()->handleMessage(p2pMimcMessages);
					}
				}
			}
		}
	}

	return 0;
}

std::string PacketManager::createPacketId() {
	pthread_mutex_lock(&packetIdMutex);
	std::string packetId = packetIdPrefix + "_" + Utils::int2str(packetIdSeq++);
	pthread_mutex_unlock(&packetIdMutex);
	return packetId;
}

void PacketManager::checkMessageSendTimeout(const User * user) {
	pthread_mutex_lock(&packetsTimeoutMutex);
	std::vector<std::string> packetsWaitToDelete;
	std::map<std::string, struct waitToTimeoutContent>::iterator iter;
	for (iter = (this->packetsWaitToTimeout).begin(); iter != (this->packetsWaitToTimeout).end(); iter++) {
		struct waitToTimeoutContent timeoutPacket = iter->second;
		if (time(NULL) - timeoutPacket.timestamp < SEND_TIMEOUT) {
			continue;
		}
		mimc::MIMCPacket * mimcPacket = timeoutPacket.mimcPacket;
		if (mimcPacket->type() == mimc::P2P_MESSAGE) {
			mimc::MIMCP2PMessage p2pMessage;
			if (!p2pMessage.ParseFromString(mimcPacket->payload())) {
				LOG4CPLUS_ERROR(LOGGER, "p2pMessage timeout packet parse failed");
				continue;
			}
			long sequence = mimcPacket->sequence();
			long timestamp = mimcPacket->timestamp();
			MIMCMessage mimcMessage(mimcPacket->packetid(), sequence, p2pMessage.from().appaccount(), p2pMessage.from().resource(), p2pMessage.payload(), timestamp);
			if (user->getMessageHandler() != NULL) {
				user->getMessageHandler()->handleSendMsgTimeout(mimcMessage);
			}
		}
		packetsWaitToDelete.push_back(mimcPacket->packetid());
	}
	int toDeleteSize = packetsWaitToDelete.size();
	for (int i = 0; i < toDeleteSize; i++) {
		(this->packetsWaitToTimeout).erase(packetsWaitToDelete[i]);
	}

	pthread_mutex_unlock(&packetsTimeoutMutex);
}

void PacketManager::short2char(short data, unsigned char* result, int index) {
	unsigned char lowByte = data & 0XFF;
	unsigned char highByte = (data >> 8) & 0xFF;
	result[index] = highByte;
	result[index + 1] = lowByte;
}

void PacketManager::int2char(int data, unsigned char* result, int index) {
	unsigned char firstByte = data & (0xff);
	unsigned char secondByte = (data >> 8) & (0xff);
	unsigned char thirdByte = (data >> 16) & (0xff);
	unsigned char fourthByte = (data >> 24) & (0xff);
	result[index] = fourthByte;
	result[index + 1] = thirdByte;
	result[index + 2] = secondByte;
	result[index + 3] = firstByte;
}

short PacketManager::char2short(const unsigned char* result, int index) {
	unsigned char highByte = result[index];
	unsigned char lowByte = result[index + 1];
	short ret = (highByte << 8) | lowByte;
	return ret;
}

int PacketManager::char2int(const unsigned char* result, int index) {
	unsigned char fourthByte = result[index];
	unsigned char thirdByte = result[index + 1];
	unsigned char secondByte = result[index + 2];
	unsigned char firstByte = result[index + 3];
	int ret = (fourthByte << 24) | (thirdByte << 16) | (secondByte << 8) | firstByte;
	return ret;
}

uint32_t PacketManager::compute_crc32(const unsigned char *data, size_t len)
{
	return adler32(1L, data, len);
}

std::string PacketManager::generateSig(const ims::ClientHeader * header, const ims::XMMsgBind * bindmsg, const Connection * connection) {
	std::map<std::string, std::string> params;
	params.insert(std::pair<std::string, std::string>("challenge", connection->getChallenge()));
	params.insert(std::pair<std::string, std::string>("token", bindmsg->token()));
	params.insert(std::pair<std::string, std::string>("chid", Utils::int2str(header->chid())));
	params.insert(std::pair<std::string, std::string>("from", Utils::int2str(header->uuid()).append("@xiaomi.com/").append(header->resource())));
	params.insert(std::pair<std::string, std::string>("id", header->id()));
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