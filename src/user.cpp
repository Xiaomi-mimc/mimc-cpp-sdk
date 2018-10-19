#include <mimc/user.h>
#include <mimc/connection.h>
#include <mimc/packet_manager.h>
#include <mimc/utils.h>
#include <mimc/threadsafe_queue.h>
#include <mimc/rts_connection_handler.h>
#include <mimc/rts_stream_handler.h>
#include <json-c/json.h>
#include <mimc/ims_push_service.pb.h>
#include <stdlib.h>
#include <unistd.h>

User::User(std::string appAccount, std::string resource) {
	this->appAccount = appAccount;
	this->testPacketLoss = 0;
	this->permitLogin = false;
	this->onlineStatus = Offline;
	this->resetRelayLinkState();
	if (resource == "") {
		resource = DEFAULT_RESOURCE;
	}
	this->resource = resource;
	this->lastLoginTimestamp = 0;
	this->lastCreateConnTimestamp = 0;
	this->lastPingTimestamp = 0;
	this->tokenFetcher = NULL;
	this->statusHandler = NULL;
	this->messageHandler = NULL;
	this->rtsCallEventHandler = NULL;
	this->rtsConnectionHandler = NULL;
	this->rtsStreamHandler = NULL;
	this->packetManager = new PacketManager();

	this->audioStreamConfig = new mimc::StreamConfig();
	this->audioStreamConfig->set_stream_strategy(mimc::ACK_STRATEGY);
	this->audioStreamConfig->set_ack_stream_wait_time_ms(ACK_STREAM_WAIT_TIME_MS);
	this->audioStreamConfig->set_stream_timeout_s(STREAM_TIMEOUT);
	this->audioStreamConfig->set_stream_is_encrypt(false);

	this->videoStreamConfig = new mimc::StreamConfig();
	this->videoStreamConfig->set_stream_strategy(mimc::FEC_STRATEGY);
	this->videoStreamConfig->set_stream_timeout_s(STREAM_TIMEOUT);
	this->videoStreamConfig->set_stream_is_encrypt(false);

	this->currentChats = new std::map<long, P2PChatSession>();
	this->onlaunchChats = new std::map<long, pthread_t>();
	this->xmdTranseiver = NULL;
	this->maxCallNum = 1;
	this->isDialCalling = false;
	this->isFirstDialCall = true;
	this->mutex = PTHREAD_MUTEX_INITIALIZER;

	this->conn = new Connection();
	this->conn->setUser(this);

	pthread_create(&sendThread, NULL, sendPacket, (void *)(this->conn));
	pthread_create(&receiveThread, NULL, receivePacket, (void *)(this->conn));
	pthread_create(&checkThread, NULL, checkTimeout, (void *)(this->conn));
}

User::~User() {
	pthread_cancel(sendThread);
	pthread_join(sendThread, NULL);
	pthread_cancel(receiveThread);
	pthread_join(receiveThread, NULL);
	pthread_cancel(checkThread);
	pthread_join(checkThread, NULL);

	if (!this->isFirstDialCall) {
		this->xmdTranseiver->stop();
		this->xmdTranseiver->join();
		this->isFirstDialCall = true;
	}

	delete this->audioStreamConfig;
	delete this->videoStreamConfig;
	delete this->packetManager;
	delete this->currentChats;
	delete this->onlaunchChats;
	delete this->xmdTranseiver;
	this->conn->resetSock();
	delete this->conn;
	delete this->tokenFetcher;
	delete this->statusHandler;
	delete this->messageHandler;
	delete this->rtsCallEventHandler;
	delete this->rtsConnectionHandler;
	delete this->rtsStreamHandler;
}

