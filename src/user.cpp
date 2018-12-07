#include <mimc/user.h>
#include <mimc/connection.h>
#include <mimc/packet_manager.h>
#include <mimc/utils.h>
#include <mimc/threadsafe_queue.h>
#include <mimc/rts_connection_handler.h>
#include <mimc/rts_stream_handler.h>
#include <mimc/ims_push_service.pb.h>
#include <mimc/rts_context.h>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>

User::User(std::string appAccount, std::string resource, std::string cachePath) 
	:audioStreamConfig(ACK_TYPE, ACK_STREAM_WAIT_TIME_MS, false), videoStreamConfig(FEC_TYPE, ACK_STREAM_WAIT_TIME_MS, false) {
	XMDLoggerWrapper::instance()->setXMDLogLevel(XMD_INFO);
	this->appAccount = appAccount;
	this->testPacketLoss = 0;
	this->permitLogin = false;
	this->onlineStatus = Offline;
	this->tokenExpired = false;
	this->addressInvalid = false;
	this->tokenFetchSucceed = false;
	this->serverFetchSucceed = false;
	this->resetRelayLinkState();
	if (resource == "") {
		resource = "cpp_default";
	}
	this->cacheExist = false;
	if (cachePath == "") {
		char buffer[MAXPATHLEN];
		Utils::getCwd(buffer, MAXPATHLEN);
		cachePath = buffer;
		XMDLoggerWrapper::instance()->info("cachePath is %s", cachePath.c_str());
	}
	if (cachePath.back() == '/') {
		cachePath.pop_back();
	}
	this->cachePath = cachePath + '/' + appAccount + '/' + resource;
	this->cacheFile = this->cachePath + '/' + "mimc.info";
	createCacheFileIfNotExist(this);
	if (resource == "cpp_default") {
		this->resource = Utils::generateRandomString(8);
	} else {
		this->resource = resource;
	}

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

	this->xmdSendBufferSize = 0;
	this->xmdRecvBufferSize = 0;

	this->currentChats = new std::map<long, P2PChatSession>();
	this->onlaunchChats = new std::map<long, pthread_t>();
	this->xmdTranseiver = NULL;
	this->maxCallNum = 1;
	this->mutex_0 = PTHREAD_RWLOCK_INITIALIZER;
	this->mutex_1 = PTHREAD_MUTEX_INITIALIZER;

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

	if (this->xmdTranseiver) {
		this->xmdTranseiver->stop();
		this->xmdTranseiver->join();
	}

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

void* User::sendPacket(void *arg) {
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
				usleep(10000);
				continue;
			}
			if (user->getTestPacketLoss() >= 100) {
				usleep(10000);
				continue;
			}
			if (!fetchToken(user) || !fetchServerAddr(user)) {
				usleep(1000);
				continue;
			}
			user->lastCreateConnTimestamp = time(NULL);
			if (!conn->connect()) {
				XMDLoggerWrapper::instance()->error("In sendPacket, socket connect failed, user is %s", user->getAppAccount().c_str());
				continue;
			}

			conn->setState(SOCK_CONNECTED);

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
				if (now - user->lastLoginTimestamp > LOGIN_TIMEOUT || user->tokenExpired) {
					if (user->permitLogin && fetchToken(user)) {
						packet_size = user->getPacketManager()->encodeBindPacket(packetBuffer, conn);
						if (packet_size < 0) {

							continue;
						}
						user->lastLoginTimestamp = time(NULL);
					}
				}
			} else {
				struct waitToSendContent *obj;
				user->getPacketManager()->packetsWaitToSend.pop(&obj);
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
			usleep(10000);
			continue;
		}

		if (msgType == C2S_DOUBLE_DIRECTION) {
			conn->trySetNextResetTs();
		}

		user->lastPingTimestamp = time(NULL);

		if (user->getTestPacketLoss() < 100) {
			int ret = conn->writen((void *)packetBuffer, packet_size);
			if (ret != packet_size) {
				XMDLoggerWrapper::instance()->error("In sendPacket, ret != packet_size, ret=%d, packet_size=%d", ret, packet_size);
				conn->resetSock();
			}
			else {

			}
		}

		delete[] packetBuffer;
		packetBuffer = NULL;
	}
}

