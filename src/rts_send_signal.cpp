#include <mimc/rts_send_signal.h>
#include <mimc/user.h>
#include <mimc/packet_manager.h>

bool RtsSendSignal::sendCreateRequest(const User* user, uint64_t chatId) {
	const mimc::BindRelayResponse& bindRelayResponse = user->getBindRelayResponse();
	if (!bindRelayResponse.IsInitialized()) {
		
		return false;
	}

	std::string localIp;
	uint16_t localPort;
	if (user->getXmdTransceiver()->getLocalInfo(localIp, localPort) < 0) {
		
		return false;
	}

	P2PChatSession& chatSession = user->getCurrentChats()->at(chatId);
	mimc::CreateRequest createRequest;
	mimc::UserInfo* fromUser = createRequest.add_members();
	fromUser->set_uuid(user->getUuid());
	fromUser->set_resource(user->getResource());
	fromUser->set_appid(user->getAppId());
	fromUser->set_appaccount(user->getAppAccount());
	fromUser->set_intranetip(localIp);
	fromUser->set_intranetport(localPort);
	fromUser->set_internetip(bindRelayResponse.internet_ip());
	fromUser->set_internetport(bindRelayResponse.internet_port());
	fromUser->set_relayip(bindRelayResponse.relay_ip());
	fromUser->set_relayport(bindRelayResponse.relay_port());
	fromUser->set_connid(user->getRelayConnId());

	mimc::UserInfo* toUser = createRequest.add_members();
	mimc::UserInfo toUserInfo = chatSession.getPeerUser();
	toUser->set_appid(toUserInfo.appid());
	toUser->set_appaccount(toUserInfo.appaccount());
	toUser->set_resource(toUserInfo.resource());

	createRequest.set_appcontent(chatSession.getAppContent());

	int create_request_size = createRequest.ByteSize();
	char createRequestBytes[create_request_size];
	memset(createRequestBytes, 0, create_request_size);
	createRequest.SerializeToArray(createRequestBytes, create_request_size);
	std::string createRequestBytesStr(createRequestBytes, create_request_size);

	std::string packetId = sendRtsMessage(user, chatId, mimc::CREATE_REQUEST, chatSession.getChatType(), createRequestBytesStr);

	chatSession.setChatState(WAIT_CREATE_RESPONSE);
	chatSession.setLatestLegalChatStateTs(time(NULL));

	XMDLoggerWrapper::instance()->info("RtsSendSignal::sendCreateRequest has called, user is %s, chatId is %llu", user->getAppAccount().c_str(), chatId);

	return true;
}

bool RtsSendSignal::sendInviteResponse(const User* user, uint64_t chatId, mimc::ChatType chatType, mimc::RTSResult result, std::string errmsg) {
	std::string localIp;
	uint16_t localPort;
	if (user->getXmdTransceiver()->getLocalInfo(localIp, localPort) < 0) {
		
		return false;
	}

	mimc::InviteResponse inviteResponse;
	mimc::UserInfo* theUser = inviteResponse.mutable_user();
	theUser->set_uuid(user->getUuid());
	theUser->set_resource(user->getResource());
	theUser->set_appid(user->getAppId());
	theUser->set_appaccount(user->getAppAccount());
	theUser->set_intranetip(localIp);
	theUser->set_intranetport(localPort);
	const mimc::BindRelayResponse& bindRelayResponse = user->getBindRelayResponse();
	if (bindRelayResponse.IsInitialized()) {
		theUser->set_internetip(bindRelayResponse.internet_ip());
		theUser->set_internetport(bindRelayResponse.internet_port());
		theUser->set_relayip(bindRelayResponse.relay_ip());
		theUser->set_relayport(bindRelayResponse.relay_port());
	}

	theUser->set_connid(user->getRelayConnId());

	inviteResponse.set_result(result);
	inviteResponse.set_errmsg(errmsg);

	int invite_response_size = inviteResponse.ByteSize();
	char inviteResponseBytes[invite_response_size];
	memset(inviteResponseBytes, 0, invite_response_size);
	inviteResponse.SerializeToArray(inviteResponseBytes, invite_response_size);
	std::string inviteResponseBytesStr(inviteResponseBytes, invite_response_size);

	std::string packetId = sendRtsMessage(user, chatId, mimc::INVITE_RESPONSE, chatType, inviteResponseBytesStr);

	XMDLoggerWrapper::instance()->info("RtsSendSignal::sendInviteResponse has called, user is %s, chatId is %llu, result is %d, errmsg is %s", user->getAppAccount().c_str(), chatId, result, errmsg.c_str());

	return true;
}

bool RtsSendSignal::sendByeRequest(const User* user, uint64_t chatId, std::string byeReason) {
	const P2PChatSession& chatSession = user->getCurrentChats()->at(chatId);
	if (chatSession.getChatState() != RUNNING && chatSession.getChatState() != WAIT_SEND_UPDATE_REQUEST && chatSession.getChatState() != WAIT_UPDATE_RESPONSE) {
		return false;
	}
	mimc::ByeRequest byeRequest;
	if (byeReason != "") {
		byeRequest.set_reason(byeReason);
	}

	int bye_request_size = byeRequest.ByteSize();
	char byeRequestBytes[bye_request_size];
	memset(byeRequestBytes, 0, bye_request_size);
	byeRequest.SerializeToArray(byeRequestBytes, bye_request_size);
	std::string byeRequestBytesStr(byeRequestBytes, bye_request_size);

	std::string packetId = sendRtsMessage(user, chatId, mimc::BYE_REQUEST, chatSession.getChatType(), byeRequestBytesStr);

	XMDLoggerWrapper::instance()->info("RtsSendSignal::sendByeRequest has called, user is %s, chatId is %llu, byeReason is %s", user->getAppAccount().c_str(), chatId, byeReason.c_str());

	return true;
}