void * User::sendPacket(void *arg) {
	XMDLoggerWrapper::instance()->info("sendThread start to run");
	Connection *conn = (Connection *)arg;
	User * user = conn->getUser();
	unsigned char * packetBuffer = NULL;
	int packet_size = 0;
	MessageDirection msgType;
	while (true) {
		msgType = C2S_DOUBLE_DIRECTION;
		if (conn->getState() == NOT_CONNECTED) {

			long now = time(NULL);
			if (now - user->lastCreateConnTimestamp <= CONNECT_TIMEOUT) {
				usleep(200000);
				continue;
			}
			if (user->getTestPacketLoss() >= 100) {
				usleep(200000);
				continue;
			}
			user->lastCreateConnTimestamp = time(NULL);
			if (!conn->connect()) {
				XMDLoggerWrapper::instance()->error("In sendPacket, socket connect failed");
				continue;
			}
			conn->setState(SOCK_CONNECTED);
			XMDLoggerWrapper::instance()->info("In sendPacket, socket connect succeed");

			packet_size = user->getPacketManager()->encodeConnectionPacket(packetBuffer, conn);
			if (packet_size < 0) {

				continue;
			}
		}
		if (conn->getState() == SOCK_CONNECTED) {
			usleep(5000);
		}
		if (conn->getState() == HANDSHAKE_CONNECTED) {
			long now = time(NULL);
			if (user->getOnlineStatus() == Offline) {
				if (now - user->lastLoginTimestamp <= LOGIN_TIMEOUT) {
					usleep(200000);
					continue;
				} else {
					if (!user->permitLogin) {
						usleep(200000);
						continue;
					}

					packet_size = user->getPacketManager()->encodeBindPacket(packetBuffer, conn);
					if (packet_size < 0) {

						continue;
					}
					user->lastLoginTimestamp = time(NULL);
				}
			} else {
				struct waitToSendContent *obj;
				(user->getPacketManager()->packetsWaitToSend).pop(&obj);
				if (obj != NULL) {
					msgType = obj->type;

					if (obj->cmd == BODY_CLIENTHEADER_CMD_SECMSG) {
						packet_size = user->getPacketManager()->encodeSecMsgPacket(packetBuffer, conn, obj->message);
						if (packet_size < 0) {

							continue;
						}
					}
					else if (obj->cmd == BODY_CLIENTHEADER_CMD_UNBIND) {
						packet_size = user->getPacketManager()->encodeUnBindPacket(packetBuffer, conn);
						if (packet_size < 0) {

							continue;
						}
					}
				} else {
					if (now - user->lastPingTimestamp > PING_TIMEINTERVAL) {
						packet_size = user->getPacketManager()->encodePingPacket(packetBuffer, conn);
						if (packet_size < 0) {

							continue;
						}
					}
				}
			}
		}

		if (packetBuffer == NULL) {
			usleep(20000);
			continue;
		}

		if (msgType == C2S_DOUBLE_DIRECTION) {
			conn->trySetNextResetTs();
		}

		user->lastPingTimestamp = time(NULL);

		if (user->getTestPacketLoss() < 100) {
			int ret = conn->writen(conn->getSock(), (void *)packetBuffer, packet_size);
			if (ret != packet_size) {
				XMDLoggerWrapper::instance()->error("In sendPacket, ret != packet_size, ret=%d,packet_size=%d", ret, packet_size);
				conn->resetSock();
			}
			else {

			}
		}

		delete[] packetBuffer;
		packetBuffer = NULL;
	}
}

