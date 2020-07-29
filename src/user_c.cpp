#include "mimc/user_c.h"
#include "mimc/user.h"
#include "XMDTransceiver.h"
#include "curl/curl.h"

class CTokenFetcher : public MIMCTokenFetcher {
public:
	CTokenFetcher(std::string app_account) : _app_account(app_account) {
		#ifndef STAGING
        _app_id = "2882303761517669588";
        _app_key = "5111766983588";
        _app_secret = "b0L3IOz/9Ob809v8H2FbVg==";
        #else
        _app_id = "2882303761517479657";
        _app_key = "5221747911657";
        _app_secret = "PtfBeZyC+H8SIM/UXhZx1w==";
        #endif

	}

	std::string fetchToken()
	{
		CURL *curl = curl_easy_init();
		CURLcode res;
		#ifndef STAGING
		const std::string url = "https://mimc.chat.xiaomi.net/api/account/token";
		#else
		const std::string url = "http://10.38.162.149/api/account/token";
		#endif
		std::string result;
		const std::string body = "{\"appId\":\"" + _app_id + "\",\"appKey\":\"" + _app_key + "\",\"appSecret\":\"" + _app_secret + "\",\"appAccount\":\"" + _app_account + "\"}";
		if (curl)
		{
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

			struct curl_slist *headers = NULL;
			headers = curl_slist_append(headers, "Content-Type: application/json");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&result));

			char errbuf[CURL_ERROR_SIZE];
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
			errbuf[0] = 0;

			res = curl_easy_perform(curl);
			if (res != CURLE_OK)
			{
				XMDLoggerWrapper::instance()->error("curl perform error, error code is %d, error msg is %s", res, errbuf);
			}
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
		}
		
        return result;
    }

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        std::string *bodyp = (std::string *)userp;
        bodyp->append((const char *)contents, size * nmemb);

        return size * nmemb;
    }
private:
    std::string _app_id;
    std::string _app_key;
    std::string _app_secret;
    std::string _app_account;
};


class TokenFetcher : public MIMCTokenFetcher {
private:
	void *_args;
	fetch_token_t _fetch_token;
	free_args_t _free_args;
	char *_token;
	#define MAX_LEN_TOKEN 2 * 1024

public:
	TokenFetcher(void *args, fetch_token_t fetch_token, free_args_t free_args) {
		_args = args;
		_fetch_token = fetch_token;
		_free_args = free_args;
		_token = new char[MAX_LEN_TOKEN];
	}

	~TokenFetcher(){
		delete[] _token;
		_free_args(_args);
	}

	std::string fetchToken()
	{
		memset(_token, 0, MAX_LEN_TOKEN);
		_fetch_token(_args, _token, MAX_LEN_TOKEN);

		return std::string(_token);
	}
};

class COnlineStatusHandler : public OnlineStatusHandler {
public:
	COnlineStatusHandler(const online_status_handler_t& online_status_handler) : _online_status_handler(online_status_handler) {

	}

	void statusChange(OnlineStatus status, const std::string& type, const std::string& reason, const std::string& desc) {
		online_status_t online_status = OFFLINE;
		if (status == Online) {
			online_status = ONLINE;
		}
		_online_status_handler.status_change(online_status, type.c_str(), reason.c_str(), desc.c_str());
	}

private:
	online_status_handler_t _online_status_handler;
};

class CRTSCallEventHandler : public RTSCallEventHandler {
public:
	CRTSCallEventHandler(const rtscall_event_handler_t& rtscall_event_handler) : _rtscall_event_handler(rtscall_event_handler) {

	}

	LaunchedResponse onLaunched(uint64_t callId, const std::string& fromAccount, const std::string& appContent, const std::string& fromResource) {
		const launched_response_t& launched_response = _rtscall_event_handler.on_launched(callId, fromAccount.c_str(), appContent.c_str(), appContent.length(), fromResource.c_str());
		return LaunchedResponse(launched_response.accepted, launched_response.desc);
	}

	void onAnswered(uint64_t callId, bool accepted, const std::string& desc) {
		_rtscall_event_handler.on_answered(callId, accepted, desc.c_str());
	}

	void onClosed(uint64_t callId, const std::string& desc) {
		_rtscall_event_handler.on_closed(callId, desc.c_str());
	}

