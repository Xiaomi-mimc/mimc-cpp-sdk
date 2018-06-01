#include <mimc/user.h>
#include <mimc/connection.h>
#include <mimc/packet_manager.h>
#include <mimc/utils.h>
#include <mimc/threadsafe_queue.h>
#include <json/json.h>
#include <mimc/ims_push_service.pb.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/stringhelper.h>

#define LOG4CPLUS_CONF_FILE "mimc-cpp-sdk.properties"

User::User(std::string appAccount, std::string resource) {
	log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(LOG4CPLUS_CONF_FILE));
	this->appAccount = appAccount;
	this->permitLogin = false;
	this->onlineStatus = Offline;
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
	this->packetManager = new PacketManager();

	Connection *conn = new Connection();
	conn->setUser(this);

	pthread_create(&sendThread, NULL, sendPacket, (void *)conn);
	pthread_create(&receiveThread, NULL, receivePacket, (void *)conn);
	pthread_create(&checkThread, NULL, checkTimeout, (void *)conn);
}

User::~User() {
	pthread_cancel(sendThread);
	pthread_join(sendThread, NULL);
	pthread_cancel(receiveThread);
	pthread_join(receiveThread, NULL);
	pthread_cancel(checkThread);
	pthread_join(checkThread, NULL);

	delete this->packetManager;
	delete this->tokenFetcher;
	delete this->statusHandler;
	delete this->messageHandler;
}