void * User::receivePacket(void *arg) {
	XMDLoggerWrapper::instance()->info("receiveThread start to run");
	Connection *conn = (Connection *)arg;
	User * user = conn->getUser();
	while (true) {
		if (conn->getState() == NOT_CONNECTED) {
			usleep(20000);
			continue;
		}

		unsigned char * packetHeaderBuffer = new unsigned char[HEADER_LENGTH];
		memset(packetHeaderBuffer, 0, HEADER_LENGTH);

		int ret = conn->readn(conn->getSock(), (void *)packetHeaderBuffer, HEADER_LENGTH);
		if (ret != HEADER_LENGTH) {

			conn->resetSock();

			delete[] packetHeaderBuffer;
			packetHeaderBuffer = NULL;

			continue;
		}
		else {
			int body_len = user->getPacketManager()->char2int(packetHeaderBuffer, HEADER_BODYLEN_OFFSET) + BODY_CRC_LEN;
			unsigned char * packetBodyBuffer = new unsigned char[body_len];
			memset(packetBodyBuffer, 0, body_len);
			ret = conn->readn(conn->getSock(), (void *)packetBodyBuffer, body_len);
			if (ret != body_len) {

				conn->resetSock();

				delete[] packetBodyBuffer;
				packetBodyBuffer = NULL;

				continue;
			}
			else {
				if (user->getTestPacketLoss() >= 100) {
					delete[] packetHeaderBuffer;
					packetHeaderBuffer = NULL;

					delete[] packetBodyBuffer;
					packetBodyBuffer = NULL;

					continue;
				}
				conn->clearNextResetSockTs();
				int packet_size = HEADER_LENGTH + body_len;
				unsigned char *packetBuffer = new unsigned char[packet_size];
				memset(packetBuffer, 0, packet_size);
				memmove(packetBuffer, packetHeaderBuffer, HEADER_LENGTH);
				memmove(packetBuffer + HEADER_LENGTH, packetBodyBuffer, body_len);

				delete[] packetHeaderBuffer;
				packetHeaderBuffer = NULL;

				delete[] packetBodyBuffer;
				packetBodyBuffer = NULL;

				ret = user->getPacketManager()->decodePacketAndHandle(packetBuffer, conn);
				delete[] packetBuffer;
				packetBuffer = NULL;
				if (ret < 0) {
					conn->resetSock();
				}
			}
		}
	}
}

void * User::checkTimeout(void *arg) {
	XMDLoggerWrapper::instance()->info("checkThread start to run");
	Connection *conn = (Connection *)arg;
	User * user = conn->getUser();
	while (true) {
		if ((conn->getNextResetSockTs() > 0) && (time(NULL) - conn->getNextResetSockTs() > 0)) {
			XMDLoggerWrapper::instance()->info("In checkTimeout, packet recv timeout");
			conn->resetSock();
		}
		user->getPacketManager()->checkMessageSendTimeout(user);
		user->rtsScanAndCallBack();
		user->relayConnScanAndCallBack();
		RtsSendData::sendPingRelayRequest(user);
		sleep(1);
	}
}

void User::relayConnScanAndCallBack() {
	if (this->relayLinkState != BEING_CREATED) {
		return;
	}
	if (time(NULL) - this->latestLegalRelayLinkStateTs < RELAY_CONN_TIMEOUT) {
		return;
	}

	if (this->relayConnId != 0) {
		this->xmdTranseiver->closeConnection(relayConnId);
	}
	for (std::map<long, P2PChatSession>::const_iterator iter = currentChats->begin(); iter != currentChats->end(); iter++) {
		long chatId = iter->first;
		XMDLoggerWrapper::instance()->error("In relayConnScanAndCallBack >= RELAY_CONN_TIMEOUT, chatId=%d", chatId);
		const P2PChatSession& chatSession = iter->second;
		if (chatSession.getChatState() == WAIT_SEND_UPDATE_REQUEST) {
			RtsSendSignal::sendByeRequest(this, chatId, UPDATE_TIMEOUT);
		}
	}
	this->currentChats->clear();

	this->resetRelayLinkState();
}

