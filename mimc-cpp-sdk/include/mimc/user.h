#ifndef MIMC_CPP_SDK_USER_H
#define MIMC_CPP_SDK_USER_H

#include <mimc/tokenfetcher.h>
#include <mimc/onlinestatus_handler.h>
#include <mimc/message_handler.h>
#include <mimc/constant.h>
#include <mimc/mimc.pb.h>
#include <pthread.h>
#include <time.h>
#include <map>
#include <vector>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

class PacketManager;

class User {
public:
	User();
	~User();

	void setLastLoginTimestamp(long ts) {this->lastLoginTimestamp = ts;}
	void setLastCreateConnTimestamp(long ts) {this->lastCreateConnTimestamp = ts;}
	void setOnlineStatus(OnlineStatus status) {this->onlineStatus = status;}
	
	int getChid() const{return this->chid;}
	long getUuid() const{return this->uuid;}
	const std::string &getResource() const{return this->resource;}
	const std::string &getToken() const{return this->token;}
	const std::string &getSecurityKey() const{return this->securityKey;}
	const std::string &getAppAccount() const{return this->appAccount;}
	const std::string &getAppPackage() const{return this->appPackage;}
	OnlineStatus getOnlineStatus() const{return this->onlineStatus;}
	std::string getClientAttrs() const{return join(clientAttrs);}
	std::string getCloudAttrs() const{return join(cloudAttrs);}
	PacketManager * getPacketManager() const{return packetManager;}

	std::string sendMessage(const std::string& toAppAccount, const std::string& msg);
	std::string sendMessage(const std::string& toAppAccount, const std::string& msg, const bool isStore);

	bool login();
	bool logout();

	void registerTokenFetcher(MIMCTokenFetcher* tokenFetcher);
	void registerOnlineStatusHandler(OnlineStatusHandler* handler);
	void registerMessageHandler(MessageHandler* handler);
	OnlineStatusHandler* getStatusHandler() const;
	MessageHandler* getMessageHandler() const;
private:
	int chid;
	long uuid;
	std::string resource;
	std::string token;
	std::string securityKey;
	std::string appId;
	std::string appAccount;
	std::string appPackage;

	long lastLoginTimestamp;
	long lastCreateConnTimestamp;
	long lastPingTimestamp;

	bool permitLogin;
	OnlineStatus onlineStatus;

	std::map<std::string, std::string> clientAttrs;
	std::map<std::string, std::string> cloudAttrs;

	std::string join(std::map<std::string, std::string> kvs) const;

	PacketManager * packetManager;
	static void *sendPacket(void *arg);
	static void *receivePacket(void *arg);
	static void *checkTimeout(void *arg);

	MIMCTokenFetcher* tokenFetcher;
	OnlineStatusHandler* statusHandler;
	MessageHandler* messageHandler;
};

#endif //MIMC_CPP_SDK_USER_H