void* User::receivePacket(void *arg) {
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

		int ret = conn->readn((void *)packetHeaderBuffer, HEADER_LENGTH);
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
			ret = conn->readn((void *)packetBodyBuffer, body_len);
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

void* User::checkTimeout(void *arg) {
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
	pthread_rwlock_wrlock(&mutex_0);
	for (std::map<long, P2PChatSession>::const_iterator iter = currentChats->begin(); iter != currentChats->end(); iter++) {
		long chatId = iter->first;
		XMDLoggerWrapper::instance()->error("In relayConnScanAndCallBack >= RELAY_CONN_TIMEOUT, chatId=%d", chatId);
		const P2PChatSession& chatSession = iter->second;
		if (chatSession.getChatState() == WAIT_SEND_UPDATE_REQUEST) {
			RtsSendSignal::sendByeRequest(this, chatId, UPDATE_TIMEOUT);
		}
	}
	this->currentChats->clear();
	pthread_rwlock_unlock(&mutex_0);

	this->resetRelayLinkState();
}

void User::rtsScanAndCallBack() {
	pthread_rwlock_wrlock(&mutex_0);
	if (this->currentChats->empty()) {
		pthread_rwlock_unlock(&mutex_0);
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
				this->currentChats->erase(iter++);
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
				this->currentChats->erase(iter++);
				rtsCallEventHandler->onClosed(chatId, INVITE_RESPONSE_TIMEOUT);
			} else {
				iter++;
			}
		} else if (chatState == WAIT_UPDATE_RESPONSE) {
			XMDLoggerWrapper::instance()->info("In rtsScanAndCallBack, chatId %ld state WAIT_UPDATE_RESPONSE is timeout, user is %s", chatId, appAccount.c_str());
			RtsSendSignal::sendByeRequest(this, chatId, UPDATE_TIMEOUT);
			this->currentChats->erase(iter++);
			rtsCallEventHandler->onClosed(chatId, UPDATE_TIMEOUT);
		} else {
			iter++;
		}
		RtsSendData::closeRelayConnWhenNoChat(this);
	}

	pthread_rwlock_unlock(&mutex_0);
}

void* User::onLaunched(void *arg) {
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	onLaunchedParam* param = (onLaunchedParam*)arg;
	User* user = param->user;
	long chatId = param->chatId;
	mimc::UserInfo fromUser;
	std::string appContent;
	pthread_cleanup_push(cleanup, (void*)(&user->getChatsRwlock()));
	pthread_rwlock_rdlock(&user->getChatsRwlock());
	const P2PChatSession& p2pChatSession = user->getCurrentChats()->at(chatId);
	fromUser = p2pChatSession.getPeerUser();
	appContent = p2pChatSession.getAppContent();
	pthread_rwlock_unlock(&user->getChatsRwlock());
	pthread_cleanup_pop(0);

	LaunchedResponse userResponse = user->getRTSCallEventHandler()->onLaunched(chatId, fromUser.appaccount(), appContent, fromUser.resource());

	pthread_cleanup_push(cleanup, (void*)(&user->getChatsRwlock()));
	pthread_rwlock_wrlock(&user->getChatsRwlock());
	if (user->getCurrentChats()->count(chatId) == 0) {
		pthread_rwlock_unlock(&user->getChatsRwlock());
		return 0;
	}
	P2PChatSession& p2pChatSession = user->getCurrentChats()->at(chatId);
	if (!userResponse.isAccepted()) {
		RtsSendSignal::sendInviteResponse(user, chatId, p2pChatSession.getChatType(), mimc::PEER_REFUSE, userResponse.getErrmsg());
		user->getCurrentChats()->erase(chatId);
		RtsSendData::closeRelayConnWhenNoChat(user);
		user->getOnlaunchChats()->erase(chatId);
		pthread_rwlock_unlock(&user->getChatsRwlock());
		return 0;
	}
	RtsSendSignal::sendInviteResponse(user, chatId, p2pChatSession.getChatType(), mimc::SUCC, userResponse.getErrmsg());
	p2pChatSession.setChatState(RUNNING);
	p2pChatSession.setLatestLegalChatStateTs(time(NULL));
	pthread_rwlock_unlock(&user->getChatsRwlock());
	pthread_cleanup_pop(0);

	delete param;
	param = NULL;
	//发送打洞请求

	user->getOnlaunchChats()->erase(chatId);
	return 0;
}