	void onData(uint64_t callId, const std::string& fromAccount, const std::string& resource, const std::string& data, RtsDataType dataType, RtsChannelType channelType) {
		data_type_t data_type;
		switch(dataType) {
			case AUDIO:
				data_type = AUD;
				break;
			case VIDEO:
				data_type = VID;
				break;
			case FILEDATA:
				data_type = FILED;
			default:
				break;
		}

		channel_type_t channel_type;
		switch(channelType) {
			case RELAY:
				channel_type = RELAY_T;
				break;
			case P2P_INTRANET:
				channel_type = P2P_INTRANET_T;
				break;
			case P2P_INTERNET:
				channel_type = P2P_INTERNET_T;
				break;
			default:
				break;
		}

		_rtscall_event_handler.on_data(callId, fromAccount.c_str(), resource.c_str(), data.c_str(), data.length(), data_type, channel_type);
	}

	void onSendDataSuccess(uint64_t callId, int dataId, const std::string& ctx) {
		_rtscall_event_handler.on_send_data_success(callId, dataId, ctx.c_str(), ctx.length());
	}

	void onSendDataFailure(uint64_t callId, int dataId, const std::string& ctx) {
		_rtscall_event_handler.on_send_data_failure(callId, dataId, ctx.c_str(), ctx.length());
	}

	//added by huanghua for p2p
	void onP2PResult(uint64_t callId, int result, int selfNatType, int peerNatType) {
		_rtscall_event_handler.on_p2p_result(callId, result, selfNatType, peerNatType);
	}

private:
	rtscall_event_handler_t _rtscall_event_handler;
};

class CMessageHandler : public MessageHandler {
public:
	CMessageHandler(const message_handler_t& message_handler) : _message_handler(message_handler) {
	}

	bool handleMessage(const std::vector<MIMCMessage>& packets) {
		uint64_t pkts_len = packets.size();
		mimc_message_t* mimc_messages = new mimc_message_t[pkts_len];

		int temp_len;
		for(int index = 0; index < pkts_len; index++) {
			temp_len = packets[index].getPacketId().length() + 1;
			mimc_messages[index].packetid = new char[temp_len];
			memset((void *)mimc_messages[index].packetid, 0, temp_len);
			memcpy((char *)mimc_messages[index].packetid, packets[index].getPacketId().c_str(), temp_len);
			mimc_messages[index].sequence = packets[index].getSequence();
			temp_len = packets[index].getFromAccount().length() + 1;
			mimc_messages[index].from_account = new char[temp_len];
			memset((void *)mimc_messages[index].from_account, 0, temp_len);
			memcpy((char *)mimc_messages[index].from_account, packets[index].getFromAccount().c_str(), temp_len);
			temp_len = packets[index].getFromResource().length() + 1;
			mimc_messages[index].from_resource = new char[temp_len];
			memset((void *)mimc_messages[index].from_resource, 0, temp_len);
			memcpy((char *)mimc_messages[index].from_resource, packets[index].getFromResource().c_str(), temp_len);
			temp_len = packets[index].getToAccount().length() + 1;
			mimc_messages[index].to_account = new char[temp_len];
			memset((void *)mimc_messages[index].to_account, 0, temp_len);
			memcpy((char *)mimc_messages[index].to_account, packets[index].getToAccount().c_str(), temp_len);
			temp_len = packets[index].getToResource().length() + 1;
			mimc_messages[index].to_resource = new char[temp_len];
			memset((void *)mimc_messages[index].to_resource, 0, temp_len);
			memcpy((char *)mimc_messages[index].to_resource, packets[index].getToResource().c_str(), temp_len);
			temp_len = packets[index].getPayload().length() + 1;
			mimc_messages[index].payload = new char[temp_len];
			memset((void *)mimc_messages[index].payload, 0, temp_len);
			memcpy((char *)mimc_messages[index].payload, packets[index].getPayload().c_str(), temp_len);
			mimc_messages[index].payload_len = packets[index].getPayload().length();
			temp_len = packets[index].getBizType().length() + 1;
			mimc_messages[index].biz_type = new char[temp_len];
			memset((void *)mimc_messages[index].biz_type, 0, temp_len);
			memcpy((char *)mimc_messages[index].biz_type, packets[index].getBizType().c_str(), temp_len);
			mimc_messages[index].timestamp = packets[index].getTimeStamp();
		}

		_message_handler.handle_message(mimc_messages, pkts_len);
		
		for (int index = 0; index < pkts_len; index++)
		{
			delete[] mimc_messages[index].packetid;
			mimc_messages[index].packetid = NULL;
			delete[] mimc_messages[index].from_account;
			mimc_messages[index].from_account = NULL;
			delete[] mimc_messages[index].from_resource;
			mimc_messages[index].from_resource = NULL;
			delete[] mimc_messages[index].to_account;
			mimc_messages[index].to_account = NULL;
			delete[] mimc_messages[index].to_resource;
			mimc_messages[index].to_resource = NULL;
			delete[] mimc_messages[index].payload;
			mimc_messages[index].payload = NULL;
			delete[] mimc_messages[index].biz_type;
			mimc_messages[index].biz_type = NULL;
		}
		delete[] mimc_messages;
		mimc_messages = NULL;

		return true;
	}

