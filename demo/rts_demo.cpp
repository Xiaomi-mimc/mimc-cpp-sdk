#include <string>
#include <mimc/user.h>
#include "include/ring_queue.h"
#include <mimc/utils.h>
#include <mimc/mimc_group_message.h>
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

string appAccount1 = "LeiJun";
string appAccount2 = "LinBin";
string appAccount3 = "Jerry";

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

	bool pollMessage(MIMCMessage& message) {
		return messages.pop(message);
	}
	
	void handleOnlineMessage(const MIMCMessage &packets) {

	}
	
	bool onPullNotification() {
		return true;
	}

private:
	ThreadSafeQueue<MIMCMessage> messages;
};

class AVRTSCallEventHandler : public RTSCallEventHandler {
public:
	virtual LaunchedResponse onLaunched(uint64_t callId, const std::string& fromAccount, const std::string& appContent, const std::string& fromResource) {
		LaunchedResponse response = LaunchedResponse(true, LAUNCH_OK);
		callIds.push_back(callId);

		return response;
	}

	virtual void onAnswered(uint64_t callId, bool accepted, const std::string& desc) {
		if (accepted) {
			callIds.push_back(callId);
		} else {
			XMDLoggerWrapper::instance()->error("onAnswered: peer rejected, desc is %s", desc.c_str());
		}
	}

	virtual void onClosed(uint64_t callId, const std::string& desc) {
		XMDLoggerWrapper::instance()->info("onClosed: callId is %llu, desc is %s", callId, desc.c_str());
		for (list<uint64_t>::iterator iter = callIds.begin(); iter != callIds.end(); iter++) {
			if (*iter == callId) {
				callIds.erase(iter);
				break;
			}
		}
	}

	virtual void onData(uint64_t callId, const std::string& fromAccount, const std::string& resource, const std::string& data, RtsDataType dataType, RtsChannelType channelType) {
		user->sendRtsData(callId, data, dataType, channelType);
		if (callDataMap.count(callId) == 0) {
			list<string> dataList;
			dataList.push_back(data);
			callDataMap.insert(pair<uint64_t, list<string>>(callId, dataList));
		} else {
			list<string>& audioDataList = callDataMap.at(callId);
			audioDataList.push_back(data);
		}
	}