void User::cleanup(void *arg) {
	pthread_rwlock_unlock((pthread_rwlock_t *)arg);
}

void User::createCacheFileIfNotExist(User * user) {
	if (Utils::createDirIfNotExist(user->getCachePath())) {
		std::fstream file;
		file.open(user->getCacheFile().c_str());
		if (file) {
			user->cacheExist = true;
			if (file.is_open()) {
				file.close();
			}
		} else {
			XMDLoggerWrapper::instance()->info("cacheFile is not exist, create now");
			file.open(user->getCacheFile().c_str(), std::ios::out);
			if (file) {
				XMDLoggerWrapper::instance()->info("cacheFile create succeed");
				user->cacheExist = true;
				if (file.is_open()) {
					file.close();
				}
			}
		}
	}
}

bool User::fetchToken(User * user) {
	if (user->tokenFetchSucceed && !user->tokenExpired) {
		return true;
	}
	const char* str = NULL;
	int str_size = 0;
	if (user->cacheExist) {
		std::ifstream file(user->getCacheFile(), std::ios::in | std::ios::ate);
		long size = file.tellg();
		file.seekg(0, std::ios::beg);
		char localStr[size];
		file.read(localStr, size);
		file.close();
		json_object* pobj_local;
		if (user->parseToken(localStr, pobj_local) && !user->tokenExpired) {
			json_object_put(pobj_local);
			return true;
		} else {
			if (user->getTokenFetcher() == NULL) {
				json_object_put(pobj_local);
				return false;
			}
			std::string remoteStr = user->getTokenFetcher()->fetchToken();
			json_object* pobj_remote;
			if (user->parseToken(remoteStr.c_str(), pobj_remote)) {
				user->tokenExpired = false;
				std::ofstream file(user->getCacheFile(), std::ios::out | std::ios::ate);
				if (file.is_open()) {
					if (pobj_local == NULL) {
						str = remoteStr.c_str();
						str_size = remoteStr.size();
					} else {
						str = json_object_get_string(pobj_local);
						json_object* dataobj = json_object_new_object();
						char s[20];
						json_object_object_add(dataobj, "appId", json_object_new_string(Utils::ltoa(user->getAppId(), s)));
						json_object_object_add(dataobj, "appPackage", json_object_new_string(user->getAppPackage().c_str()));
						json_object_object_add(dataobj, "appAccount", json_object_new_string(user->getAppAccount().c_str()));
						json_object_object_add(dataobj, "miChid", json_object_new_int(user->getChid()));
						json_object_object_add(dataobj, "miUserId", json_object_new_string(Utils::ltoa(user->getUuid(), s)));
						json_object_object_add(dataobj, "miUserSecurityKey", json_object_new_string(user->getSecurityKey().c_str()));
						json_object_object_add(dataobj, "token", json_object_new_string(user->getToken().c_str()));
						json_object_object_add(dataobj, "regionBucket", json_object_new_int(user->getRegionBucket()));
						json_object_object_add(dataobj, "feDomainName", json_object_new_string(user->getFeDomain().c_str()));
						json_object_object_add(dataobj, "relayDomainName", json_object_new_string(user->getRelayDomain().c_str()));
						json_object_object_add(pobj_local, "code", json_object_new_int(200));
						json_object_object_add(pobj_local, "data", dataobj);

						str = json_object_get_string(pobj_local);
						str_size = strlen(str);
					}
					file.write(str, str_size);
					file.close();
				}
				json_object_put(pobj_local);
				json_object_put(pobj_remote);
				return true;
			} else {
				json_object_put(pobj_local);
				json_object_put(pobj_remote);
				return false;
			}
		}
	} else {
		if (user->getTokenFetcher() == NULL) {
			return false;
		}
		std::string remoteStr = user->getTokenFetcher()->fetchToken();
		json_object* pobj_remote;
		if (user->parseToken(remoteStr.c_str(), pobj_remote)) {
			user->tokenExpired = false;
			createCacheFileIfNotExist(user);
			if (user->cacheExist) {
				std::ofstream file(user->getCacheFile(), std::ios::out | std::ios::ate);
				if (file.is_open()) {
					str = remoteStr.c_str();
					str_size = remoteStr.size();
					file.write(str, str_size);
					file.close();
				}
			}
			json_object_put(pobj_remote);
			return true;
		} else {
			json_object_put(pobj_remote);
			return false;
		}
	}
}