	void handleOnlineMessage(const MIMCMessage& packets) {
		mimc_message_t* mimc_messages = new mimc_message_t();
		XMDLoggerWrapper::instance()->info("receive a online message,from_account is %s,from_resource is %s,to_account is %s,to_resource is %s",
		packets.getFromAccount().c_str(),packets.getFromResource().c_str(), packets.getToAccount().c_str(),packets.getToResource().c_str());
		
		int temp_len;
		temp_len = packets.getPacketId().length() + 1;
		mimc_messages->packetid = new char[temp_len];
		memset((void *)mimc_messages->packetid, 0, temp_len);
		memcpy((char *)mimc_messages->packetid, packets.getPacketId().c_str(), temp_len);
		mimc_messages->sequence = packets.getSequence();
		temp_len = packets.getFromAccount().length() + 1;
		mimc_messages->from_account = new char[temp_len];
		memset((void *)mimc_messages->from_account, 0, temp_len);
		memcpy((char *)mimc_messages->from_account, packets.getFromAccount().c_str(), temp_len);
		temp_len = packets.getFromResource().length() + 1;
		mimc_messages->from_resource = new char[temp_len];
		memset((void *)mimc_messages->from_resource, 0, temp_len);
		memcpy((char *)mimc_messages->from_resource, packets.getFromResource().c_str(), temp_len);
		temp_len = packets.getToAccount().length() + 1;
		mimc_messages->to_account = new char[temp_len];
		memset((void *)mimc_messages->to_account, 0, temp_len);
		memcpy((char *)mimc_messages->to_account, packets.getToAccount().c_str(), temp_len);
		temp_len = packets.getToResource().length() + 1;
		mimc_messages->to_resource = new char[temp_len];
		memset((void *)mimc_messages->to_resource, 0, temp_len);
		memcpy((char *)mimc_messages->to_resource, packets.getToResource().c_str(), temp_len);
		temp_len = packets.getPayload().length() + 1;
		mimc_messages->payload = new char[temp_len];
		memset((void *)mimc_messages->payload, 0, temp_len);
		memcpy((char *)mimc_messages->payload, packets.getPayload().c_str(), temp_len);
		mimc_messages->payload_len = packets.getPayload().length();
		temp_len = packets.getBizType().length() + 1;
		mimc_messages->biz_type = new char[temp_len];
		memset((void *)mimc_messages->biz_type, 0, temp_len);
		memcpy((char *)mimc_messages->biz_type, packets.getBizType().c_str(), temp_len);
		mimc_messages->timestamp = packets.getTimeStamp();

		_message_handler.handle_online_message(mimc_messages);

		delete[] mimc_messages->packetid;
		mimc_messages->packetid = NULL;
		delete[] mimc_messages->from_account;
		mimc_messages->from_account = NULL;
		delete[] mimc_messages->from_resource;
		mimc_messages->from_resource = NULL;
		delete[] mimc_messages->to_account;
		mimc_messages->to_account = NULL;
		delete[] mimc_messages->to_resource;
		mimc_messages->to_resource = NULL;
		delete[] mimc_messages->payload;
		mimc_messages->payload = NULL;
		delete[] mimc_messages->biz_type;
		mimc_messages->biz_type = NULL;

		delete mimc_messages;
		mimc_messages = NULL;
	}

	bool handleGroupMessage(const std::vector<MIMCGroupMessage>& packets) {
		uint64_t pkts_len = packets.size();
		mimc_group_message_t* mimc_group_messages = new mimc_group_message_t[pkts_len];

		for(int index = 0; index < pkts_len; index++) {
			mimc_group_messages[index].packetid = packets[index].getPacketId().c_str();
			mimc_group_messages[index].sequence = packets[index].getSequence();
			mimc_group_messages[index].timestamp = packets[index].getTimeStamp();
			mimc_group_messages[index].from_account = packets[index].getFromAccount().c_str();
			mimc_group_messages[index].from_resource = packets[index].getFromResource().c_str();
			mimc_group_messages[index].topicid = packets[index].getTopicId();
			mimc_group_messages[index].payload = packets[index].getPayload().c_str();
			mimc_group_messages[index].payload_len = packets[index].getPayload().length();
			mimc_group_messages[index].biz_type = packets[index].getBizType().c_str();
		}
		_message_handler.handle_group_message(mimc_group_messages, pkts_len);
		delete[] mimc_group_messages;

		return true;
	}

