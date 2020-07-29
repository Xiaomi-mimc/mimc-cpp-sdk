#include <string>
#include "mimc/user.h"
#include "include/ring_queue.h"
#include "mimc/utils.h"
#include "mimc/mimc_group_message.h"
#include <fstream>
#include <curl/curl.h>
#include <list>
#include <thread>
#include <chrono>

using namespace std;

#ifndef STAGING
string appId = "2882303761517669588";
string appKey = "5111766983588";
string appSecret = "b0L3IOz/9Ob809v8H2FbVg==";
#else
string appId = "2882303761517479657";
string appKey = "5221747911657";
string appSecret = "PtfBeZyC+H8SIM/UXhZx1w==";
#endif

string selfAccount = "";
string selfDesc = "";
string peerAccount = "";
string peerDesc = "";
bool isLauncher = false;

class AVTokenFetcher : public MIMCTokenFetcher {
public:
	string fetchToken() {
		curl_global_init(CURL_GLOBAL_ALL);
		CURL *curl = curl_easy_init();
		CURLcode res;
#ifndef STAGING
		const string url = "https://mimc.chat.xiaomi.net/api/account/token";
#else
		const string url = "http://10.38.162.149/api/account/token";
#endif
		const string body = "{\"appId\":\"" + this->appId + "\",\"appKey\":\"" + this->appKey + "\",\"appSecret\":\"" + this->appSecret + "\",\"appAccount\":\"" + this->appAccount + "\"}";

		XMDLoggerWrapper::instance()->info("In fetchToken, body is %s", body.c_str());
		XMDLoggerWrapper::instance()->info("In fetchToken, url is %s", url.c_str());

		string result;
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

#ifdef _WIN32
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
#endif // _WIN32

			res = curl_easy_perform(curl);
			if (res != CURLE_OK) {
				XMDLoggerWrapper::instance()->error("curl perform error, error code is %d", res);
			}
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
		}

		curl_global_cleanup();

		return result;
	}

	static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
		string *bodyp = (string *)userp;
		bodyp->append((const char *)contents, size * nmemb);

		return size * nmemb;
	}

	AVTokenFetcher(string appId, string appKey, string appSecret, string appAccount)
	{
		this->appId = appId;
		this->appKey = appKey;
		this->appSecret = appSecret;
		this->appAccount = appAccount;
	}
private:
	string appId;
	string appKey;
	string appSecret;
	string appAccount;
};

class AVOnlineStatusHandler : public OnlineStatusHandler {
public:
	void statusChange(OnlineStatus status, const std::string& type, const std::string& reason, const std::string& desc) {
		XMDLoggerWrapper::instance()->info("In statusChange, status is %d, type is %s, reason is %s, desc is %s", status, type.c_str(), reason.c_str(), desc.c_str());
	}
};

class AVMessageHandler : public MessageHandler {
public:
	bool handleMessage(const std::vector<MIMCMessage>& packets) {
		std::vector<MIMCMessage>::const_iterator it = packets.begin();
		for (; it != packets.end(); ++it) {
			messages.push(*it);
		}

		return true;
	}

	bool handleGroupMessage(const std::vector<MIMCGroupMessage>& packets) {
		return true;
	}

	void handleServerAck(const MIMCServerAck &serverAck)
	{
	}

	void handleSendMsgTimeout(const MIMCMessage& message) {

	}

	void handleSendGroupMsgTimeout(const MIMCGroupMessage& groupMessage) {

	}

	MIMCMessage& pollMessage() {
		MIMCMessage messagePtr;
		messages.pop(messagePtr);
		return messagePtr;
	}

	void handleOnlineMessage(const MIMCMessage &packets)
	{
		
	}

	bool onPullNotification()
	{
		return true;
	}

private:
	ThreadSafeQueue<MIMCMessage> messages;
};

class AVRTSCallEventHandler : public RTSCallEventHandler {
public:

	virtual LaunchedResponse onLaunched(uint64_t callId, const std::string& fromAccount, const std::string& appContent, const std::string& fromResource) {

        XMDLoggerWrapper::instance()->info("In onLaunched, callId is %llu, fromAccount is %s, appContent is %s, fromResource is %s", callId, fromAccount.c_str(), appContent.c_str(), fromResource.c_str());

        return LaunchedResponse(true, LAUNCH_OK);
	}

	virtual void onAnswered(uint64_t callId, bool accepted, const std::string& desc) {

        XMDLoggerWrapper::instance()->info("In onAnswered, callId is %llu, accepted is %d, desc is %s", callId, accepted, desc.c_str());
	}

	virtual void onClosed(uint64_t callId, const std::string& desc) {
		XMDLoggerWrapper::instance()->info("onClosed: callId is %llu, desc is %s, user(%s)", callId, desc.c_str(), user->getAppAccount().c_str());

        if (!isLauncher) {
            user->logout();
            delete user;
            user = NULL;
        }
	}