bool User::fetchServerAddr(User* user) {
	pthread_mutex_lock(&user->mutex_1);
	if (user->serverFetchSucceed && !user->addressInvalid) {
		pthread_mutex_unlock(&user->mutex_1);
		return true;
	}
	if (user->feDomain == "" || user->relayDomain == "") {
		XMDLoggerWrapper::instance()->error("User::fetchServerAddr, feDomain or relayDomain is empty");
		pthread_mutex_unlock(&user->mutex_1);
		return false;
	}
	const char* str = NULL;
	int str_size = 0;
	if (user->cacheExist) {
		std::ifstream file(user->getCacheFile(), std::ios::in | std::ios::ate);
		long size = file.tellg();
		file.seekg(0, std::ios::beg);
		char localStr[size];
		file.read(localStr, size);
		file.close();
		json_object* pobj_local;
		if (user->parseServerAddr(localStr, pobj_local) && !user->addressInvalid) {
			json_object_put(pobj_local);
			pthread_mutex_unlock(&user->mutex_1);
			return true;
		} else {
			std::string remoteStr = ServerFetcher::fetchServerAddr(RESOLVER_URL, user->feDomain + "," + user->relayDomain);
			json_object* pobj_remote;
			if (user->parseServerAddr(remoteStr.c_str(), pobj_remote)) {
				user->addressInvalid = false;
				std::ofstream file(user->getCacheFile(), std::ios::out | std::ios::ate);
				if (file.is_open()) {
					if (pobj_local == NULL) {
						str = remoteStr.c_str();
						str_size = remoteStr.size();
					} else {
						json_object* feAddrArray = json_object_new_array();
						for (std::vector<std::string>::const_iterator iter = user->feAddresses.begin(); iter != user->feAddresses.end(); iter++) {
							std::string feAddr = *iter;
							json_object_array_add(feAddrArray, json_object_new_string(feAddr.c_str()));
						}
						json_object* relayAddrArray = json_object_new_array();
						for (std::vector<std::string>::const_iterator iter = user->relayAddresses.begin(); iter != user->relayAddresses.end(); iter++) {
							std::string relayAddr = *iter;
							json_object_array_add(relayAddrArray, json_object_new_string(relayAddr.c_str()));
						}

						json_object_object_add(pobj_local, user->feDomain.c_str(), feAddrArray);
						json_object_object_add(pobj_local, user->relayDomain.c_str(), relayAddrArray);

						str = json_object_get_string(pobj_local);
						str_size = strlen(str);
					}
					file.write(str, str_size);
					file.close();
				}
				json_object_put(pobj_local);
				json_object_put(pobj_remote);
				pthread_mutex_unlock(&user->mutex_1);
				return true;
			} else {
				json_object_put(pobj_local);
				json_object_put(pobj_remote);
				pthread_mutex_unlock(&user->mutex_1);
				return false;
			}
		}
	} else {
		std::string remoteStr = ServerFetcher::fetchServerAddr(RESOLVER_URL, user->feDomain + "," + user->relayDomain);
		json_object* pobj_remote;
		if (user->parseServerAddr(remoteStr.c_str(), pobj_remote)) {
			user->addressInvalid = false;
			createCacheFileIfNotExist(user);
			if (user->cacheExist) {
				std::ofstream file(user->getCacheFile(), std::ios::out | std::ios::ate);
				if (file.is_open()) {
					str = remoteStr.c_str();
					str_size = remoteStr.size();
					file.write(str, str_size);
					file.close();
				}
			}
			json_object_put(pobj_remote);
			pthread_mutex_unlock(&user->mutex_1);
			return true;
		} else {
			json_object_put(pobj_remote);
			pthread_mutex_unlock(&user->mutex_1);
			return false;
		}
	}
}