	void handleServerAck(const MIMCServerAck &serverAck) {
		_message_handler.handle_server_ack(serverAck.getPacketId().c_str(), serverAck.getSequence(), serverAck.getTimestamp(), serverAck.getDesc().c_str());
	}

	void handleSendMsgTimeout(const MIMCMessage& message) {
		mimc_message_t mimc_message;
		mimc_message.packetid = message.getPacketId().c_str();
		mimc_message.sequence = message.getSequence();
		mimc_message.from_account = message.getFromAccount().c_str();
		mimc_message.from_resource = message.getFromResource().c_str();
		mimc_message.to_account = message.getToAccount().c_str();
		mimc_message.to_resource = message.getToResource().c_str();
		mimc_message.payload = message.getPayload().c_str();
		mimc_message.payload_len = message.getPayload().length();
		mimc_message.biz_type = message.getBizType().c_str();
		mimc_message.timestamp = message.getTimeStamp();

		_message_handler.handle_send_msg_timeout(&mimc_message);
	}

	void handleSendGroupMsgTimeout(const MIMCGroupMessage& groupMessage) {
		mimc_group_message_t mimc_group_message;
		mimc_group_message.packetid = groupMessage.getPacketId().c_str();
		mimc_group_message.sequence = groupMessage.getSequence();
		mimc_group_message.timestamp = groupMessage.getTimeStamp();
		mimc_group_message.from_account = groupMessage.getFromAccount().c_str();
		mimc_group_message.from_resource = groupMessage.getFromResource().c_str();
		mimc_group_message.topicid = groupMessage.getTopicId();
		mimc_group_message.payload = groupMessage.getPayload().c_str();
		mimc_group_message.payload_len = groupMessage.getPayload().length();
		mimc_group_message.biz_type = groupMessage.getBizType().c_str();

		_message_handler.handle_send_group_msg_timeout(&mimc_group_message);
	}

	bool onPullNotification()
	{
		return true;
	}

private:
	message_handler_t _message_handler;
};

class LogHandler : public ExternalLog
{
private:
	log_handler_t log_handler;

public:
	LogHandler(log_handler_t &log_handler)
	{
		this->log_handler = log_handler;
	}

	void debug(const char *msg)
	{
		if (log_handler.debug)
		{
			log_handler.debug(msg);
		}
	}

	void info(const char *msg)
	{
		if (log_handler.info)
		{
			log_handler.info(msg);
		}
	}

	void warn(const char *msg)
	{
		if (log_handler.warn)
		{
			log_handler.warn(msg);
		}
	}

	void error(const char *msg)
	{
		if (log_handler.error)
		{
			log_handler.error(msg);
		}
	}
};

void mimc_rtc_init(user_t *user, int64_t appid, const char *appaccount, const char *resource, bool is_save_cache, const char *cachepath) {
	curl_global_init(CURL_GLOBAL_ALL);
	std::string appaccountStr(appaccount);
	std::string resourceStr;
	if (resource == NULL || resource[0] == 0) {
		resourceStr = "";
	} else {
		resourceStr = resource;
	}
	std::string cachepathStr;
	if (cachepath == NULL || cachepath[0] == 0) {
		cachepathStr = "";
	} else {
		cachepathStr = cachepath;
	}
	user->value = new User(appid, appaccountStr, resourceStr, cachepathStr,is_save_cache);
	user->token_fetcher = NULL;
	user->online_status_handler = NULL;
	user->rtscall_event_handler = NULL;
}

void mimc_rtc_fini(user_t* user) {
	User* userObj = (User*)(user->value);
	delete userObj;
	user->value = NULL;
	MIMCTokenFetcher* tokenFetcher = (MIMCTokenFetcher*)(user->token_fetcher);
	delete tokenFetcher;
	user->token_fetcher = NULL;
	OnlineStatusHandler* onlineStatusHandler = (OnlineStatusHandler*)(user->online_status_handler);
	delete onlineStatusHandler;
	user->online_status_handler = NULL;
	RTSCallEventHandler* rtsCallEventHandler = (RTSCallEventHandler*)(user->rtscall_event_handler);
	delete rtsCallEventHandler;
	user->rtscall_event_handler = NULL;
	curl_global_cleanup();
}