void * User::sendPacket(void *arg) {
	LOG4CPLUS_INFO(LOGGER, "sendThread start");
	Connection *conn = (Connection *)arg;
	User * user = conn->getUser();
	unsigned char * packetBuffer = NULL;
	int packet_size = 0;
	MessageDirection msgType;
	while (true) {
		msgType = C2S_DOUBLE_DIRECTION;
		if (conn->getState() == NOT_CONNECTED) {
			LOG4CPLUS_INFO(LOGGER, "NOT_CONNECTED");
			long now = time(NULL);
			if (now - user->lastCreateConnTimestamp <= CONNECT_TIMEOUT) {
				usleep(200000);
				continue;
			}
			user->lastCreateConnTimestamp = time(NULL);
			if (!conn->connect()) {
				LOG4CPLUS_ERROR(LOGGER, "conn connect error");
				continue;
			}
			conn->setState(SOCK_CONNECTED);
			LOG4CPLUS_INFO(LOGGER, "SOCK_CONNECTED");

			LOG4CPLUS_INFO(LOGGER, "send cmd is " << BODY_CLIENTHEADER_CMD_CONN);
			packet_size = user->getPacketManager()->encodeConnectionPacket(packetBuffer, conn);
			if (packet_size < 0) {
				LOG4CPLUS_ERROR(LOGGER, "connectionPacket build error");
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
					LOG4CPLUS_INFO(LOGGER, "send cmd is " << BODY_CLIENTHEADER_CMD_BIND);
					packet_size = user->getPacketManager()->encodeBindPacket(packetBuffer, conn);
					if (packet_size < 0) {
						LOG4CPLUS_ERROR(LOGGER, "bindPacket build error");
						continue;
					}
					user->lastLoginTimestamp = time(NULL);
				}
			} else {
				struct waitToSendContent *obj;
				(user->getPacketManager()->packetsWaitToSend).pop(&obj);
				if (obj != NULL) {
					msgType = obj->type;
					LOG4CPLUS_INFO(LOGGER, "send cmd is " << obj->cmd);
					if (obj->cmd == BODY_CLIENTHEADER_CMD_SECMSG) {
						packet_size = user->getPacketManager()->encodeSecMsgPacket(packetBuffer, conn, obj->message);
						if (packet_size < 0) {
							LOG4CPLUS_ERROR(LOGGER, "secMsgPacket build error");
							continue;
						}
					}
					else if (obj->cmd == BODY_CLIENTHEADER_CMD_UNBIND) {
						packet_size = user->getPacketManager()->encodeUnBindPacket(packetBuffer, conn);
						if (packet_size < 0) {
							LOG4CPLUS_ERROR(LOGGER, "unBindPacket build error");
							continue;
						}
					}
				} else {
					if (now - user->lastPingTimestamp > PING_TIMEINTERVAL) {
						packet_size = user->getPacketManager()->encodePingPacket(packetBuffer, conn);
						if (packet_size < 0) {
							LOG4CPLUS_ERROR(LOGGER, "pingPacket build error");
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

		int ret = conn->writen(conn->getSock(), (void *)packetBuffer, packet_size);
		LOG4CPLUS_INFO(LOGGER, "packet_size == " << packet_size << ", ret == " << ret);

		delete[] packetBuffer;
		packetBuffer = NULL;

		if (ret != packet_size) {
			conn->resetSock();
		}
		else {
			LOG4CPLUS_INFO(LOGGER, "packet send succeed");
		}
	}
}

void * User::receivePacket(void *arg) {
	LOG4CPLUS_INFO(LOGGER, "receiveThread start");
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
			LOG4CPLUS_ERROR(LOGGER, "ret != HEADER_LENGTH, ret = " << ret);
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
				LOG4CPLUS_ERROR(LOGGER, "ret != body_len, ret is " << ret << ", body_len is " << body_len);
				conn->resetSock();

				delete[] packetBodyBuffer;
				packetBodyBuffer = NULL;

				continue;
			}
			else {
				LOG4CPLUS_INFO(LOGGER, "packet received succeed");
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
	LOG4CPLUS_INFO(LOGGER, "checkThread start");
	Connection *conn = (Connection *)arg;
	User * user = conn->getUser();
	while (true) {
		if ((conn->getNextResetSockTs() > 0) && (time(NULL) - conn->getNextResetSockTs() > RESETSOCK_TIMEOUT)) {
			LOG4CPLUS_WARN(LOGGER, "Network latency exceed!Reconnect!");
			conn->resetSock();
		}
		user->getPacketManager()->checkMessageSendTimeout(user);
		usleep(200000);
	}
}

std::string User::join(std::map<std::string, std::string> kvs) const {
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
	if (toAppAccount == "" || msg == "" || msg.size() > MIMC_MAX_PAYLOAD_SIZE) {
		return "";
	}
	return sendMessage(toAppAccount, msg, true);
}

std::string User::sendMessage(const std::string& toAppAccount, const std::string& msg, const bool isStore) {
	if (toAppAccount == "" || msg == "" || msg.size() > MIMC_MAX_PAYLOAD_SIZE) {
		return "";
	}

	mimc::MIMCUser *from, *to;
	from = new mimc::MIMCUser();
	from->set_appid(atol((this->appId).c_str()));
	from->set_appaccount(this->appAccount);
	from->set_uuid(this->uuid);
	from->set_resource(this->resource);

	to = new mimc::MIMCUser();
	to->set_appid(atol((this->appId).c_str()));
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

bool User::login() {
	this->permitLogin = false;
	if (this->tokenFetcher == NULL || this->statusHandler == NULL || this->messageHandler == NULL) {
		LOG4CPLUS_ERROR(LOGGER, "login Failed! User must register all callback functions first!");
	} else {
		std::string mimcToken = this->tokenFetcher->fetchToken();
		Json::Reader *readerinfo = new Json::Reader(Json::Features::strictMode());
		Json::Value root;
		if (readerinfo->parse(mimcToken, root)) {
			int retCode = root["code"].asInt();
			if (retCode == 200) {
				if (this->appAccount == root["data"]["appAccount"].asString()) {
					this->appId = root["data"]["appId"].asString();
					this->appPackage = root["data"]["appPackage"].asString();
					this->chid = root["data"]["miChid"].asInt();
					this->uuid = atol(root["data"]["miUserId"].asString().c_str());
					this->securityKey = root["data"]["miUserSecurityKey"].asString();
					this->token = root["data"]["token"].asString();

					this->permitLogin = true;
				} else {
					LOG4CPLUS_ERROR(LOGGER, "login Failed! Mismatched appAccount! current user is " << this->appAccount << ", token user is " << root["data"]["appAccount"].asString());
				}
			} else {
				LOG4CPLUS_ERROR(LOGGER, "login Failed! RetCode expect 200, but " << retCode);
			}
		} else {
			LOG4CPLUS_ERROR(LOGGER, "login Failed! Wrong token!");
		}
		delete readerinfo;
	}

	return this->permitLogin;
}

bool User::logout() {
	if (this->permitLogin == false) {
		LOG4CPLUS_WARN(LOGGER, "user has not logged in or has already logged out");
		return false;
	}

	this->permitLogin = false;
	struct waitToSendContent logout_obj;
	logout_obj.cmd = BODY_CLIENTHEADER_CMD_UNBIND;
	logout_obj.type = C2S_DOUBLE_DIRECTION;
	logout_obj.message = NULL;
	(this->packetManager->packetsWaitToSend).push(logout_obj);
	return true;
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