bool User::parseToken(const char* str, json_object*& pobj) {
	tokenFetchSucceed = false;

	pobj = json_tokener_parse(str);

	if (pobj == NULL) {
		XMDLoggerWrapper::instance()->info("User::parseToken, json_tokener_parse failed, pobj is NULL");
	}

	json_object* retobj = NULL;
	json_object_object_get_ex(pobj, "code", &retobj);
	int retCode = json_object_get_int(retobj);
	if (retCode == 200) {
		json_object* dataobj = NULL;
		json_object_object_get_ex(pobj, "data", &dataobj);
		json_object* dataitem_obj = NULL;
		const char* pstr = NULL;
		json_object_object_get_ex(dataobj, "appAccount", &dataitem_obj);
		pstr = json_object_get_string(dataitem_obj);
		if (!pstr) {
			return tokenFetchSucceed;
		}
		std::string appAccount = pstr;
		if (this->appAccount == appAccount) {
			json_object_object_get_ex(dataobj, "appId", &dataitem_obj);
			pstr = json_object_get_string(dataitem_obj);
			if (!pstr) {
				return tokenFetchSucceed;
			} else {
				this->appId = atol(pstr);
			}
			json_object_object_get_ex(dataobj, "appPackage", &dataitem_obj);
			pstr = json_object_get_string(dataitem_obj);
			if (!pstr) {
				return tokenFetchSucceed;
			} else {
				this->appPackage = pstr;
			}
			json_object_object_get_ex(dataobj, "miChid", &dataitem_obj);
			this->chid = json_object_get_int(dataitem_obj);
			json_object_object_get_ex(dataobj, "miUserId", &dataitem_obj);
			pstr = json_object_get_string(dataitem_obj);
			if (!pstr) {
				return tokenFetchSucceed;
			} else {
				this->uuid = atol(pstr);
			}
			json_object_object_get_ex(dataobj, "miUserSecurityKey", &dataitem_obj);
			pstr = json_object_get_string(dataitem_obj);
			if (!pstr) {
				return tokenFetchSucceed;
			} else {
				this->securityKey = pstr;
			}
			json_object_object_get_ex(dataobj, "token", &dataitem_obj);
			pstr = json_object_get_string(dataitem_obj);
			if (!pstr) {
				return tokenFetchSucceed;
			} else {
				this->token = pstr;
			}
			json_object_object_get_ex(dataobj, "regionBucket", &dataitem_obj);
			this->regionBucket = json_object_get_int(dataitem_obj);
			json_object_object_get_ex(dataobj, "feDomainName", &dataitem_obj);
			pstr = json_object_get_string(dataitem_obj);
			if (!pstr) {
				return tokenFetchSucceed;
			} else {
				this->feDomain = pstr;
			}
			json_object_object_get_ex(dataobj, "relayDomainName", &dataitem_obj);
			pstr = json_object_get_string(dataitem_obj);
			if (!pstr) {
				return tokenFetchSucceed;
			} else {
				this->relayDomain = pstr;
			}
			tokenFetchSucceed = true;
		}
	}
	if (tokenFetchSucceed) {
		XMDLoggerWrapper::instance()->info("User::parseToken, tokenFetchSucceed is true, user is %s", appAccount.c_str());
	} else {
		XMDLoggerWrapper::instance()->info("User::parseToken, tokenFetchSucceed is false, user is %s", appAccount.c_str());
	}
	
	return tokenFetchSucceed;
}