bool RtsSendSignal::sendByeResponse(const User* user, uint64_t chatId, mimc::RTSResult result) {
	const P2PChatSession& chatSession = user->getCurrentChats()->at(chatId);
	mimc::ByeResponse byeResponse;
	byeResponse.set_result(result);
	int bye_response_size = byeResponse.ByteSize();
	char byeResponseBytes[bye_response_size];
	memset(byeResponseBytes, 0, bye_response_size);
	byeResponse.SerializeToArray(byeResponseBytes, bye_response_size);
	std::string byeResponseBytesStr(byeResponseBytes, bye_response_size);

	std::string packetId = sendRtsMessage(user, chatId, mimc::BYE_RESPONSE, chatSession.getChatType(), byeResponseBytesStr);

	

	return true;
}

bool RtsSendSignal::sendUpdateRequest(const User* user, uint64_t chatId) {
	const mimc::BindRelayResponse& bindRelayResponse = user->getBindRelayResponse();
	if (!bindRelayResponse.IsInitialized()) {
		
		return false;
	}

	std::string localIp;
	uint16_t localPort;
	if (user->getXmdTransceiver()->getLocalInfo(localIp, localPort) < 0) {
		
		return false;
	}
	mimc::UpdateRequest updateRequest;
	mimc::UserInfo* theUser = updateRequest.mutable_user();
	theUser->set_uuid(user->getUuid());
	theUser->set_resource(user->getResource());
	theUser->set_appid(user->getAppId());
	theUser->set_appaccount(user->getAppAccount());
	theUser->set_intranetip(localIp);
	theUser->set_intranetport(localPort);
	theUser->set_internetip(bindRelayResponse.internet_ip());
	theUser->set_internetport(bindRelayResponse.internet_port());
	theUser->set_relayip(bindRelayResponse.relay_ip());
	theUser->set_relayport(bindRelayResponse.relay_port());
	theUser->set_connid(user->getRelayConnId());

	int update_request_size = updateRequest.ByteSize();
	char updateRequestBytes[update_request_size];
	memset(updateRequestBytes, 0, update_request_size);
	updateRequest.SerializeToArray(updateRequestBytes, update_request_size);
	std::string updateRequestBytesStr(updateRequestBytes, update_request_size);

	P2PChatSession& chatSession = user->getCurrentChats()->at(chatId);

	std::string packetId = sendRtsMessage(user, chatId, mimc::UPDATE_REQUEST, chatSession.getChatType(), updateRequestBytesStr);
	

	chatSession.setChatState(WAIT_UPDATE_RESPONSE);
	chatSession.setLatestLegalChatStateTs(time(NULL));

	return true;
}

bool RtsSendSignal::sendUpdateResponse(const User* user, uint64_t chatId, mimc::RTSResult result) {
	mimc::UpdateResponse updateResponse;
	updateResponse.set_result(result);

	int update_response_size = updateResponse.ByteSize();
	char updateResponseBytes[update_response_size];
	memset(updateResponseBytes, 0, update_response_size);
	updateResponse.SerializeToArray(updateResponseBytes, update_response_size);
	std::string updateResponseBytesStr(updateResponseBytes, update_response_size);

	const P2PChatSession& chatSession = user->getCurrentChats()->at(chatId);

	std::string packetId = sendRtsMessage(user, chatId, mimc::UPDATE_RESPONSE, chatSession.getChatType(), updateResponseBytesStr);
	

	return true;
}

bool RtsSendSignal::pingChatManager(const User* user, uint64_t chatId) {
	const P2PChatSession& chatSession = user->getCurrentChats()->at(chatId);
	mimc::PingRequest pingRequest;
	int ping_request_size = pingRequest.ByteSize();
	char pingRequestBytes[ping_request_size];
	memset(pingRequestBytes, 0, ping_request_size);
	pingRequest.SerializeToArray(pingRequestBytes, ping_request_size);
	std::string pingRequestBytesStr(pingRequestBytes, ping_request_size);

	std::string packetId = sendRtsMessage(user, chatId, mimc::PING_REQUEST, chatSession.getChatType(), pingRequestBytesStr);

	

	return true;
}

std::string RtsSendSignal::sendRtsMessage(const User* user, uint64_t chatId, mimc::RTSMessageType messageType, mimc::ChatType chatType, std::string payload) {
	mimc::RTSMessage rtsMessage;
	rtsMessage.set_type(messageType);
	rtsMessage.set_chatid(chatId);
	rtsMessage.set_chattype(chatType);
	rtsMessage.set_uuid(user->getUuid());
	rtsMessage.set_resource(user->getResource());
	rtsMessage.set_payload(payload);

	int message_size = rtsMessage.ByteSize();
	char messageBytes[message_size];
	memset(messageBytes, 0, message_size);
	rtsMessage.SerializeToArray(messageBytes, message_size);
	std::string messageBytesStr(messageBytes, message_size);

	std::string packetId = user->getPacketManager()->createPacketId();
	mimc::MIMCPacket * v6payload = new mimc::MIMCPacket();
	v6payload->set_packetid(packetId);
	v6payload->set_package(user->getAppPackage());
	v6payload->set_type(mimc::RTS_SIGNAL);
	v6payload->set_payload(messageBytesStr);
	v6payload->set_timestamp(time(NULL));

	struct waitToSendContent mimc_obj;
	mimc_obj.cmd = BODY_CLIENTHEADER_CMD_SECMSG;
	mimc_obj.type = C2S_SINGLE_DIRECTION;
	mimc_obj.message = v6payload;
	(user->getPacketManager()->packetsWaitToSend).push(mimc_obj);

	return packetId;
}