	virtual void onData(uint64_t callId, const std::string& fromAccount, const std::string& resource, const std::string& data, RtsDataType dataType, RtsChannelType channelType) {

        XMDLoggerWrapper::instance()->info("In onData, callId is %llu, fromAccount is %s, resource is %s, dataLen is %d, data is %s, dataType is %d, channelType is %d",
                callId, fromAccount.c_str(), resource.c_str(), data.length(), data.c_str(), dataType, channelType);

        if (!isLauncher) {
            if (user->sendRtsData(callId, "echo: " + data, dataType, channelType) == -1) {
                XMDLoggerWrapper::instance()->error("send echo data failed");
            }
        }
	}

	virtual void onSendDataSuccess(uint64_t callId, int dataId, const std::string& ctx) {
        XMDLoggerWrapper::instance()->info("onSendDataSuccess: callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
    }

    virtual void onSendDataFailure(uint64_t callId, int dataId, const std::string& ctx) {
        XMDLoggerWrapper::instance()->warn("onSendDataFailure: callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
    }

	AVRTSCallEventHandler(User* user) {
		this->user = user;
	}

private:
	User* user;
	const string LAUNCH_OK = "OK";
};

class RtsDemo {
public:

	static void run() {
		string resource1 = Utils::generateRandomString(8);
		User* user1 = new User(atoll(appId.c_str()), selfAccount, resource1);
		user1->enableP2P(true);
		MIMCTokenFetcher* tokenFetcher = new AVTokenFetcher(appId, appKey, appSecret, selfAccount);
		user1->registerTokenFetcher(tokenFetcher);
		user1->registerOnlineStatusHandler(new AVOnlineStatusHandler());
		user1->registerMessageHandler(new AVMessageHandler());
		AVRTSCallEventHandler* rtsCallEventHandler = new AVRTSCallEventHandler(user1);
		user1->registerRTSCallEventHandler(rtsCallEventHandler);

		user1->login();

        time_t loginTs = time(NULL);
        XMDLoggerWrapper::instance()->info("user %s called login", user1->getAppAccount().c_str());
        while (time(NULL) - loginTs < LOGIN_TIMEOUT && user1->getOnlineStatus() == Offline) {
            //usleep(50000);
            this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (user1->getOnlineStatus() != Online) {
            XMDLoggerWrapper::instance()->error("user %s login timeout. program exit", user1->getAppAccount().c_str());
            delete user1;
            exit(-1);
        }

		//sleep(2);
		this_thread::sleep_for(chrono::seconds(2));

        uint64_t callId = 0;
        if (isLauncher) {

            this_thread::sleep_for(chrono::seconds(10));

            callId = user1->dialCall(peerAccount, peerDesc);
            if (callId == 0) {
                XMDLoggerWrapper::instance()->error("user %s dialCall failed. program exit", user1->getAppAccount().c_str());
                delete user1;
                exit(-1);
            }

            sendRtsData(user1, callId);
        }

		//sleep(10);
		this_thread::sleep_for(chrono::seconds(10));

        if (isLauncher) {
            user1->closeCall(callId); // request to close session
            user1->logout();
            //sleep(2);
            this_thread::sleep_for(chrono::seconds(2));
            delete user1;
        } else {
            this_thread::sleep_for(chrono::seconds(5 * 60));
        }
	}

    static void sendRtsData(User* user1, uint64_t callId)
    {
        int sendFailed = 0;
        const int dataSizeKB = 150;
        const int dataSpeedKB = 300;
        const int durationSec = 100;
        const int COUNT = (durationSec * dataSpeedKB) / dataSizeKB;
        const int TIMEVAL_US = 1000 * (1000 * dataSizeKB / dataSpeedKB);

        for (int i = 0; i < COUNT; i++) {

            string sendData = "hello from ";
            sendData += selfAccount;

            RtsDataType dataType = RtsDataType::AUDIO;
            if (isLauncher) {
                dataType = RtsDataType::VIDEO;
            }

            if (user1->sendRtsData(callId, sendData, dataType, CHANNEL_AUTO) == -1) {
                XMDLoggerWrapper::instance()->warn("DATA_ID:%d, SEND FAILED", i);
                sendFailed++;
            }

            this_thread::sleep_for(std::chrono::microseconds(TIMEVAL_US));
        }

        this_thread::sleep_for(std::chrono::seconds(5));

        XMDLoggerWrapper::instance()->warn("\n>>>>>>>>>>>>>>>>>>>  PERFORMANCE TEST RESULT:  <<<<<<<<<<<<<<<<<");
        XMDLoggerWrapper::instance()->warn("SEND %d DATAS DURING %ds, DATA SIZE: %dKB, SPEED: %dKB/S", COUNT, durationSec, dataSizeKB, dataSpeedKB);
        XMDLoggerWrapper::instance()->warn("FAILED: %d\n", sendFailed);
    }
};

int main(int argc, char **argv) {

    if (argc < 5) {
        XMDLoggerWrapper::instance()->error("usage: ./rts_cpp_demo self_account self_desc peer_account peer_desc [launcher]. if specify launcher, it means that dialCall is requested on current side");
        return -1;
    }

    selfAccount = argv[1];
    selfDesc = argv[2];
    peerAccount = argv[3];
    peerDesc = argv[4];

    if (argc >= 6) {
        isLauncher = true;
        XMDLoggerWrapper::instance()->error("isLauncher(true)");
    }

	RtsDemo::run();
	return 0;
}