bool User::parseServerAddr(const char* str, json_object*& pobj) {
	serverFetchSucceed = false;

	pobj = json_tokener_parse(str);

	if (pobj == NULL) {
		XMDLoggerWrapper::instance()->info("User::parseServerAddr, json_tokener_parse failed, pobj is NULL");
	}

	json_object* dataobj = NULL;
	json_object_object_get_ex(pobj, this->feDomain.c_str(), &dataobj);
	if (dataobj && json_object_get_type(dataobj) == json_type_array) {
		int arraySize = json_object_array_length(dataobj);
		if (arraySize == 0) {
			return serverFetchSucceed;
		}
		this->feAddresses.clear();
		for (int i = 0; i < arraySize; i++) {
			json_object* dataitem_obj = json_object_array_get_idx(dataobj, i);
			const char* feAddress = json_object_get_string(dataitem_obj);
			this->feAddresses.push_back(feAddress);
		}
	} else {
		return serverFetchSucceed;
	}

	json_object_object_get_ex(pobj, this->relayDomain.c_str(), &dataobj);
	if (dataobj && json_object_get_type(dataobj) == json_type_array) {
		int arraySize = json_object_array_length(dataobj);
		if (arraySize == 0) {
			return serverFetchSucceed;
		}
		this->relayAddresses.clear();
		for (int i = 0; i < arraySize; i++) {
			json_object* dataitem_obj = json_object_array_get_idx(dataobj, i);
			const char* relayAddress = json_object_get_string(dataitem_obj);
			this->relayAddresses.push_back(relayAddress);
		}
	} else {
		return serverFetchSucceed;
	}

	serverFetchSucceed = true;

	if (serverFetchSucceed) {
		XMDLoggerWrapper::instance()->info("User::parseServerAddr, serverFetchSucceed is true, user is %s", appAccount.c_str());
	} else {
		XMDLoggerWrapper::instance()->info("User::parseServerAddr, serverFetchSucceed is false, user is %s", appAccount.c_str());
	}

	return serverFetchSucceed;
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

std::string User::sendMessage(const std::string & toAppAccount, const std::string & msg, const std::string & bizType, const bool isStore) {
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
	message->set_biztype(bizType);
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

	MIMCMessage mimcMessage(packetId, payload->sequence(), this->appAccount, this->resource, toAppAccount, "", msg, bizType, payload->timestamp());
	(this->packetManager->packetsWaitToTimeout).insert(std::pair<std::string, MIMCMessage>(packetId, mimcMessage));

	struct waitToSendContent mimc_obj;
	mimc_obj.cmd = BODY_CLIENTHEADER_CMD_SECMSG;
	mimc_obj.type = C2S_DOUBLE_DIRECTION;
	mimc_obj.message = payload;
	(this->packetManager->packetsWaitToSend).push(mimc_obj);

	return packetId;
}

long User::dialCall(const std::string & toAppAccount, const std::string & appContent, const std::string & toResource) {
	if (this->rtsCallEventHandler == NULL) {
		XMDLoggerWrapper::instance()->error("In dialCall, rtsCallEventHandler is not registered!");
		return -1;
	}
	if (toAppAccount == "") {
		XMDLoggerWrapper::instance()->error("In dialCall, toAppAccount can not be empty!");
		return -1;
	}
	if (this->getOnlineStatus() == Offline) {
		XMDLoggerWrapper::instance()->warn("In dialCall, user:%ld is offline!", uuid);
		return -1;
	}

	pthread_rwlock_wrlock(&mutex_0);
	if (currentChats->size() == maxCallNum) {
		XMDLoggerWrapper::instance()->warn("In dialCall, currentChats size has reached maxCallNum %d!", maxCallNum);
		pthread_rwlock_unlock(&mutex_0);
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
			pthread_rwlock_unlock(&mutex_0);
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
			XMDLoggerWrapper::instance()->error("In dialCall, launch create relay conn failed!");
			pthread_rwlock_unlock(&mutex_0);
			return -1;
		}
		currentChats->insert(std::pair<long, P2PChatSession>(chatId, P2PChatSession(chatId, toUser, mimc::SINGLE_CHAT, WAIT_SEND_CREATE_REQUEST, time(NULL), true, appContent)));

		pthread_rwlock_unlock(&mutex_0);
		return chatId;
	} else if (this->relayLinkState == BEING_CREATED) {

		currentChats->insert(std::pair<long, P2PChatSession>(chatId, P2PChatSession(chatId, toUser, mimc::SINGLE_CHAT, WAIT_SEND_CREATE_REQUEST, time(NULL), true, appContent)));

		pthread_rwlock_unlock(&mutex_0);
		return chatId;
	} else if (this->relayLinkState == SUCC_CREATED) {

		currentChats->insert(std::pair<long, P2PChatSession>(chatId, P2PChatSession(chatId, toUser, mimc::SINGLE_CHAT, WAIT_CREATE_RESPONSE, time(NULL), true, appContent)));

		RtsSendSignal::sendCreateRequest(this, chatId);
		pthread_rwlock_unlock(&mutex_0);
		return chatId;
	} else {

		pthread_rwlock_unlock(&mutex_0);
		return -1;
	}
}