bool mimc_rtc_get_token_fetch_succeed(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->getTokenFetchSucceed();
}

short mimc_rtc_get_token_request_status(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->getTokenRequestStatus();
}

bool mimc_rtc_login(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->login();
}

bool mimc_rtc_logout(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->logout();
}

bool mimc_rtc_isonline(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->getOnlineStatus() == Online ? true : false;
}

void mimc_init(user_t *user, int64_t appid, const char *appaccount, const char *resource, bool is_save_cache,const char *cachepath)
{
	curl_global_init(CURL_GLOBAL_ALL);
	std::string appaccountStr(appaccount);
	std::string resourceStr;
	if (resource == NULL || resource[0] == 0)
	{
		resourceStr = "";
	}
	else
	{
		resourceStr = resource;
	}
	std::string cachepathStr;
	if (cachepath == NULL || cachepath[0] == 0)
	{
		cachepathStr = "";
	}
	else
	{
		cachepathStr = cachepath;
	}
	
	user->value = new User(appid, appaccountStr, resourceStr, cachepathStr,is_save_cache);
	user->token_fetcher = NULL;
	user->online_status_handler = NULL;
	user->rtscall_event_handler = NULL;
}

void mimc_fini(user_t *user)
{
	User *userObj = (User *)(user->value);
	delete userObj;
	user->value = NULL;
	MIMCTokenFetcher *tokenFetcher = (MIMCTokenFetcher *)(user->token_fetcher);
	delete tokenFetcher;
	user->token_fetcher = NULL;
	OnlineStatusHandler *onlineStatusHandler = (OnlineStatusHandler *)(user->online_status_handler);
	delete onlineStatusHandler;
	user->online_status_handler = NULL;
	RTSCallEventHandler *rtsCallEventHandler = (RTSCallEventHandler *)(user->rtscall_event_handler);
	delete rtsCallEventHandler;
	user->rtscall_event_handler = NULL;
	curl_global_cleanup();
}

bool mimc_get_token_fetch_succeed(user_t *user)
{
	User *userObj = (User *)(user->value);
	return userObj->getTokenFetchSucceed();
}

short mimc_get_token_request_status(user_t *user)
{
	User *userObj = (User *)(user->value);
	return userObj->getTokenRequestStatus();
}

bool mimc_login(user_t *user)
{
	User *userObj = (User *)(user->value);
	return userObj->login();
}

bool mimc_logout(user_t *user)
{
	User *userObj = (User *)(user->value);
	return userObj->logout();
}

bool mimc_isonline(user_t *user)
{
	User *userObj = (User *)(user->value);
	return userObj->getOnlineStatus() == Online ? true : false;
}

int mimc_get_login_timeout()
{
	return LOGIN_TIMEOUT;
}

bool mimc_rtc_channel_connected(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->getRelayLinkState() == SUCC_CREATED ? true : false;
}

int mimc_rtc_get_login_timeout() {
	return LOGIN_TIMEOUT;
}

void mimc_rtc_set_max_callnum(user_t* user, unsigned int num) {
	User* userObj = (User*)(user->value);
	userObj->setMaxCallNum(num);
}

unsigned int mimc_rtc_get_max_callnum(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->getMaxCallNum();
}

void mimc_rtc_init_audiostream_config(user_t* user, const stream_config_t* stream_config) {
	User* userObj = (User*)(user->value);
	RtsStreamType streamType = ACK_TYPE;
	if(stream_config->type == FEC_T) {
		streamType = FEC_TYPE;
	}
	RtsStreamConfig audioStreamConfig(streamType, stream_config->ackstream_waittime_ms, stream_config->is_encrypt);
	userObj->initAudioStreamConfig(audioStreamConfig);
}

void mimc_rtc_init_videostream_config(user_t* user, const stream_config_t* stream_config) {
	User* userObj = (User*)(user->value);
	RtsStreamType streamType = FEC_TYPE;
	if(stream_config->type == ACK_T) {
		streamType = ACK_TYPE;
	}
	RtsStreamConfig videoStreamConfig(streamType, stream_config->ackstream_waittime_ms, stream_config->is_encrypt);
	userObj->initVideoStreamConfig(videoStreamConfig);
}

void mimc_rtc_set_sendbuffer_max_size(user_t* user, int size) {
	User* userObj = (User*)(user->value);
	userObj->setSendBufferMaxSize(size);
}

void mimc_rtc_set_sendbuffer_size(user_t* user, int size) {
    mimc_rtc_set_sendbuffer_max_size(user, size);
}

