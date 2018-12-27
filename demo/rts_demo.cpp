#include <string>
#include <mimc/user.h>
#include <mimc/threadsafe_queue.h>
#include <mimc/utils.h>
#include <fstream>
#include <curl/curl.h>
#include <list>

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
	void statusChange(OnlineStatus status, std::string errType, std::string errReason, std::string errDescription) {
		XMDLoggerWrapper::instance()->info("In statusChange, status is %d, errType is %s, errReason is %s, errDescription is %s", status, errType.c_str(), errReason.c_str(), errDescription.c_str());
	}
};

class AVMessageHandler : public MessageHandler {
public:
	void handleMessage(std::vector<MIMCMessage> packets) {
		std::vector<MIMCMessage>::iterator it = packets.begin();
		for (; it != packets.end(); ++it) {
			messages.push(*it);
		}
	}

	void handleServerAck(std::string packetId, int64_t sequence, time_t timestamp, std::string errorMsg) {

	}

	void handleSendMsgTimeout(MIMCMessage message) {

	}

	MIMCMessage* pollMessage() {
		MIMCMessage *messagePtr;
		messages.pop(&messagePtr);
		return messagePtr;
	}

private:
	ThreadSafeQueue<MIMCMessage> messages;
};

class AVRTSCallEventHandler : public RTSCallEventHandler {
public:
	virtual LaunchedResponse onLaunched(uint64_t callId, const std::string fromAccount, const std::string appContent, const std::string fromResource) {
		LaunchedResponse response = LaunchedResponse(true, LAUNCH_OK);
		callIds.push_back(callId);

		return response;
	}

	virtual void onAnswered(uint64_t callId, bool accepted, const std::string errMsg) {
		if (accepted) {
			callIds.push_back(callId);
		} else {
			XMDLoggerWrapper::instance()->error("onAnswered: peer rejected, error message is %s", errMsg.c_str());
		}
	}

	virtual void onClosed(uint64_t callId, const std::string errMsg) {
		XMDLoggerWrapper::instance()->info("onClosed: callId is %llu, error message is %s", callId, errMsg.c_str());
		for (list<uint64_t>::iterator iter = callIds.begin(); iter != callIds.end(); iter++) {
			if (*iter == callId) {
				callIds.erase(iter);
				break;
			}
		}
	}

	virtual void handleData(uint64_t callId, const std::string data, RtsDataType dataType, RtsChannelType channelType) {
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

	virtual void handleSendDataSucc(uint64_t callId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->info("handleSendDataSucc: callId is %llu, groupId is %d, ctx is %s", callId, groupId, ctx.c_str());
    }

    virtual void handleSendDataFail(uint64_t callId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->warn("handleSendDataFail: callId is %llu, groupId is %d, ctx is %s", callId, groupId, ctx.c_str());
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
		MIMCTokenFetcher* tokenFetcher = new AVTokenFetcher(appId, appKey, appSecret, appAccount1);
		user1->registerTokenFetcher(tokenFetcher);
		user1->registerOnlineStatusHandler(new AVOnlineStatusHandler());
		user1->registerMessageHandler(new AVMessageHandler());
		AVRTSCallEventHandler* rtsCallEventHandler = new AVRTSCallEventHandler(user1);
		user1->registerRTSCallEventHandler(rtsCallEventHandler);

		user1->login();

		sleep(10);

		list<uint64_t>& callIds = rtsCallEventHandler->getChatIds();

		for (list<uint64_t>::iterator iter = callIds.begin(); iter != callIds.end(); iter++) {
			uint64_t callId = *iter;
			const string& data = rtsCallEventHandler->getChatData(callId);
			const char* filename = "test.pcm";
			ofstream file(filename, ios::out | ios::binary | ios::ate);
			if (file.is_open()) {
				file.write(data.c_str(), data.size());
				file.close();
			}
			break;
		}

		sleep(1);
		user1->logout();
		sleep(2);
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
		MIMCTokenFetcher* tokenFetcher = new AVTokenFetcher(appId, appKey, appSecret, appAccount1);
		user1->registerTokenFetcher(tokenFetcher);
		user1->registerOnlineStatusHandler(new AVOnlineStatusHandler());
		user1->registerMessageHandler(new AVMessageHandler());
		AVRTSCallEventHandler* rtsCallEventHandler = new AVRTSCallEventHandler(user1);
		user1->registerRTSCallEventHandler(rtsCallEventHandler);

		user1->login();

		sleep(2);

		user1->dialCall("5566", "AUDIO", "JAVA-FEEtSinu");

		sleep(2);
		std::list<uint64_t>& callIds = rtsCallEventHandler->getChatIds();
		if (!callIds.empty()) {
			std::list<uint64_t>::iterator iter;
			for (iter = callIds.begin(); iter != callIds.end(); iter++) {
				uint64_t callId = *iter;
				XMDLoggerWrapper::instance()->info("from callId is %llu", callId);
				user1->sendRtsData(callId, rtsData, AUDIO);
			}
		}
		sleep(5);
		user1->logout();
		sleep(1);
		delete user1;
	}

	static void call() {
		string resource1 = Utils::generateRandomString(8);
		User* user1 = new User(atoll(appId.c_str()), appAccount1, resource1);
		MIMCTokenFetcher* tokenFetcher = new AVTokenFetcher(appId, appKey, appSecret, appAccount1);
		user1->registerTokenFetcher(tokenFetcher);
		user1->registerOnlineStatusHandler(new AVOnlineStatusHandler());
		user1->registerMessageHandler(new AVMessageHandler());
		AVRTSCallEventHandler* rtsCallEventHandler = new AVRTSCallEventHandler(user1);
		user1->registerRTSCallEventHandler(rtsCallEventHandler);

		user1->login();

		sleep(2);

		user1->dialCall("5566","AUDIO");

		sleep(60);

		user1->logout();
		sleep(2);
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