bool User::sendRtsData(long chatId, const std::string & data, const RtsDataType dataType, const RtsChannelType channelType, const std::string& ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
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
	pthread_rwlock_rdlock(&mutex_0);
	if (currentChats->count(chatId) == 0) {
		pthread_rwlock_unlock(&mutex_0);
		return false;
	}

	const P2PChatSession& chatSession = currentChats->at(chatId);
	if (chatSession.getChatState() != RUNNING) {
		pthread_rwlock_unlock(&mutex_0);
		return false;
	}
	if (onlineStatus == Offline) {
		pthread_rwlock_unlock(&mutex_0);
		return false;
	}

	RtsContext* rtsContext = new RtsContext(chatId, ctx);

	if (channelType == RELAY) {

		bool result = RtsSendData::sendRtsDataByRelay(this, chatId, data, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
		pthread_rwlock_unlock(&mutex_0);
		return result;
	}
	pthread_rwlock_unlock(&mutex_0);
	return true;
}

void User::closeCall(long chatId, std::string byeReason) {
	pthread_rwlock_wrlock(&mutex_0);
	if (currentChats->count(chatId) == 0) {
		pthread_rwlock_unlock(&mutex_0);
		return;
	}

	XMDLoggerWrapper::instance()->info("In closeCall, chatId is %ld, byeReason is %s", chatId, byeReason.c_str());
	RtsSendSignal::sendByeRequest(this, chatId, byeReason);
	if (onlaunchChats->count(chatId) > 0) {
		pthread_t onlaunchChatThread = onlaunchChats->at(chatId);
		if (onlaunchChatThread != pthread_self()) {
			pthread_cancel(onlaunchChatThread);
			onlaunchChats->erase(chatId);
		}
	}
	currentChats->erase(chatId);
	RtsSendData::closeRelayConnWhenNoChat(this);
	rtsCallEventHandler->onClosed(chatId, "CLOSED_INITIATIVELY");
	pthread_rwlock_unlock(&mutex_0);
}

bool User::login() {
	if (onlineStatus == Online) {
		return true;
	}
	if (tokenFetcher == NULL || statusHandler == NULL || messageHandler == NULL) {
		XMDLoggerWrapper::instance()->warn("login failed, user must register all essential callback functions first!");
		return false;
	}
	permitLogin = true;
	return true;
}

bool User::logout() {
	if (onlineStatus == Offline) {
		return true;
	}
	pthread_rwlock_wrlock(&mutex_0);
	for (std::map<long, P2PChatSession>::const_iterator iter = currentChats->begin(); iter != currentChats->end(); iter++) {
		long chatId = iter->first;
		if (onlaunchChats->count(chatId) > 0) {
			pthread_t onlaunchChatThread = onlaunchChats->at(chatId);
			if (onlaunchChatThread != pthread_self()) {
				pthread_cancel(onlaunchChatThread);
				onlaunchChats->erase(chatId);
			}
		}
		RtsSendSignal::sendByeRequest(this, chatId, "LOGOUT");
	}

	struct waitToSendContent logout_obj;
	logout_obj.cmd = BODY_CLIENTHEADER_CMD_UNBIND;
	logout_obj.type = C2S_DOUBLE_DIRECTION;
	logout_obj.message = NULL;
	packetManager->packetsWaitToSend.push(logout_obj);

	currentChats->clear();
	RtsSendData::closeRelayConnWhenNoChat(this);

	permitLogin = false;

	pthread_rwlock_unlock(&mutex_0);
	return true;
}

void User::resetRelayLinkState() {
	XMDLoggerWrapper::instance()->info("In resetRelayLinkState, relayConnId to be reset is %ld", this->relayConnId);
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
		pthread_rwlock_rdlock(&mutex_0);
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
		pthread_rwlock_unlock(&mutex_0);
	}

	checkAndCloseChats();
}