void mimc_rtc_set_recvbuffer_max_size(user_t* user, int size) {
	User* userObj = (User*)(user->value);
	userObj->setRecvBufferMaxSize(size);
}

void mimc_rtc_set_recvbuffer_size(user_t* user, int size) {
    mimc_rtc_set_recvbuffer_max_size(user, size);
}

int mimc_rtc_get_sendbuffer_used_size(user_t* user) {
    User* userObj = (User*)(user->value);
	return userObj->getSendBufferUsedSize();
}

int mimc_rtc_get_sendbuffer_size(user_t* user) {
    return mimc_rtc_get_sendbuffer_used_size(user);
}


int mimc_rtc_get_sendbuffer_max_size(user_t* user) {
    User* userObj = (User*)(user->value);
	return userObj->getSendBufferMaxSize();
}

int mimc_rtc_get_recvbuffer_used_size(user_t* user) {
    User* userObj = (User*)(user->value);
	return userObj->getRecvBufferUsedSize();
}

int mimc_rtc_get_recvbuffer_max_size(user_t* user) {
    User* userObj = (User*)(user->value);
	return userObj->getRecvBufferMaxSize();
}

int mimc_rtc_get_recvbuffer_size(user_t* user) {
    return mimc_rtc_get_recvbuffer_used_size(user);
}



float mimc_rtc_get_sendbuffer_usagerate(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->getSendBufferUsageRate();
}

float mimc_rtc_get_recvbuffer_usagerate(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->getRecvBufferUsageRate();
}

void mimc_rtc_clear_sendbuffer(user_t* user) {
	User* userObj = (User*)(user->value);
	userObj->clearSendBuffer();
}

void mimc_rtc_clear_recvbuffer(user_t* user) {
	User* userObj = (User*)(user->value);
	userObj->clearRecvBuffer();
}

uint64_t mimc_rtc_dial_call(user_t* user, const char* to_appaccount, const char* appcontent, const int appcontent_len, const char* to_resource) {
	User* userObj = (User*)(user->value);
	std::string appContent;
	if (appcontent != NULL && appcontent_len > 0) {
		appContent.assign(appcontent, appcontent_len);
	}
	if (to_resource == NULL) {
		to_resource = "";
	}
	return userObj->dialCall(to_appaccount, appContent, to_resource);
}

void mimc_rtc_close_call(user_t* user, uint64_t callid, const char* bye_reason) {
	User* userObj = (User*)(user->value);
	userObj->closeCall(callid, bye_reason);
}

int mimc_rtc_send_data(user_t* user, uint64_t callid, const char* data, const int data_len, const data_type_t data_type, const channel_type_t channel_type, const char* ctx, const int ctx_len, const bool can_be_dropped, const data_priority_t data_priority, const unsigned int resend_count) {
    User* userObj = (User*)(user->value);
	RtsDataType dataType;
	switch(data_type) {
		case AUD:
			dataType = AUDIO;
			break;
		case VID:
			dataType = VIDEO;
			break;
		case FILED:
			dataType = FILEDATA;
			break;
		default:
			break;
	}

	RtsChannelType channelType = RELAY;
	switch(channel_type) {
		case RELAY_T:
			channelType = RELAY;
			break;
		case P2P_INTRANET_T:
			channelType = P2P_INTRANET;
			break;
		case P2P_INTERNET_T:
			channelType = P2P_INTERNET;
			break;
			//added by huanghua on 2019-04-13
		case CHANNEL_AUTO_T:
			channelType = CHANNEL_AUTO;
		default:
			break;
	}

	DataPriority dataPriority = P1;
	switch(data_priority) {
		case P0_T:
			dataPriority = P0;
			break;
		case P1_T:
			dataPriority = P1;
			break;
		case P2_T:
			dataPriority = P2;
			break;
		default:
			break;
	}

	//modified by huanghua on 201-9-08-20
    std::string rtsCtx(ctx, ctx_len);
#if 0
	std::string rtsData(data, data_len);
	return userObj->sendRtsData(callid, rtsData, dataType, channelType, rtsCtx, can_be_dropped, dataPriority, resend_count);
#endif
	return userObj->sendRtsData(callid, data, data_len, dataType, channelType, rtsCtx, can_be_dropped, dataPriority, resend_count);
}

void mimc_rtc_register_token_fetcher(user_t* user, const char* app_account) {
	User* userObj = (User*)(user->value);
	MIMCTokenFetcher* tokenFetcher = new CTokenFetcher(app_account);
	userObj->registerTokenFetcher(tokenFetcher);
	user->token_fetcher = tokenFetcher;
}

