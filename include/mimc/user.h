#ifndef MIMC_CPP_SDK_USER_H
#define MIMC_CPP_SDK_USER_H

#include <mimc/tokenfetcher.h>
#include <mimc/onlinestatus_handler.h>
#include <mimc/message_handler.h>
#include <mimc/rts_callevent_handler.h>
#include <mimc/constant.h>
#include <mimc/rts_stream_config.h>
#include <XMDCommonData.h>
#include <XMDLoggerWrapper.h>
#include <atomic>
#include <pthread.h>
#include <signal.h>
#include <cerrno>
#include <time.h>
#include <map>
#include <vector>

class Connection;
class PacketManager;
class P2PCallSession;
class RtsConnectionHandler;
class RtsStreamHandler;
class XMDTransceiver;
namespace mimc {
	class BindRelayResponse;
}
struct json_object;

class User {
public:
	User(int64_t appId, std::string appAccount, std::string resource = "", std::string cachePath = "");
	~User();

	void registerTokenFetcher(MIMCTokenFetcher* tokenFetcher) {this->tokenFetcher = tokenFetcher;}
	void registerOnlineStatusHandler(OnlineStatusHandler* handler) {this->statusHandler = handler;}
	void registerMessageHandler(MessageHandler* handler) {this->messageHandler = handler;}
	void registerRTSCallEventHandler(RTSCallEventHandler* handler) {this->rtsCallEventHandler = handler;}

	MIMCTokenFetcher* getTokenFetcher() const {return this->tokenFetcher;}
	OnlineStatusHandler* getStatusHandler() const {return this->statusHandler;}
	MessageHandler* getMessageHandler() const {return this->messageHandler;}
	RTSCallEventHandler* getRTSCallEventHandler() const {return this->rtsCallEventHandler;}

	void setTestPacketLoss(int testPacketLoss);
	void setLastLoginTimestamp(time_t ts) {this->lastLoginTimestamp = ts;}
	void setLastCreateConnTimestamp(time_t ts) {this->lastCreateConnTimestamp = ts;}
	void setOnlineStatus(OnlineStatus status) {this->onlineStatus = status;}
	void setRelayLinkState(RelayLinkState state) {this->relayLinkState = state;}
	void setRelayConnId(uint64_t relayConnId) {this->relayConnId = relayConnId;}
	void setRelayControlStreamId(uint16_t relayControlStreamId) {this->relayControlStreamId = relayControlStreamId;}
	void setRelayVideoStreamId(uint16_t relayVideoStreamId) {this->relayVideoStreamId = relayVideoStreamId;}
	void setRelayAudioStreamId(uint16_t relayAudioStreamId) {this->relayAudioStreamId = relayAudioStreamId;}
	void setRelayFileStreamId(uint16_t relayFileStreamId) {this->relayFileStreamId = relayFileStreamId;}
	void setBindRelayResponse(mimc::BindRelayResponse* bindRelayResponse) {this->bindRelayResponse = bindRelayResponse;}
	void setLatestLegalRelayLinkStateTs(time_t ts) {this->latestLegalRelayLinkStateTs = ts;}
	void setMaxCallNum(unsigned int num) {this->maxCallNum = num;}
	void setTokenInvalid(bool tokenInvalid) {this->tokenInvalid = tokenInvalid;}
	void setAddressInvalid(bool addressInvalid) {this->addressInvalid = addressInvalid;}