void User::rtsScanAndCallBack() {
	if (this->currentChats->empty()) {
		return;
	}

	for (std::map<long, P2PChatSession>::const_iterator iter = this->currentChats->begin(); iter != this->currentChats->end();) {
		const long& chatId = iter->first;
		const P2PChatSession& chatSession = iter->second;
		const ChatState& chatState = chatSession.getChatState();
		if (chatState == RUNNING) {
			XMDLoggerWrapper::instance()->info("In rtsScanAndCallBack, chatId %ld state RUNNING ping chatmanager, user is %s", chatId, appAccount.c_str());
			RtsSendSignal::pingChatManager(this, chatId);
			iter++;
			continue;
		}

		if (time(NULL) - chatSession.getLatestLegalChatStateTs() < RTS_CHECK_TIMEOUT) {
			iter++;
			continue;
		}

		if (chatState == WAIT_CREATE_RESPONSE) {
			if (time(NULL) - chatSession.getLatestLegalChatStateTs() >= RTS_CALL_TIMEOUT) {
				XMDLoggerWrapper::instance()->info("In rtsScanAndCallBack, chatId %ld state WAIT_CREATE_RESPONSE is timeout, user is %s", chatId, appAccount.c_str());
				iter = this->currentChats->erase(iter);
				rtsCallEventHandler->onAnswered(chatId, false, DIAL_CALL_TIMEOUT);
			} else {
				iter++;
			}
		} else if (chatState == WAIT_INVITEE_RESPONSE) {
			if (time(NULL) - chatSession.getLatestLegalChatStateTs() >= RTS_CALL_TIMEOUT) {
				XMDLoggerWrapper::instance()->info("In rtsScanAndCallBack, chatId %ld state WAIT_INVITEE_RESPONSE is timeout, user is %s", chatId, appAccount.c_str());
				if (this->onlaunchChats->count(chatId) > 0) {
					pthread_t onlaunchChatThread = this->onlaunchChats->at(chatId);
					pthread_cancel(onlaunchChatThread);
					this->onlaunchChats->erase(chatId);
				}
				iter = this->currentChats->erase(iter);
				rtsCallEventHandler->onClosed(chatId, INVITE_RESPONSE_TIMEOUT);
			} else {
				iter++;
			}
		} else if (chatState == WAIT_UPDATE_RESPONSE) {
			XMDLoggerWrapper::instance()->info("In rtsScanAndCallBack, chatId %ld state WAIT_UPDATE_RESPONSE is timeout, user is %s", chatId, appAccount.c_str());
			RtsSendSignal::sendByeRequest(this, chatId, UPDATE_TIMEOUT);
			iter = this->currentChats->erase(iter);
			rtsCallEventHandler->onClosed(chatId, UPDATE_TIMEOUT);
		} else {
			iter++;
		}
		RtsSendData::closeRelayConnWhenNoChat(this);
	}
}

void * User::onLaunched(void *arg) {
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	onLaunchedParam* param = (onLaunchedParam*)arg;
	User* user = param->user;
	long chatId = param->chatId;
	P2PChatSession& p2pChatSession = user->getCurrentChats()->at(chatId);
	const mimc::UserInfo& fromUser = p2pChatSession.getPeerUser();
	LaunchedResponse userResponse = user->getRTSCallEventHandler()->onLaunched(chatId, fromUser.appaccount(), p2pChatSession.getAppContent(), fromUser.resource());
	if (!userResponse.isAccepted()) {
		RtsSendSignal::sendInviteResponse(user, chatId, p2pChatSession.getChatType(), mimc::PEER_REFUSE, userResponse.getErrmsg());
		user->getCurrentChats()->erase(chatId);

		RtsSendData::closeRelayConnWhenNoChat(user);
		user->getOnlaunchChats()->erase(chatId);
		return 0;
	}

	XMDLoggerWrapper::instance()->info("In onLaunched thread, sendInviteResponse, chatId=%ld", chatId);
	RtsSendSignal::sendInviteResponse(user, chatId, p2pChatSession.getChatType(), mimc::SUCC, userResponse.getErrmsg());
	p2pChatSession.setChatState(RUNNING);
	p2pChatSession.setLatestLegalChatStateTs(time(NULL));
	delete param;
	param = NULL;
	//发送打洞请求

	user->getOnlaunchChats()->erase(chatId);
	return 0;
}