void mimc_register_token_fetcher(user_t *user, void *args, fetch_token_t fetch_token, free_args_t free_args)
{
	User *userObj = (User *)(user->value);
	MIMCTokenFetcher *tokenFetcher = new TokenFetcher(args, fetch_token, free_args);
	userObj->registerTokenFetcher(tokenFetcher);
	user->token_fetcher = tokenFetcher;
}

void mimc_rtc_register_online_status_handler(user_t* user, const online_status_handler_t* online_status_handler) {
	User* userObj = (User*)(user->value);
	OnlineStatusHandler* onlineStatusHandler = new COnlineStatusHandler(*online_status_handler);
	userObj->registerOnlineStatusHandler(onlineStatusHandler);
	user->online_status_handler = onlineStatusHandler;
}

void mimc_register_online_status_handler(user_t *user, const online_status_handler_t *online_status_handler)
{
	User *userObj = (User *)(user->value);
	OnlineStatusHandler *onlineStatusHandler = new COnlineStatusHandler(*online_status_handler);
	userObj->registerOnlineStatusHandler(onlineStatusHandler);
	user->online_status_handler = onlineStatusHandler;
}

void mimc_rtc_register_rtscall_event_handler(user_t* user, const rtscall_event_handler_t* rtscall_event_handler) {
	User* userObj = (User*)(user->value);
	RTSCallEventHandler* rtsCallEventHandler = new CRTSCallEventHandler(*rtscall_event_handler);
	userObj->registerRTSCallEventHandler(rtsCallEventHandler);
	user->rtscall_event_handler = rtsCallEventHandler;
}

void mimc_send_message(user_t *user, const char *to_appaccount, const char *payload, const int payload_len, const char *biz_type, const bool is_store, char *result, int result_len)
{
	User* userObj = (User*)(user->value);
	std::string payloadStr;
	if (payload == NULL) {
		payloadStr = "";
	} else {
		payloadStr.assign(payload, payload_len);
	}

	if (biz_type == NULL) {
		biz_type = "";
	}

	std::string packetId = userObj->sendMessage(to_appaccount, payloadStr, biz_type, is_store);
	if (result && result_len > 0) {
		if (packetId.length() < result_len) {
			memcpy(result, packetId.c_str(), packetId.length() + 1);
		} else {
			memcpy(result, packetId.c_str(), result_len);
			result[result_len - 1] = '\0';
		}
	}
}

void mimc_send_online_message(user_t* user,const char* to_appaccount,const char *payload, const int payload_len, const char *biz_type, const bool is_store, char *result, int result_len) {
	User* userObj = (User*)(user->value);
	std::string payloadStr;
	if (payload == NULL) {
		payloadStr = "";
	} else {
		payloadStr.assign(payload, payload_len);
	}

	if (biz_type == NULL) {
		biz_type = "";
	}

	std::string packetId = userObj->sendOnlineMessage(to_appaccount,payloadStr,biz_type,true);
	if (result && result_len > 0) {
		if (packetId.length() < result_len) {
			memcpy(result, packetId.c_str(), packetId.length() + 1);
		} else {
			memcpy(result, packetId.c_str(), result_len);
			result[result_len - 1] = '\0';
		}
	}
}


void mimc_send_group_message(user_t *user, int64_t topicid, const char *payload, const int payload_len, const char *biz_type, const bool is_store, char *result, int result_len)
{
	User* userObj = (User*)(user->value);
	std::string payloadStr;
	if (payload == NULL) {
		payloadStr = "";
	} else {
		payloadStr.assign(payload, payload_len);
	}

	if (biz_type == NULL) {
		biz_type = "";
	}

	std::string packetId = userObj->sendGroupMessage(topicid, payloadStr, biz_type, is_store).c_str();
	if (result && result_len > 0)
	{
		if (packetId.length() < result_len)
		{
			memcpy(result, packetId.c_str(), packetId.length() + 1);
		}
		else
		{
			memcpy(result, packetId.c_str(), result_len);
			result[result_len - 1] = '\0';
		}
	}
}

void mimc_register_message_handler(user_t* user, const message_handler_t* message_handler) {
	User* userObj = (User*)(user->value);
	MessageHandler* messageHandler = new CMessageHandler(*message_handler);
	userObj->registerMessageHandler(messageHandler);
	user->message_handler = messageHandler;
}
const char* mimc_im_send_message(user_t* user, const char* to_appaccount, const char* payload, const int payload_len, const char* biz_type, const bool is_store) {
	User* userObj = (User*)(user->value);
	std::string payloadStr;
	if (payload == NULL) {
		payloadStr = "";
	} else {
		payloadStr.assign(payload, payload_len);
	}

	if (biz_type == NULL) {
		biz_type = "";
	}

	return userObj->sendMessage(to_appaccount, payloadStr, biz_type, is_store).c_str();
}

