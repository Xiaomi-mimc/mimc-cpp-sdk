#include <mimc/user_c.h>
#include <mimc/user.h>

class CTokenFetcher : public MIMCTokenFetcher {
public:
	CTokenFetcher(const token_fetcher_t& token_fetcher) : _token_fetcher(token_fetcher) {

	}

	std::string fetchToken() {
		const char* tokenp = _token_fetcher.fetch_token();
	    std::string token(tokenp);
		free((char *)tokenp);
	    return token;
	}

private:
	token_fetcher_t _token_fetcher;
};

class COnlineStatusHandler : public OnlineStatusHandler {
public:
	COnlineStatusHandler(const online_status_handler_t& online_status_handler) : _online_status_handler(online_status_handler) {

	}

	void statusChange(OnlineStatus status, std::string errType, std::string errReason, std::string errDescription) {
		online_status_t online_status = OFFLINE;
		if (status == Online) {
			online_status = ONLINE;
		}
		_online_status_handler.status_change(online_status, errType.c_str(), errReason.c_str(), errDescription.c_str());
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
		return LaunchedResponse(launched_response.accepted, launched_response.err_msg);
	}

	void onAnswered(uint64_t callId, bool accepted, const std::string errMsg) {
		_rtscall_event_handler.on_answered(callId, accepted, errMsg.c_str());
	}

	void onClosed(uint64_t callId, const std::string errMsg) {
		_rtscall_event_handler.on_closed(callId, errMsg.c_str());
	}

	void handleData(uint64_t callId, const std::string data, RtsDataType dataType, RtsChannelType channelType) {
		data_type_t data_type;
		switch(dataType) {
			case AUDIO:
				data_type = AUD;
				break;
			case VIDEO:
				data_type = VID;
				break;
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

		_rtscall_event_handler.handle_data(callId, data.c_str(), data.length(), data_type, channel_type);
	}

	void handleSendDataSucc(uint64_t callId, int groupId, const std::string ctx) {
		_rtscall_event_handler.handle_send_data_succ(callId, groupId, ctx.c_str(), ctx.length());
	}

	void handleSendDataFail(uint64_t callId, int groupId, const std::string ctx) {
		_rtscall_event_handler.handle_send_data_fail(callId, groupId, ctx.c_str(), ctx.length());
	}

private:
	rtscall_event_handler_t _rtscall_event_handler;
};

void mimc_rtc_init(user_t* user, int64_t appid, const char* appaccount, const char* resource, const char* cachepath) {
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
}

void mimc_rtc_fini(user_t* user) {
	User* userObj = (User*)(user->value);
	delete userObj;
	user->value = NULL;
}

bool mimc_rtc_login(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->login();
}

bool mimc_rtc_logout(user_t* user) {
	User* userObj = (User*)(user->value);
	return userObj->logout();
}

void mimc_rtc_set_max_callnum(user_t* user, unsigned int num) {
	User* userObj = (User*)(user->value);
	userObj->setMaxCallNum(num);
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
	return userObj->dialCall(to_appaccount, appcontent, to_resource);
}

void mimc_rtc_close_call(user_t* user, uint64_t callid, const char* bye_reason) {
	User* userObj = (User*)(user->value);
	userObj->closeCall(callid, bye_reason);
}

bool mimc_rtc_send_data(user_t* user, uint64_t callid, const char* data, const int data_len, const data_type_t data_type, const channel_type_t channel_type, const char* ctx, const int ctx_len, const bool can_be_dropped, const data_priority_t data_priority, const unsigned int resend_count) {
	User* userObj = (User*)(user->value);
	RtsDataType dataType;
	switch(data_type) {
		case AUD:
			dataType = AUDIO;
			break;
		case VID:
			dataType = VIDEO;
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

void mimc_rtc_register_token_fetcher(user_t* user, const token_fetcher_t* token_fetcher) {
	User* userObj = (User*)(user->value);
	userObj->registerTokenFetcher(new CTokenFetcher(*token_fetcher));
}

void mimc_rtc_register_online_status_handler(user_t* user, const online_status_handler_t* online_status_handler) {
	User* userObj = (User*)(user->value);
	userObj->registerOnlineStatusHandler(new COnlineStatusHandler(*online_status_handler));
}

void mimc_rtc_register_rtscall_event_handler(user_t* user, const rtscall_event_handler_t* rtscall_event_handler) {
	User* userObj = (User*)(user->value);
	userObj->registerRTSCallEventHandler(new CRTSCallEventHandler(*rtscall_event_handler));
}