void User::checkAndCloseChats() {
	if (this->relayConnId != 0) {
		return;
	}

	pthread_rwlock_wrlock(&mutex_0);
	for (std::map<long, P2PChatSession>::iterator iter = currentChats->begin(); iter != currentChats->end();) {
		long chatId = iter->first;
		const P2PChatSession& chatSession = currentChats->at(chatId);
		if (chatSession.getP2PIntranetConnId() == 0 && chatSession.getP2PInternetConnId() == 0) {
			RtsSendSignal::sendByeRequest(this, chatId, ALL_DATA_CHANNELS_CLOSED);
			if (onlaunchChats->count(chatId) > 0) {
				pthread_t onlaunchChatThread = onlaunchChats->at(chatId);
				if (onlaunchChatThread != pthread_self()) {
					pthread_cancel(onlaunchChatThread);
					onlaunchChats->erase(chatId);
				}
			}
			currentChats->erase(iter++);
			rtsCallEventHandler->onClosed(chatId, ALL_DATA_CHANNELS_CLOSED);
		} else {
			iter++;
		}
	}
	pthread_rwlock_unlock(&mutex_0);
}

unsigned long User::getP2PIntranetConnId(long chatId) {
	pthread_rwlock_rdlock(&mutex_0);
	if (currentChats->count(chatId) == 0) {
		pthread_rwlock_unlock(&mutex_0);
		return 0;
	}
	const P2PChatSession& chatSession = currentChats->at(chatId);
	pthread_rwlock_unlock(&mutex_0);
	return chatSession.getP2PIntranetConnId();
}

unsigned long User::getP2PInternetConnId(long chatId) {
	pthread_rwlock_rdlock(&mutex_0);
	if (currentChats->count(chatId) == 0) {
		pthread_rwlock_unlock(&mutex_0);
		return 0;
	}
	const P2PChatSession& chatSession = currentChats->at(chatId);
	pthread_rwlock_unlock(&mutex_0);
	return chatSession.getP2PInternetConnId();
}

void User::registerTokenFetcher(MIMCTokenFetcher * tokenFetcher) {
	this->tokenFetcher = tokenFetcher;
}

MIMCTokenFetcher* User::getTokenFetcher() const {
	return this->tokenFetcher;
}

void User::registerOnlineStatusHandler(OnlineStatusHandler * handler) {
	this->statusHandler = handler;
}

OnlineStatusHandler* User::getStatusHandler() const {
	return this->statusHandler;
}

void User::registerMessageHandler(MessageHandler * handler) {
	this->messageHandler = handler;
}

MessageHandler* User::getMessageHandler() const {
	return this->messageHandler;
}

void User::registerRTSCallEventHandler(RTSCallEventHandler * handler) {
	this->rtsCallEventHandler = handler;
}

RTSCallEventHandler* User::getRTSCallEventHandler() const {
	return this->rtsCallEventHandler;
}

void User::checkToRunXmdTranseiver() {
	if (!this->xmdTranseiver) {
		this->xmdTranseiver = new XMDTransceiver(1);
		this->rtsConnectionHandler = new RtsConnectionHandler(this);
		this->rtsStreamHandler = new RtsStreamHandler(this);
		this->xmdTranseiver->registerConnHandler(this->rtsConnectionHandler);
		this->xmdTranseiver->registerStreamHandler(this->rtsStreamHandler);
		this->xmdTranseiver->setTestPacketLoss(this->testPacketLoss);
		if (this->xmdSendBufferSize > 0) {
			this->xmdTranseiver->setSendBufferSize(this->xmdSendBufferSize);
		}
		if (this->xmdRecvBufferSize > 0) {
			this->xmdTranseiver->setRecvBufferSize(this->xmdRecvBufferSize);
		}
		this->xmdTranseiver->run();
		usleep(100000);
	}
}
