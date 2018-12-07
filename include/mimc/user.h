#ifndef MIMC_CPP_SDK_USER_H
#define MIMC_CPP_SDK_USER_H

#include <mimc/tokenfetcher.h>
#include <mimc/serverfetcher.h>
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
#include <mimc/rts_stream_config.h>
#include <mimc/p2p_chatsession.h>
#include <json-c/json.h>
#include <pthread.h>
#include <time.h>
#include <map>
#include <vector>
#include <XMDTransceiver.h>
#include <XMDLoggerWrapper.h>

struct onLaunchedParam {
	User* user;
	long chatId;
};

class Connection;
class PacketManager;
class RtsConnectionHandler;
class RtsStreamHandler;

class User {
public:
	User(std::string appAccount, std::string resource = "", std::string cachePath = "");
	~User();

	void setTestPacketLoss(int testPacketLoss) {this->testPacketLoss = testPacketLoss;if(this->xmdTranseiver) {this->xmdTranseiver->setTestPacketLoss(testPacketLoss);}}
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
	void setTokenExpired(bool tokenExpired) {this->tokenExpired = tokenExpired;}
	void setAddressInvalid(bool addressInvalid) {this->addressInvalid = addressInvalid;}

	void initAudioStreamConfig(const RtsStreamConfig& audioStreamConfig) {this->audioStreamConfig = audioStreamConfig;}
	void initVideoStreamConfig(const RtsStreamConfig& videoStreamConfig) {this->videoStreamConfig = videoStreamConfig;}
	void setSendBufferSize(int size) {if (size > 0) {if(this->xmdTranseiver) {this->xmdTranseiver->setSendBufferSize(size);} else {this->xmdSendBufferSize = size;}}}
	void setRecvBufferSize(int size) {if (size > 0) {if(this->xmdTranseiver) {this->xmdTranseiver->setRecvBufferSize(size);} else {this->xmdRecvBufferSize = size;}}}
	int getSendBufferSize() {return this->xmdTranseiver ? this->xmdTranseiver->getSendBufferSize() : 0;}
	int getRecvBufferSize() {return this->xmdTranseiver ? this->xmdTranseiver->getRecvBufferSize() : 0;}
	float getSendBufferUsageRate() {return this->xmdTranseiver ? this->xmdTranseiver->getSendBufferUsageRate() : 0;}
	float getRecvBufferUsageRate() {return this->xmdTranseiver ? this->xmdTranseiver->getRecvBufferUsageRate() : 0;}
	void clearSendBuffer() {if(this->xmdTranseiver) {this->xmdTranseiver->clearSendBuffer();}}
	void clearRecvBuffer() {if(this->xmdTranseiver) {this->xmdTranseiver->clearRecvBuffer();}}

	int getChid() const {return this->chid;}
	long getUuid() const {return this->uuid;}
	std::string getResource() const {return this->resource;}
	std::string getToken() const {return this->token;}
	std::string getSecurityKey() const {return this->securityKey;}
	std::string getAppAccount() const {return this->appAccount;}
	long getAppId() const {return this->appId;}
	std::string getAppPackage() const {return this->appPackage;}
	int getRegionBucket() const {return this->regionBucket;}
	std::string getFeDomain() const {return this->feDomain;}
	std::string getRelayDomain() const {return this->relayDomain;}
	std::vector<std::string>& getFeAddresses() {return this->feAddresses;}
	std::vector<std::string>& getRelayAddresses() {return this->relayAddresses;}
	pthread_rwlock_t& getChatsRwlock() {return this->mutex_0;}
	pthread_mutex_t& getAddressMutex() {return this->mutex_1;}
	int getTestPacketLoss() const {return this->testPacketLoss;}
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
	const RtsStreamConfig& getStreamConfig(RtsDataType rtsDataType) const {return rtsDataType == AUDIO ? this->audioStreamConfig : this->videoStreamConfig;}

	unsigned long getP2PIntranetConnId(long chatId);
	unsigned long getP2PInternetConnId(long chatId);