	virtual void onSendDataSuccess(uint64_t callId, int dataId, const std::string& ctx) {
        XMDLoggerWrapper::instance()->info("onSendDataSuccess: callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
    }

    virtual void onSendDataFailure(uint64_t callId, int dataId, const std::string& ctx) {
        XMDLoggerWrapper::instance()->warn("onSendDataFailure: callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
    }

	string getChatData(uint64_t callId) {
		string callData;
		list<string>& audioDataList = callDataMap.at(callId);
		for (list<string>::iterator iter = audioDataList.begin(); iter != audioDataList.end(); iter++) {
			const string& audioData = *iter;
			callData.append(audioData);
		}
		return callData;
	}

	list<uint64_t>& getChatIds() {
		return callIds;
	}

	AVRTSCallEventHandler(User* user) {
		this->user = user;
	}

private:
	User* user;
	list<uint64_t> callIds;
	const string LAUNCH_OK = "OK";
	map<uint64_t, list<string>> callDataMap;
};

class RtsDemo {
public:
	static void receiveDataToFile() {
		string resource1 = Utils::generateRandomString(8);

		User* user1 = new User(atoll(appId.c_str()), appAccount1, resource1);
		AVTokenFetcher tokenFetcher(appId, appKey, appSecret, appAccount1);
		user1->registerTokenFetcher(&tokenFetcher);
		AVOnlineStatusHandler onlineStatusHandler;
		user1->registerOnlineStatusHandler(&onlineStatusHandler);
		AVMessageHandler messageHandler;
		user1->registerMessageHandler(&messageHandler);
		AVRTSCallEventHandler rtsCallEventHandler(user1);
		user1->registerRTSCallEventHandler(&rtsCallEventHandler);

		user1->login();

		//sleep(10);
		this_thread::sleep_for(chrono::seconds(10));

		list<uint64_t>& callIds = rtsCallEventHandler.getChatIds();

		for (list<uint64_t>::iterator iter = callIds.begin(); iter != callIds.end(); iter++) {
			uint64_t callId = *iter;
			const string& data = rtsCallEventHandler.getChatData(callId);
			const char* filename = "test.pcm";
			ofstream file(filename, ios::out | ios::binary | ios::ate);
			if (file.is_open()) {
				file.write(data.c_str(), data.size());
				file.close();
			}
			break;
		}

		//sleep(1);
		this_thread::sleep_for(chrono::seconds(1));
		user1->logout();
		//sleep(2);
		this_thread::sleep_for(chrono::seconds(2));
		delete user1;
	}

	static void sendDataFromFile() {
		const char* filename = "test.pcm";
		ifstream file(filename, ios::in | ios::binary | ios::ate);
		long size = file.tellg();
		file.seekg(0, ios::beg);
		char* buffer;
		buffer = new char[size];
		file.read(buffer, size);
		file.close();

		string rtsData(buffer, size);
		delete buffer;
		buffer = NULL;

		string resource1 = Utils::generateRandomString(8);
		User* user1 = new User(atoll(appId.c_str()), appAccount1, resource1);
		AVTokenFetcher tokenFetcher(appId, appKey, appSecret, appAccount1);
		user1->registerTokenFetcher(&tokenFetcher);
		AVOnlineStatusHandler onlineStatusHandler;
		user1->registerOnlineStatusHandler(&onlineStatusHandler);
		AVMessageHandler messageHandler;
		user1->registerMessageHandler(&messageHandler);
		AVRTSCallEventHandler rtsCallEventHandler(user1);
		user1->registerRTSCallEventHandler(&rtsCallEventHandler);

		user1->login();

		//sleep(2);
		this_thread::sleep_for(chrono::seconds(2));

		user1->dialCall("5566", "AUDIO", "JAVA-FEEtSinu");

		//sleep(2);
		this_thread::sleep_for(chrono::seconds(2));

		std::list<uint64_t>& callIds = rtsCallEventHandler.getChatIds();
		if (!callIds.empty()) {
			std::list<uint64_t>::iterator iter;
			for (iter = callIds.begin(); iter != callIds.end(); iter++) {
				uint64_t callId = *iter;
				XMDLoggerWrapper::instance()->info("from callId is %llu", callId);
				user1->sendRtsData(callId, rtsData, AUDIO);
			}
		}
		//sleep(5);
		this_thread::sleep_for(chrono::seconds(5));
		user1->logout();
		//sleep(1);
		this_thread::sleep_for(chrono::seconds(1));
		delete user1;
	}

	static void call() {
		string resource1 = Utils::generateRandomString(8);
		User* user1 = new User(atoll(appId.c_str()), appAccount1, resource1);
		AVTokenFetcher tokenFetcher(appId, appKey, appSecret, appAccount1);
		user1->registerTokenFetcher(&tokenFetcher);
		AVOnlineStatusHandler onlineStatusHandler;
		user1->registerOnlineStatusHandler(&onlineStatusHandler);
		AVMessageHandler messageHandler;
		user1->registerMessageHandler(&messageHandler);
		AVRTSCallEventHandler rtsCallEventHandler(user1);
		user1->registerRTSCallEventHandler(&rtsCallEventHandler);

		user1->login();

		//sleep(2);
		this_thread::sleep_for(chrono::seconds(2));

		user1->dialCall("5566","AUDIO");

		//sleep(60);
		this_thread::sleep_for(chrono::seconds(60));

		user1->logout();
		//sleep(2);
		this_thread::sleep_for(chrono::seconds(2));
		delete user1;
	}

private:
	//static User* user1;
	//static string resource1;
};

int main(int argc, char **argv) {
	RtsDemo::call();
	return 0;
}