	int getChid() const {return this->chid;}
	int64_t getUuid() const {return this->uuid;}
	std::string getResource() const {return this->resource;}
	std::string getToken() const {return this->token;}
	std::string getSecurityKey() const {return this->securityKey;}
	std::string getAppAccount() const {return this->appAccount;}
	int64_t getAppId() const {return this->appId;}
	std::string getAppPackage() const {return this->appPackage;}
	int getRegionBucket() const {return this->regionBucket;}
	std::string getFeDomain() const {return this->feDomain;}
	std::string getRelayDomain() const {return this->relayDomain;}
	std::vector<std::string>& getFeAddresses() {return this->feAddresses;}
	std::vector<std::string>& getRelayAddresses() {return this->relayAddresses;}
	pthread_rwlock_t& getCallsRwlock() { return this->mutex_0; }
	pthread_mutex_t& getAddressMutex() { return this->mutex_1; }
	int getTestPacketLoss() const {return this->testPacketLoss;}
	OnlineStatus getOnlineStatus() const {return this->onlineStatus;}
	std::string getClientAttrs() const {return join(clientAttrs);}
	std::string getCloudAttrs() const {return join(cloudAttrs);}
	PacketManager* getPacketManager() const {return this->packetManager;}
	RelayLinkState getRelayLinkState() const {return this->relayLinkState;}
	uint64_t getRelayConnId() const {return this->relayConnId;}
	uint16_t getRelayControlStreamId() const {return this->relayControlStreamId;}
	uint16_t getRelayVideoStreamId() const {return this->relayVideoStreamId;}
	uint16_t getRelayAudioStreamId() const {return this->relayAudioStreamId;}
	uint16_t getRelayFileStreamId() const {return this->relayFileStreamId;}
	std::map<uint64_t, P2PCallSession>* getCurrentCalls() const {return this->currentCalls;}
	std::map<uint64_t, pthread_t>* getOnlaunchCalls() const {return this->onlaunchCalls;}
	XMDTransceiver* getXmdTransceiver() const {return this->xmdTranseiver;}
	mimc::BindRelayResponse* getBindRelayResponse() const {return this->bindRelayResponse;}
	unsigned int getMaxCallNum() const {return this->maxCallNum;}
	bool getTokenFetchSucceed() const {return this->tokenFetchSucceed;}
	short getTokenRequestStatus() const {return this->tokenRequestStatus;}
	const RtsStreamConfig& getStreamConfig(RtsDataType rtsDataType) const {return rtsDataType == AUDIO ? this->audioStreamConfig : this->videoStreamConfig;}
	uint64_t getP2PIntranetConnId(uint64_t callId);
	uint64_t getP2PInternetConnId(uint64_t callId);

	void resetRelayLinkState();
	void handleXMDConnClosed(uint64_t connId, ConnCloseType type);

	bool login();
	bool logout();

	std::string sendMessage(const std::string& toAppAccount, const std::string& payload, const std::string& bizType = "", const bool isStore = true);
	std::string sendGroupMessage(int64_t topicId, const std::string& payload, const std::string& bizType = "", const bool isStore = true);

	uint64_t dialCall(const std::string& toAppAccount, const std::string& appContent = "", const std::string& toResource = "");
	int sendRtsData(uint64_t callId, const std::string& data, const RtsDataType dataType, const RtsChannelType channelType = RELAY, const std::string& ctx = "", const bool canBeDropped = false, const DataPriority priority = P1, const unsigned int resendCount = 2);
	void closeCall(uint64_t callId, std::string byeReason = "");

	void initAudioStreamConfig(const RtsStreamConfig& audioStreamConfig);
	void initVideoStreamConfig(const RtsStreamConfig& videoStreamConfig);
	void setSendBufferSize(int size);
	void setRecvBufferSize(int size);
	int getSendBufferSize();
	int getRecvBufferSize();
	float getSendBufferUsageRate();
	float getRecvBufferUsageRate();
	void clearSendBuffer();
	void clearRecvBuffer();

	void checkToRunXmdTranseiver();

	static void* onLaunched(void *arg);

	static bool fetchServerAddr(User* user);

private:
	bool permitLogin;
	std::string cachePath;
	std::string cacheFile;
	bool cacheExist;
	bool tokenInvalid;
	bool addressInvalid;
	short tokenRequestStatus; 
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
	int64_t uuid;
	std::string resource;
	std::string token;
	std::string securityKey;
	int64_t appId;
	std::string appAccount;
	std::string appPackage;
	int regionBucket;
	std::string feDomain;
	std::string relayDomain;

	time_t lastLoginTimestamp;
	time_t lastCreateConnTimestamp;
	time_t lastPingTimestamp;
	time_t latestLegalRelayLinkStateTs;

	int testPacketLoss;
	OnlineStatus onlineStatus;
	RelayLinkState relayLinkState;
	uint64_t relayConnId;
	uint16_t relayControlStreamId;
	uint16_t relayVideoStreamId;
	uint16_t relayAudioStreamId;
	uint16_t relayFileStreamId;

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

	std::map<uint64_t, P2PCallSession>* currentCalls;
	std::map<uint64_t, pthread_t>* onlaunchCalls;

	mimc::BindRelayResponse* bindRelayResponse;

	pthread_t sendThread, receiveThread, checkThread;
	pthread_rwlock_t mutex_0;
	pthread_mutex_t mutex_1;
#ifdef __ANDROID__
	static void handle_quit(int signo);
#endif
	static void cleanup(void *arg);

	void relayConnScanAndCallBack();
	void rtsScanAndCallBack();

	void checkAndCloseCalls();
};

struct onLaunchedParam {
	User* user;
	uint64_t callId;
};

#endif //MIMC_CPP_SDK_USER_H