std::string User::join(const std::map<std::string, std::string>& kvs) const {
	if (kvs.empty()) {
		return "";
	}

	std::ostringstream oss;
	for (std::map<std::string, std::string>::const_iterator iter = kvs.begin(); iter != kvs.end(); iter++) {
		const std::string& key = iter->first;
		const std::string& value = iter->second;
		oss << key << ":" << value;
		if (iter != kvs.end()) {
			oss << ",";
		}
	}

	return oss.str();
}

std::string User::sendMessage(const std::string& toAppAccount, const std::string& msg) {
	if (this->onlineStatus == Offline || toAppAccount == "" || msg == "" || msg.size() > MIMC_MAX_PAYLOAD_SIZE) {
		return "";
	}
	return sendMessage(toAppAccount, msg, true);
}

std::string User::sendMessage(const std::string& toAppAccount, const std::string& msg, const bool isStore) {
	if (this->onlineStatus == Offline || toAppAccount == "" || msg == "" || msg.size() > MIMC_MAX_PAYLOAD_SIZE) {
		return "";
	}

	mimc::MIMCUser *from, *to;
	from = new mimc::MIMCUser();
	from->set_appid(this->appId);
	from->set_appaccount(this->appAccount);
	from->set_uuid(this->uuid);
	from->set_resource(this->resource);

	to = new mimc::MIMCUser();
	to->set_appid(this->appId);
	to->set_appaccount(toAppAccount);

	mimc::MIMCP2PMessage * message = new mimc::MIMCP2PMessage();
	message->set_allocated_from(from);
	message->set_allocated_to(to);
	message->set_payload(std::string(msg));
	message->set_isstore(isStore);

	int message_size = message->ByteSize();
	char * messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	message->SerializeToArray(messageBytes, message_size);
	std::string messageBytesStr(messageBytes, message_size);

	delete message;
	message = NULL;

	std::string packetId = this->packetManager->createPacketId();
	mimc::MIMCPacket * payload = new mimc::MIMCPacket();
	payload->set_packetid(packetId);
	payload->set_package(this->appPackage);
	payload->set_type(mimc::P2P_MESSAGE);
	payload->set_payload(messageBytesStr);
	payload->set_timestamp(time(NULL));

	delete[] messageBytes;
	messageBytes = NULL;

	MIMCMessage mimcMessage(packetId, payload->sequence(), this->appAccount, this->resource, toAppAccount, "", msg, payload->timestamp());
	(this->packetManager->packetsWaitToTimeout).insert(std::pair<std::string, MIMCMessage>(packetId, mimcMessage));

	struct waitToSendContent mimc_obj;
	mimc_obj.cmd = BODY_CLIENTHEADER_CMD_SECMSG;
	mimc_obj.type = C2S_DOUBLE_DIRECTION;
	mimc_obj.message = payload;
	(this->packetManager->packetsWaitToSend).push(mimc_obj);

	return packetId;
}

