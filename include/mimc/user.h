#ifndef MIMC_CPP_SDK_USER_H
#define MIMC_CPP_SDK_USER_H

#include <mimc/tokenfetcher.h>
#include <mimc/onlinestatus_handler.h>
#include <mimc/message_handler.h>
#include <mimc/rts_callevent_handler.h>
#include <mimc/rts_connection_info.h>
#include <mimc/constant.h>
#include <mimc/mimc.pb.h>
#include <mimc/rts_data.pb.h>
#include <mimc/rts_signal.pb.h>
#include <mimc/rts_send_data.h>
#include <mimc/rts_send_signal.h>
#include <mimc/p2p_chatsession.h>
#include <pthread.h>
#include <time.h>
#include <map>
#include <vector>
#include <XMDTransceiver.h>
#include <LoggerWrapper.h>

struct onLaunchedParam {
	User* user;
	long chatId;
};

class PacketManager;
class RtsConnectionHandler;
class RtsStreamHandler;

class User {
public:
	User(std::string appAccount, std::string resource = "cpp");
	~User();

	void setTestNetworkBlocked(bool testNetWorkBlocked) {this->testNetWorkBlocked = testNetWorkBlocked;this->xmdTranseiver->setTestNetFlag(testNetWorkBlocked);}
	void setPermitLogin(bool permitLogin) {this->permitLogin = permitLogin;}
	void setLastLoginTimestamp(long ts) {this->lastLoginTimestamp = ts;}
	void setLastCreateConnTimestamp(long ts) {this->lastCreateConnTimestamp = ts;}
	void setOnlineStatus(OnlineStatus status) {this->onlineStatus = status;}
	void setRelayLinkState(RelayLinkState state) {this->relayLinkState = state;}
	void setRelayConnId(unsigned long relayConnId) {this->relayConnId = relayConnId;}
	void setRelayControlStreamId(unsigned short relayControlStreamId) {this->relayControlStreamId = relayControlStreamId;}
	void setRelayVideoStreamId(unsigned short relayVideoStreamId) { this->relayVideoStreamId = relayVideoStreamId; }
	void setRelayAudioStreamId(unsigned short relayAudioStreamId) { this->relayAudioStreamId = relayAudioStreamId; }
	void setBindRelayResponse(const mimc::BindRelayResponse& bindRelayResponse) {this->bindRelayResponse = bindRelayResponse;}
	void setLatestLegalRelayLinkStateTs(long ts) {this->latestLegalRelayLinkStateTs = ts;}
	void setMaxCallNum(unsigned int num) {this->maxCallNum = num;}

	void resetRelayLinkState();

	int getChid() const {return this->chid;}
	long getUuid() const {return this->uuid;}
	std::string getResource() const {return this->resource;}
	std::string getToken() const {return this->token;}
	std::string getSecurityKey() const {return this->securityKey;}
	std::string getAppAccount() const {return this->appAccount;}
	long getAppId() const {return this->appId;}
	std::string getAppPackage() const {return this->appPackage;}
	bool getTestNetworkBlocked() const {return this->testNetWorkBlocked;}
	OnlineStatus getOnlineStatus() const {return this->onlineStatus;}
	std::string getClientAttrs() const {return join(clientAttrs);}
	std::string getCloudAttrs() const {return join(cloudAttrs);}
	PacketManager* getPacketManager() const {return this->packetManager;}
	RelayLinkState getRelayLinkState() const {return this->relayLinkState;}
	unsigned long getRelayConnId() const {return this->relayConnId;}
	unsigned short getRelayControlStreamId() const {return this->relayControlStreamId;}
	unsigned short getRelayVideoStreamId() const{ return this->relayVideoStreamId; }
	unsigned short getRelayAudioStreamId() const{ return this->relayAudioStreamId; }
	std::map<long, P2PChatSession>* getCurrentChats() const {return this->currentChats;}
	std::map<long, pthread_t>* getOnlaunchChats() const {return this->onlaunchChats;}
	XMDTransceiver* getXmdTransceiver() const {return this->xmdTranseiver;}
	const mimc::BindRelayResponse& getBindRelayResponse() const {return this->bindRelayResponse;}
	unsigned int getMaxCallNum() const {return this->maxCallNum;}

	unsigned long getP2PIntranetConnId(long chatId) const;
	unsigned long getP2PInternetConnId(long chatId) const;

	std::string sendMessage(const std::string& toAppAccount, const std::string& msg);
	std::string sendMessage(const std::string& toAppAccount, const std::string& msg, const bool isStore);
	long dialCall(const std::string& toAppAccount, const std::string& toResource = "", const std::string& appContent = "");
	bool sendRtsData(long chatId, const std::string& data, RtsDataType dataType, RtsChannelType channelType = RELAY);
	void closeCall(long chatId, std::string byeReason = "");

	bool login();
	bool logout();

	void registerTokenFetcher(MIMCTokenFetcher* tokenFetcher);
	void registerOnlineStatusHandler(OnlineStatusHandler* handler);
	void registerMessageHandler(MessageHandler* handler);
	void registerRTSCallEventHandler(RTSCallEventHandler* handler);

	OnlineStatusHandler* getStatusHandler() const;
	MessageHandler* getMessageHandler() const;
	RTSCallEventHandler* getRTSCallEventHandler() const;

	void checkToRunXmdTranseiver();

	static void *onLaunched(void *arg);
private:
	int chid;
	long uuid;
	std::string resource;
	std::string token;
	std::string securityKey;
	long appId;
	std::string appAccount;
	std::string appPackage;

	long lastLoginTimestamp;
	long lastCreateConnTimestamp;
	long lastPingTimestamp;

	long latestLegalRelayLinkStateTs;

	bool testNetWorkBlocked;
	bool permitLogin;
	OnlineStatus onlineStatus;
	RelayLinkState relayLinkState;
	unsigned long relayConnId;
	unsigned short relayControlStreamId;
	unsigned short relayVideoStreamId;
	unsigned short relayAudioStreamId;
	unsigned int maxCallNum;

	bool isDialCalling;
	bool isFirstDialCall;

	std::map<std::string, std::string> clientAttrs;
	std::map<std::string, std::string> cloudAttrs;

	std::string join(const std::map<std::string, std::string>& kvs) const;

	PacketManager * packetManager;
	static void *sendPacket(void *arg);
	static void *receivePacket(void *arg);
	static void *checkTimeout(void *arg);

	MIMCTokenFetcher* tokenFetcher;
	OnlineStatusHandler* statusHandler;
	MessageHandler* messageHandler;

	RtsConnectionHandler* rtsConnectionHandler;
	RtsStreamHandler* rtsStreamHandler;

	RTSCallEventHandler* rtsCallEventHandler;

	XMDTransceiver* xmdTranseiver;
	std::map<long, P2PChatSession>* currentChats;
	std::map<long, pthread_t>* onlaunchChats;
	mimc::BindRelayResponse bindRelayResponse;

	pthread_t sendThread, receiveThread, checkThread;
	pthread_mutex_t mutex;

	void relayConnScanAndCallBack();
	void rtsScanAndCallBack();
};

#endif //MIMC_CPP_SDK_USER_H