	std::string sendMessage(const std::string& toAppAccount, const std::string& msg, const std::string& bizType = "", const bool isStore = true);
	long dialCall(const std::string& toAppAccount, const std::string& appContent = "", const std::string& toResource = "");
	bool sendRtsData(long chatId, const std::string& data, const RtsDataType dataType, const RtsChannelType channelType = RELAY, const std::string& ctx = "", const bool canBeDropped = false, const DataPriority priority = P1, const unsigned int resendCount = 2);
	void closeCall(long chatId, std::string byeReason = "");

	void resetRelayLinkState();
	void handleXMDConnClosed(unsigned long connId, ConnCloseType type);

	bool login();
	bool logout();

	void registerTokenFetcher(MIMCTokenFetcher* tokenFetcher);
	void registerOnlineStatusHandler(OnlineStatusHandler* handler);
	void registerMessageHandler(MessageHandler* handler);
	void registerRTSCallEventHandler(RTSCallEventHandler* handler);

	MIMCTokenFetcher* getTokenFetcher() const;
	OnlineStatusHandler* getStatusHandler() const;
	MessageHandler* getMessageHandler() const;
	RTSCallEventHandler* getRTSCallEventHandler() const;

	void checkToRunXmdTranseiver();

	static void* onLaunched(void *arg);

	static bool fetchServerAddr(User* user);
private:
	bool permitLogin;
	std::string cachePath;
	std::string cacheFile;
	bool cacheExist;
	bool tokenExpired;
	bool addressInvalid;
	bool tokenFetchSucceed;
	bool serverFetchSucceed;
	bool parseToken(const char* str, json_object*& pobj);
	bool parseServerAddr(const char* str, json_object*& pobj);
	static void createCacheFileIfNotExist(User* user);
	static bool fetchToken(User* user);
	const std::string& getCachePath() const {return this->cachePath;}
	const std::string& getCacheFile() const {return this->cacheFile;} 

	std::vector<std::string> feAddresses;
	std::vector<std::string> relayAddresses;

	int chid;
	long uuid;
	std::string resource;
	std::string token;
	std::string securityKey;
	long appId;
	std::string appAccount;
	std::string appPackage;
	int regionBucket;
	std::string feDomain;
	std::string relayDomain;

	long lastLoginTimestamp;
	long lastCreateConnTimestamp;
	long lastPingTimestamp;

	long latestLegalRelayLinkStateTs;

	int testPacketLoss;
	OnlineStatus onlineStatus;
	RelayLinkState relayLinkState;
	unsigned long relayConnId;
	unsigned short relayControlStreamId;
	unsigned short relayVideoStreamId;
	unsigned short relayAudioStreamId;
	unsigned int maxCallNum;

	std::map<std::string, std::string> clientAttrs;
	std::map<std::string, std::string> cloudAttrs;

	std::string join(const std::map<std::string, std::string>& kvs) const;

	Connection* conn;
	PacketManager* packetManager;
	static void* sendPacket(void *arg);
	static void* receivePacket(void *arg);
	static void* checkTimeout(void *arg);

	MIMCTokenFetcher* tokenFetcher;
	OnlineStatusHandler* statusHandler;
	MessageHandler* messageHandler;
	RTSCallEventHandler* rtsCallEventHandler;

	RtsConnectionHandler* rtsConnectionHandler;
	RtsStreamHandler* rtsStreamHandler;

	XMDTransceiver* xmdTranseiver;
	RtsStreamConfig audioStreamConfig;
	RtsStreamConfig videoStreamConfig;
	int xmdSendBufferSize;
	int xmdRecvBufferSize;
	std::map<long, P2PChatSession>* currentChats;
	std::map<long, pthread_t>* onlaunchChats;
	mimc::BindRelayResponse bindRelayResponse;

	pthread_t sendThread, receiveThread, checkThread;
	pthread_rwlock_t mutex_0;
	pthread_mutex_t mutex_1;
	static void cleanup(void *arg);

	void relayConnScanAndCallBack();
	void rtsScanAndCallBack();

	void checkAndCloseChats();
};

#endif //MIMC_CPP_SDK_USER_H