long User::dialCall(const std::string& toAppAccount, const std::string& appContent, const std::string& toResource) {
	pthread_mutex_lock(&mutex);
	if (this->isDialCalling) {
		XMDLoggerWrapper::instance()->warn("In dialCall, another thread is dialCall!");
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	this->isDialCalling = true;
	pthread_mutex_unlock(&mutex);

	if (this->rtsCallEventHandler == NULL) {
		XMDLoggerWrapper::instance()->error("In dialCall, rtsCallEventHandler is not registered!");
		this->isDialCalling = false;
		return -1;
	}
	if (toAppAccount == "") {
		XMDLoggerWrapper::instance()->error("In dialCall, toAppAccount can not be empty!");
		this->isDialCalling = false;
		return -1;
	}
	if (this->getOnlineStatus() == Offline) {
		XMDLoggerWrapper::instance()->warn("In dialCall, user:%ld is offline!", uuid);
		this->isDialCalling = false;
		return -1;
	}
	if (currentChats->size() == maxCallNum) {
		XMDLoggerWrapper::instance()->warn("In dialCall, currentChats size has reached maxCallNum %d!", maxCallNum);
		this->isDialCalling = false;
		return -1;
	}

	this->checkToRunXmdTranseiver();

	mimc::UserInfo toUser;
	toUser.set_appid(this->appId);
	toUser.set_appaccount(toAppAccount);
	if (toResource != "") {
		toUser.set_resource(toResource);
	}

	//判断是否是同一个chat
	for (std::map<long, P2PChatSession>::const_iterator iter = currentChats->begin(); iter != currentChats->end(); iter++) {
		const P2PChatSession& chatSession = iter->second;
		const mimc::UserInfo& session_toUser = chatSession.getPeerUser();
		if (toUser.appid() == session_toUser.appid() && toUser.appaccount() == session_toUser.appaccount() && toUser.resource() == session_toUser.resource()
		        && appContent == chatSession.getAppContent()) {
			XMDLoggerWrapper::instance()->warn("In dialCall, the dialCall is repeated!");
			this->isDialCalling = false;
			return -1;
		}
	}

	long chatId;
	do {
		chatId = Utils::generateRandomLong();
	} while (currentChats->count(chatId) > 0);

	XMDLoggerWrapper::instance()->info("In dialCall, chatId is %ld", chatId);
	if (this->relayLinkState == NOT_CREATED) {

		unsigned long relayConnId = RtsSendData::createRelayConn(this);
		XMDLoggerWrapper::instance()->info("In dialCall, relayConnId is %ld", relayConnId);
		if (relayConnId == 0) {

			this->isDialCalling = false;
			return -1;
		}
		currentChats->insert(std::pair<long, P2PChatSession>(chatId, P2PChatSession(chatId, toUser, mimc::SINGLE_CHAT, WAIT_SEND_CREATE_REQUEST, time(NULL), true, appContent)));

		this->isDialCalling = false;
		return chatId;
	} else if (this->relayLinkState == BEING_CREATED) {

		currentChats->insert(std::pair<long, P2PChatSession>(chatId, P2PChatSession(chatId, toUser, mimc::SINGLE_CHAT, WAIT_SEND_CREATE_REQUEST, time(NULL), true, appContent)));

		this->isDialCalling = false;
		return chatId;
	} else if (this->relayLinkState == SUCC_CREATED) {

		currentChats->insert(std::pair<long, P2PChatSession>(chatId, P2PChatSession(chatId, toUser, mimc::SINGLE_CHAT, WAIT_CREATE_RESPONSE, time(NULL), true, appContent)));

		RtsSendSignal::sendCreateRequest(this, chatId);
		this->isDialCalling = false;
		return chatId;
	} else {

		this->isDialCalling = false;
		return -1;
	}
}

bool User::sendRtsData(long chatId, const std::string& data, RtsDataType dataType, RtsChannelType channelType) {
	if (data.size() > RTS_MAX_PAYLOAD_SIZE) {

		return false;
	}
	if (dataType != AUDIO && dataType != VIDEO) {

		return false;
	}

	mimc::PKT_TYPE pktType = mimc::USER_DATA_AUDIO;
	if (dataType == VIDEO) {
		pktType = mimc::USER_DATA_VIDEO;
	}

	if (currentChats->count(chatId) == 0) {

		return false;
	}

	const P2PChatSession& chatSession = currentChats->at(chatId);
	if (chatSession.getChatState() != RUNNING) {

		return false;
	}
	if (onlineStatus == Offline) {

		return false;
	}
	if (channelType == RELAY) {


		return RtsSendData::sendRtsDataByRelay(this, chatId, data, pktType);
	}
	return true;
}

void User::closeCall(long chatId, std::string byeReason) {
	if (currentChats->count(chatId) == 0) {

		return;
	}

	XMDLoggerWrapper::instance()->info("In closeCall, chatId is %ld, byeReason is %s", chatId, byeReason.c_str());
	RtsSendSignal::sendByeRequest(this, chatId, byeReason);
	currentChats->erase(chatId);
	RtsSendData::closeRelayConnWhenNoChat(this);
	rtsCallEventHandler->onClosed(chatId, "CLOSED_INITIATIVELY");
}

bool User::login() {
	this->permitLogin = false;
	if (this->tokenFetcher == NULL || this->statusHandler == NULL || this->messageHandler == NULL) {
		XMDLoggerWrapper::instance()->info("login failed, user must register all essential callback functions first!");
	} else {
		std::string mimcToken = this->tokenFetcher->fetchToken();

		json_object *pobj = json_tokener_parse(mimcToken.c_str());
		json_object *dataobj = NULL;
		json_object *new_obj = NULL;
		json_object_object_get_ex(pobj, "data", &dataobj);
		json_object *retobj = NULL;
		json_object_object_get_ex(pobj, "code", &retobj);
		int retCode = json_object_get_int(retobj);
		if (retCode == 200) {
			const char *pstr = NULL;
			json_object_object_get_ex(dataobj, "appAccount", &new_obj);
			pstr = json_object_get_string(new_obj);
			std::string appAccount = pstr;
			if (this->appAccount == appAccount) {
				json_object_object_get_ex(dataobj, "appId", &new_obj);
				pstr = json_object_get_string(new_obj);
				this->appId = atol(pstr);
				json_object_object_get_ex(dataobj, "appPackage", &new_obj);
				pstr = json_object_get_string(new_obj);
				this->appPackage = pstr;
				json_object_object_get_ex(dataobj, "miChid", &new_obj);
				this->chid = json_object_get_int(new_obj);
				json_object_object_get_ex(dataobj, "miUserId", &new_obj);
				pstr = json_object_get_string(new_obj);
				this->uuid = atol(pstr);
				json_object_object_get_ex(dataobj, "miUserSecurityKey", &new_obj);
				pstr = json_object_get_string(new_obj);
				this->securityKey = pstr;
				json_object_object_get_ex(dataobj, "token", &new_obj);
				pstr = json_object_get_string(new_obj);
				this->token = pstr;

				this->permitLogin = true;
			}
		}

		json_object_put(pobj);
	}

	return this->permitLogin;
}

bool User::logout() {
	if (this->permitLogin == false) {
		return false;
	}

	this->permitLogin = false;

	for (std::map<long, P2PChatSession>::const_iterator iter = currentChats->begin(); iter != currentChats->end(); iter++) {
		long chatId = iter->first;
		if (this->onlaunchChats->count(chatId) > 0) {
			pthread_t onlaunchChatThread = this->onlaunchChats->at(chatId);
			pthread_cancel(onlaunchChatThread);
			this->onlaunchChats->erase(chatId);
		}
		RtsSendSignal::sendByeRequest(this, chatId, "LOGOUT");
	}

	struct waitToSendContent logout_obj;
	logout_obj.cmd = BODY_CLIENTHEADER_CMD_UNBIND;
	logout_obj.type = C2S_DOUBLE_DIRECTION;
	logout_obj.message = NULL;
	(this->packetManager->packetsWaitToSend).push(logout_obj);

	currentChats->clear();
	RtsSendData::closeRelayConnWhenNoChat(this);

	return true;
}

void User::resetRelayLinkState() {
	XMDLoggerWrapper::instance()->info("In resetRelayLinkState, relayConnId is %ld", this->relayConnId);
	this->relayConnId = 0;
	this->relayControlStreamId = 0;
	this->relayAudioStreamId = 0;
	this->relayVideoStreamId = 0;
	this->relayLinkState = NOT_CREATED;
	this->latestLegalRelayLinkStateTs = time(NULL);
}

void User::handleXMDConnClosed(unsigned long connId, ConnCloseType type) {
	if (connId == this->relayConnId) {
		XMDLoggerWrapper::instance()->warn("XMDConnection(RELAY) is closed abnormally, connId is %ld, ConnCloseType is %d", connId, type);
		resetRelayLinkState();
	} else {
		for (std::map<long, P2PChatSession>::iterator iter = currentChats->begin(); iter != currentChats->end(); iter++) {
			long chatId = iter->first;
			P2PChatSession& chatSession = iter->second;
			if (connId == chatSession.getP2PIntranetConnId()) {
				XMDLoggerWrapper::instance()->warn("XMDConnection(P2PIntranet) is closed abnormally, connId is %ld, ConnCloseType is %d, chatId is %ld", connId, type, chatId);
				chatSession.resetP2PIntranetConn();
			} else if (connId == chatSession.getP2PInternetConnId()) {
				XMDLoggerWrapper::instance()->warn("XMDConnection(P2PInternet) is closed abnormally, connId is %ld, ConnCloseType is %d, chatId is %ld", connId, type, chatId);
				chatSession.resetP2PInternetConn();
			}
		}
	}

	checkAndCloseChats();
}

void User::checkAndCloseChats() {
	if (this->relayConnId != 0) {
		return;
	}

	std::vector<long> chatIdsToRemove;
	for (std::map<long, P2PChatSession>::const_iterator iter = currentChats->begin(); iter != currentChats->end(); iter++) {
		long chatId = iter->first;
		if (getP2PIntranetConnId(chatId) == 0 && getP2PInternetConnId(chatId) == 0) {
			RtsSendSignal::sendByeRequest(this, chatId, ALL_DATA_CHANNELS_CLOSED);
			chatIdsToRemove.push_back(chatId);
			rtsCallEventHandler->onClosed(chatId, ALL_DATA_CHANNELS_CLOSED);
		}
	}

	int toRemoveSize = chatIdsToRemove.size();
	for (int i = 0; i < toRemoveSize; i++) {
		currentChats->erase(chatIdsToRemove[i]);
	}
}

unsigned long User::getP2PIntranetConnId(long chatId) const {
	if (currentChats->count(chatId) == 0) {
		return 0;
	}
	const P2PChatSession& chatSession = currentChats->at(chatId);
	return chatSession.getP2PIntranetConnId();
}

unsigned long User::getP2PInternetConnId(long chatId) const {
	if (currentChats->count(chatId) == 0) {
		return 0;
	}
	const P2PChatSession& chatSession = currentChats->at(chatId);
	return chatSession.getP2PInternetConnId();
}

void User::registerTokenFetcher(MIMCTokenFetcher* tokenFetcher) {
	this->tokenFetcher = tokenFetcher;
}

void User::registerOnlineStatusHandler(OnlineStatusHandler* handler) {
	this->statusHandler = handler;
}

OnlineStatusHandler* User::getStatusHandler() const {
	return this->statusHandler;
}

void User::registerMessageHandler(MessageHandler* handler) {
	this->messageHandler = handler;
}

MessageHandler* User::getMessageHandler() const {
	return this->messageHandler;
}

void User::registerRTSCallEventHandler(RTSCallEventHandler* handler) {
	this->rtsCallEventHandler = handler;
}

RTSCallEventHandler* User::getRTSCallEventHandler() const {
	return this->rtsCallEventHandler;
}

void User::checkToRunXmdTranseiver() {
	if (this->isFirstDialCall) {
		this->isFirstDialCall = false;
		this->xmdTranseiver = new XMDTransceiver(1);
		this->xmdTranseiver->setXMDLogLevel(XMD_INFO);
		this->rtsConnectionHandler = new RtsConnectionHandler(this);
		this->rtsStreamHandler = new RtsStreamHandler(this);
		this->xmdTranseiver->registerConnHandler(this->rtsConnectionHandler);
		this->xmdTranseiver->registerStreamHandler(this->rtsStreamHandler);
		this->xmdTranseiver->run();
		usleep(100000);
	}
}
