#include <mimc/rts_send_data.h>
#include <mimc/user.h>

unsigned long RtsSendData::createRelayConn(User* user) {
	//待加入getRelayAddress
#ifndef STAGING
	std::string relayIp = "58.83.177.218";
	int relayPort = 80;
#else
	std::string relayIp = "10.38.162.117";
	int relayPort = 6777;
#endif
	if (user->getRelayConnId() != 0) {
		return user->getRelayConnId();
	}

	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_pkt_type(mimc::RELAY_CONN_REQUEST);

	int message_size = userPacket.ByteSize();
	char messageBytes[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	unsigned long relayConnId = user->getXmdTransceiver()->createConnection((char *)relayIp.c_str(), relayPort, messageBytes, message_size, XMD_TRAN_TIMEOUT, new RtsConnectionInfo(RELAY_CONN), true);
	if (relayConnId == 0) {
		return 0;
	}

	user->setRelayConnId(relayConnId);
	user->setRelayLinkState(BEING_CREATED);
	user->setLatestLegalRelayLinkStateTs(time(NULL));

	return relayConnId;
}

bool RtsSendData::sendBindRelayRequest(User* user) {
	std::string localIp;
	int localPort;
	if (user->getXmdTransceiver()->getLocalInfo(localIp, localPort) < 0) {
		
		return false;
	}
	mimc::BindRelayRequest bindRelayRequest;
	bindRelayRequest.set_uuid(user->getUuid());
	bindRelayRequest.set_resource(user->getResource());
	bindRelayRequest.set_intranet_ip(localIp);
	bindRelayRequest.set_intranet_port(localPort);
	bindRelayRequest.set_token(user->getToken());

	int userPacketPayloadSize = bindRelayRequest.ByteSize();
	char userPacketPayload[userPacketPayloadSize];
	memset(userPacketPayload, 0, userPacketPayloadSize);
	bindRelayRequest.SerializeToArray(userPacketPayload, userPacketPayloadSize);

	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_payload(userPacketPayload, userPacketPayloadSize);
	userPacket.set_pkt_type(mimc::BIND_RELAY_REQUEST);

	int message_size = userPacket.ByteSize();
	char messageBytes[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	if (user->getXmdTransceiver()->sendRTData(user->getRelayConnId(), user->getRelayControlStreamId(), messageBytes, message_size) < 0) {
		
		return false;
	}

	
	return true;
}

bool RtsSendData::sendPingRelayRequest(User* user) {
	if (user->getCurrentChats()->empty() || user->getRelayLinkState() != SUCC_CREATED) {
		
		return false;
	}

	mimc::PingRelayRequest pingRelayRequest;
	pingRelayRequest.set_uuid(user->getUuid());
	pingRelayRequest.set_resource(user->getResource());

	int userPacketPayloadSize = pingRelayRequest.ByteSize();
	char userPacketPayload[userPacketPayloadSize];
	memset(userPacketPayload, 0, userPacketPayloadSize);
	pingRelayRequest.SerializeToArray(userPacketPayload, userPacketPayloadSize);

	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_payload(userPacketPayload, userPacketPayloadSize);
	userPacket.set_pkt_type(mimc::PING_RELAY_REQUEST);

	int message_size = userPacket.ByteSize();
	char messageBytes[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	if (user->getXmdTransceiver()->sendRTData(user->getRelayConnId(), user->getRelayControlStreamId(), messageBytes, message_size) < 0) {
		
		return false;
	}

	
	return true;
}

bool RtsSendData::sendRtsDataByRelay(User* user, long chatId, const std::string& data, mimc::PKT_TYPE pktType) {
	unsigned long relayConnId = user->getRelayConnId();
	if (relayConnId == 0) {
		
		return false;
	}
	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_pkt_type(pktType);
	userPacket.set_payload(std::string(data));
	userPacket.set_chat_id(chatId);

	int message_size = userPacket.ByteSize();
	char messageBytes[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	XMDTransceiver* xmdTransceiver = user->getXmdTransceiver();
	if (pktType == mimc::USER_DATA_AUDIO) {
		if (user->getRelayAudioStreamId() == 0) {
			user->setRelayAudioStreamId(xmdTransceiver->createStream(relayConnId, FEC_STREAM, XMD_TRAN_TIMEOUT));
		}
		if (user->getRelayAudioStreamId() != 0) {
			if (xmdTransceiver->sendRTData(relayConnId, user->getRelayAudioStreamId(), messageBytes, message_size) < 0) {
				
				return false;
			}
		} else {
			
			return false;
		}
	} else {
		if (user->getRelayVideoStreamId() == 0) {
			user->setRelayVideoStreamId(xmdTransceiver->createStream(relayConnId, FEC_STREAM, XMD_TRAN_TIMEOUT));
		}
		if (user->getRelayVideoStreamId() != 0) {
			if (xmdTransceiver->sendRTData(relayConnId, user->getRelayVideoStreamId(), messageBytes, message_size) < 0) {
				
				return false;
			}
		} else {
			
			return false;
		}
	}

	return true;
}

bool RtsSendData::closeRelayConnWhenNoChat(User* user) {
	if (user->getRelayConnId() == 0) {
		
		return false;
	}
	if (user->getCurrentChats()->size() > 0) {
		
		return false;
	}
	user->getXmdTransceiver()->closeConnection(user->getRelayConnId());
	user->resetRelayLinkState();
	
	return true;
}