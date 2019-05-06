#include <mimc/user_c.h>
#include <mimc/user.h>
#include <XMDTransceiver.h>
#include <curl/curl.h>

class CTokenFetcher : public MIMCTokenFetcher {
public:
	CTokenFetcher(std::string app_account)
	 : _app_account(app_account) {
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
/*
	std::string fetchToken() {
        XMDLoggerWrapper::instance()->info("In %s, line %d", __FUNCTION__, __LINE__);
		if (_app_account == "") {
            XMDLoggerWrapper::instance()->info("In %s, line %d:_app_account is empty", __FUNCTION__, __LINE__);
        } else {
           XMDLoggerWrapper::instance()->info("In %s, line %d:_app_account is %s", __FUNCTION__, __LINE__, _app_account.c_str()); 
        }
        XMDLoggerWrapper::instance()->info("In %s, line %d:_token_fetcher.fetch_token is %p", __FUNCTION__, __LINE__, _token_fetcher.fetch_token);
        const char* tokenp = _token_fetcher.fetch_token(_app_account.c_str());
        XMDLoggerWrapper::instance()->info("In %s, line %d", __FUNCTION__, __LINE__);	    
        std::string token(tokenp);
		free((char *)tokenp);
	    return token;
	}
*/
    std::string fetchToken() {
        CURL *curl = curl_easy_init();
        CURLcode res;
#ifndef STAGING
        const std::string url = "https://mimc.chat.xiaomi.net/api/account/token";
#else
        const std::string url = "http://10.38.162.149/api/account/token";
#endif
        const std::string body = "{\"appId\":\"" + _app_id + "\",\"appKey\":\"" + _app_key + "\",\"appSecret\":\"" + _app_secret + "\",\"appAccount\":\"" + _app_account + "\"}";
        std::string result;
        if (curl) {
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
            if (res != CURLE_OK) {
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

class COnlineStatusHandler : public OnlineStatusHandler {
public:
	COnlineStatusHandler(const online_status_handler_t& online_status_handler) : _online_status_handler(online_status_handler) {

	}

	void statusChange(OnlineStatus status, std::string type, std::string reason, std::string desc) {
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

	LaunchedResponse onLaunched(uint64_t callId, const std::string fromAccount, const std::string appContent, const std::string fromResource) {
		const launched_response_t& launched_response = _rtscall_event_handler.on_launched(callId, fromAccount.c_str(), appContent.c_str(), appContent.length(), fromResource.c_str());
		return LaunchedResponse(launched_response.accepted, launched_response.desc);
	}

	void onAnswered(uint64_t callId, bool accepted, const std::string desc) {
		_rtscall_event_handler.on_answered(callId, accepted, desc.c_str());
	}

	void onClosed(uint64_t callId, const std::string desc) {
		_rtscall_event_handler.on_closed(callId, desc.c_str());
	}

	void onData(uint64_t callId, const std::string fromAccount, const std::string resource, const std::string data, RtsDataType dataType, RtsChannelType channelType) {
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

	void onSendDataSuccess(uint64_t callId, int dataId, const std::string ctx) {
		_rtscall_event_handler.on_send_data_success(callId, dataId, ctx.c_str(), ctx.length());
	}

	void onSendDataFailure(uint64_t callId, int dataId, const std::string ctx) {
		_rtscall_event_handler.on_send_data_failure(callId, dataId, ctx.c_str(), ctx.length());
	}

private:
	rtscall_event_handler_t _rtscall_event_handler;
};

void mimc_rtc_init(user_t* user, int64_t appid, const char* appaccount, const char* resource, const char* cachepath) {
	curl_global_init(CURL_GLOBAL_ALL);
	std::string appaccountStr(appaccount);
	std::string resourceStr;
	if (resource == NULL) {
		resourceStr = "";
	} else {
		resourceStr = resource;
	}
	std::string cachepathStr;
	if (cachepath == NULL) {
		cachepathStr = "";
	} else {
		cachepathStr = cachepath;
	}
	user->value = new User(appid, appaccountStr, resourceStr, cachepathStr);
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

void mimc_rtc_set_sendbuffer_size(user_t* user, int size) {
	User* userObj = (User*)(user->value);
	userObj->setSendBufferSize(size);
}

void mimc_rtc_set_recvbuffer_size(user_t* user, int size) {
	User* userObj = (User*)(user->value);
	userObj->setRecvBufferSize(size);
}

int mimc_rtc_get_sendbuffer_size(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->getSendBufferSize();
}

int mimc_rtc_get_recvbuffer_size(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->getRecvBufferSize();
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

	std::string rtsData(data, data_len);
	std::string rtsCtx(ctx, ctx_len);
	return userObj->sendRtsData(callid, rtsData, dataType, channelType, rtsCtx, can_be_dropped, dataPriority, resend_count);
}

void mimc_rtc_register_token_fetcher(user_t* user, const char* app_account) {
	User* userObj = (User*)(user->value);
	MIMCTokenFetcher* tokenFetcher = new CTokenFetcher(app_account);
	userObj->registerTokenFetcher(tokenFetcher);
	user->token_fetcher = tokenFetcher;
}

void mimc_rtc_register_online_status_handler(user_t* user, const online_status_handler_t* online_status_handler) {
	User* userObj = (User*)(user->value);
	OnlineStatusHandler* onlineStatusHandler = new COnlineStatusHandler(*online_status_handler);
	userObj->registerOnlineStatusHandler(onlineStatusHandler);
	user->online_status_handler = onlineStatusHandler;
}

void mimc_rtc_register_rtscall_event_handler(user_t* user, const rtscall_event_handler_t* rtscall_event_handler) {
	User* userObj = (User*)(user->value);
	RTSCallEventHandler* rtsCallEventHandler = new CRTSCallEventHandler(*rtscall_event_handler);
	userObj->registerRTSCallEventHandler(rtsCallEventHandler);
	user->rtscall_event_handler = rtsCallEventHandler;
}