const char* mimc_im_send_group_message(user_t* user, int64_t topicid, const char* payload, const int payload_len, const char* biz_type, const bool is_store) {
	User* userObj = (User*)(user->value);
	std::string payloadStr;
	if (payload == NULL) {
		payloadStr = "";
	} else {
		payloadStr.assign(payload, payload_len);
	}

	if (biz_type == NULL) {
		biz_type = "";
	}
	return userObj->sendGroupMessage(topicid, payloadStr, biz_type, is_store).c_str();
}

void mimc_im_register_message_handler(user_t* user, const message_handler_t* message_handler) {
	User* userObj = (User*)(user->value);
	MessageHandler* messageHandler = new CMessageHandler(*message_handler);
	userObj->registerMessageHandler(messageHandler);
	user->message_handler = messageHandler;
}

void mimc_enable_p2p(user_t *user, const bool enable) {
    User* userObj = (User*)(user->value);
    userObj->enableP2P(enable);
}

void mimc_set_app_package(user_t *user, const char* app_package) {
    User* userObj = (User*)(user->value);
    userObj->setAppPackage(app_package);
}

void mimc_set_test_packet_loss(user_t *user, int value) {
    User* userObj = (User*)(user->value);
    userObj->setTestPacketLoss(value);
}

void mimc_set_log_level(log_level_t log_level)
{
	XMDLogLevel xmdLevel;
	switch (log_level)
	{
	case ERROR_T:
		xmdLevel = XMD_ERROR;
		break;
	case WARN_T:
		xmdLevel = XMD_WARN;
		break;
	case INFO_T:
		xmdLevel = XMD_INFO;
		break;
	case DEBUG_T:
		xmdLevel = XMD_DEBUG;
		break;
	}
	XMDLoggerWrapper::instance()->setXMDLogLevel(xmdLevel);
}

void mimc_register_log_handler(log_handler_t *log_handler)
{
	if (log_handler)
	{
		static LogHandler logHandler = LogHandler(*log_handler);
		XMDLoggerWrapper::instance()->externalLog(&logHandler);
	}
}

TimeUnit time_unit_c_convert_cpp(time_unit_t time_unit)
{
	TimeUnit timeUnit;
	switch (time_unit)
	{
	case MILLISECONDS_T:
		timeUnit = MILLISECONDS;
		break;
	case SECONDS_T:
		timeUnit = SECONDS;
		break;
	case MINUTES_T:
		timeUnit = MINUTES;
		break;
	default:
		timeUnit = MINUTES;
		break;
	}

	return timeUnit;
}

void mimc_set_base_of_backoff_When_fetch_token(user_t *user, time_unit_t time_unit, long base)
{
	User *userObj = (User *)(user->value);
	//userObj->setBaseOfBackoffWhenFetchToken(time_unit_c_convert_cpp(time_unit), base);
	mimcBackoff::getInstance()->getbackoffWhenFetchToken().setBase(time_unit_c_convert_cpp(time_unit), base);
}

void mimc_set_cap_of_backoff_When_fetch_token(user_t *user, time_unit_t time_unit, long cap)
{
	User *userObj = (User *)(user->value);
	//userObj->setCapOfBackoffWhenFetchToken(time_unit_c_convert_cpp(time_unit), cap);
	mimcBackoff::getInstance()->getbackoffWhenFetchToken().setCap(time_unit_c_convert_cpp(time_unit), cap);
}

void mimc_set_base_of_backoff_When_connect_fe(user_t *user, time_unit_t time_unit, long base)
{
	User *userObj = (User *)(user->value);
	//userObj->setBaseOfBackoffWhenConnectFe(time_unit_c_convert_cpp(time_unit), base);
	mimcBackoff::getInstance()->getbackoffWhenConnectFe().setBase(time_unit_c_convert_cpp(time_unit), base);
}

void mimc_set_cap_of_backoff_When_connect_fe(user_t *user, time_unit_t time_unit, long cap)
{
	User *userObj = (User *)(user->value);
	//userObj->setCapOfBackoffWhenConnectFe(time_unit_c_convert_cpp(time_unit), cap);
	mimcBackoff::getInstance()->getbackoffWhenConnectFe().setCap(time_unit_c_convert_cpp(time_unit), cap);
}
