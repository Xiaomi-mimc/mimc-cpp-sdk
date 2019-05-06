#include <mimc/user.h>
#include <mimc/threadsafe_queue.h>
#include <string.h>
#include <curl/curl.h>
#include <list>
#include <chrono>
#include <thread>

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
string appAccount1 = "LeiJun222";
string appAccount2 = "LinBin333";
string appAccount3 = "Jerry";

class TestOnlineStatusHandler : public OnlineStatusHandler {
public:
    void statusChange(OnlineStatus status, std::string type, std::string reason, std::string desc) {
        XMDLoggerWrapper::instance()->info("In statusChange, status is %d, type is %s, reason is %s, desc is %s", status, type.c_str(), reason.c_str(), desc.c_str());
    }
};

class TestMessageHandler : public MessageHandler {
public:
    void handleMessage(std::vector<MIMCMessage> packets) {
        std::vector<MIMCMessage>::iterator it = packets.begin();
        for (; it != packets.end(); ++it) {
            messages.push(*it);
            MIMCMessage& message = *it;
            printf("recv message, payload is %s, bizType is %s\n", message.getPayload().c_str(), message.getBizType().c_str());
        }
    }

	void handleGroupMessage(std::vector<MIMCGroupMessage> packets) {

	}

    void handleServerAck(std::string packetId, int64_t sequence, time_t timestamp, std::string desc) {
        
    }

    void handleSendMsgTimeout(MIMCMessage message) {
        
    }

	void handleSendGroupMsgTimeout(MIMCGroupMessage groupMessage) {

	}

    bool pollMessage(MIMCMessage& message) {
        return messages.pop(message);
    }

private:
    ThreadSafeQueue<MIMCMessage> messages;
};

class TestRTSCallEventHandler : public RTSCallEventHandler {
public:
    LaunchedResponse onLaunched(uint64_t callId, const std::string fromAccount, const std::string appContent, const std::string fromResource) {
        XMDLoggerWrapper::instance()->info("In onLaunched, callId is %llu, fromAccount is %s, appContent is %s, fromResource is %s", callId, fromAccount.c_str(), appContent.c_str(), fromResource.c_str());
        if (appContent != this->appContent) {
            return LaunchedResponse(false, LAUNCH_ERR_ILLEGALSIG);
        }
        XMDLoggerWrapper::instance()->info("In onLaunched, appContent is equal to this->appContent");
        callIds.push_back(callId);
        return LaunchedResponse(true, LAUNCH_OK);
    }

    void onAnswered(uint64_t callId, bool accepted, const std::string desc) {
        XMDLoggerWrapper::instance()->info("In onAnswered, callId is %llu, accepted is %d, desc is %s", callId, accepted, desc.c_str());
        if (accepted) {
            callIds.push_back(callId);
        }
    }

    void onClosed(uint64_t callId, const std::string desc) {
        XMDLoggerWrapper::instance()->info("In onClosed, callId is %llu, desc is %s", callId, desc.c_str());
        std::list<uint64_t>::iterator iter;
        for (iter = callIds.begin(); iter != callIds.end();) {
            if(*iter == callId) {
                iter = callIds.erase(iter);
                break;
            } else {
                iter++;
            }
        }
    }

    void handleData(uint64_t callId, const std::string data, RtsDataType dataType, RtsChannelType channelType) {
        XMDLoggerWrapper::instance()->info("In handleData, callId is %llu, data is %s, dataType is %d", callId, data.c_str(), dataType);
    }

    void handleSendDataSucc(uint64_t callId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->info("In handleSendDataSucc, callId is %llu, groupId is %d, ctx is %s", callId, groupId, ctx.c_str());
    }

    void handleSendDataFail(uint64_t callId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->warn("In handleSendDataFail, callId is %llu, groupId is %d, ctx is %s", callId, groupId, ctx.c_str());
    }

    std::list<uint64_t>& getChatIds() {return this->callIds;}

    const std::string& getAppContent() {return this->appContent;}

    TestRTSCallEventHandler(std::string appContent) {
        this->appContent = appContent;
    }
private:
    std::string appContent;
    const std::string LAUNCH_OK = "OK";
    const std::string LAUNCH_ERR_ILLEGALSIG = "ILLEGALSIG";
    std::list<uint64_t> callIds;
};

class TestTokenFetcher : public MIMCTokenFetcher {
public:
    string fetchToken() {
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

        return result;
    }

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        string *bodyp = (string *)userp;
        bodyp->append((const char *)contents, size * nmemb);

        return size * nmemb;
    }

    TestTokenFetcher(string appId, string appKey, string appSecret, string appAccount)
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

class MimcDemo {
public:
    static void testP2PSendOneMessage() {
        curl_global_init(CURL_GLOBAL_ALL);
        User* from = new User(atoll(appId.c_str()), appAccount1);
        User* to = new User(atoll(appId.c_str()), appAccount2);

        TestMessageHandler fromMessageHandler;
        TestMessageHandler toMessageHandler;

        TestTokenFetcher fromTokenFetcher(appId, appKey, appSecret, appAccount1);
        TestTokenFetcher toTokenFetcher(appId, appKey, appSecret, appAccount2);

        TestOnlineStatusHandler fromOnlineStatusHandler;
        TestOnlineStatusHandler toOnlineStatusHandler;

        from->registerTokenFetcher(&fromTokenFetcher);
        from->registerOnlineStatusHandler(&fromOnlineStatusHandler);
        from->registerMessageHandler(&fromMessageHandler);

        to->registerTokenFetcher(&toTokenFetcher);
        to->registerOnlineStatusHandler(&toOnlineStatusHandler);
        to->registerMessageHandler(&toMessageHandler);

        if (!from->login()) {
            return;
        }

        if (!to->login()) {
            return;
        }

        //sleep(4);
		std::this_thread::sleep_for(std::chrono::seconds(4));

        string payload1 = "WITH MIMC,WE CAN FLY HIGHER！";
        string packetId_sent1 = from->sendMessage(to->getAppAccount(), payload1);

        //usleep(300000);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

        MIMCMessage message1;
        if (toMessageHandler.pollMessage(message1)) {
            
        }

        from->logout();
        //usleep(400000);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));

        to->logout();
        //usleep(400000);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));

        delete from;
        from = NULL;
        delete to;
        to = NULL;
        curl_global_cleanup();
    }

    static void testP2PSendMessages() {
        curl_global_init(CURL_GLOBAL_ALL);
        User* from = new User(atoll(appId.c_str()), appAccount1);
        User* to = new User(atoll(appId.c_str()), appAccount2);

        TestMessageHandler fromMessageHandler;
        TestMessageHandler toMessageHandler;

        TestTokenFetcher fromTokenFetcher(appId, appKey, appSecret, appAccount1);
        TestTokenFetcher toTokenFetcher(appId, appKey, appSecret, appAccount2);

        TestOnlineStatusHandler fromOnlineStatusHandler;
        TestOnlineStatusHandler toOnlineStatusHandler;

        from->registerTokenFetcher(&fromTokenFetcher);
        from->registerOnlineStatusHandler(&fromOnlineStatusHandler);
        from->registerMessageHandler(&fromMessageHandler);

        to->registerTokenFetcher(&toTokenFetcher);
        to->registerOnlineStatusHandler(&toOnlineStatusHandler);
        to->registerMessageHandler(&toMessageHandler);

        from->login();
        //sleep(2);
		std::this_thread::sleep_for(std::chrono::seconds(2));

        to->login();
        //sleep(2);
		std::this_thread::sleep_for(std::chrono::seconds(2));

        MIMCMessage message;
        const int MAX_NUM = 100;
        const int GOON_NUM = 20;
        int num = 0;
        while (num++ < MAX_NUM) {
            
            string payload1 = "With mimc,we can communicate much easier！";
            string packetId_sent1 = from->sendMessage(to->getAppAccount(), payload1);
            //usleep(300000);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
            while (toMessageHandler.pollMessage(message))
            {
                
            }

            string payload2 = "Yes~I feel it";
            string packetId_sent2 = to->sendMessage(from->getAppAccount(), payload2);
            //usleep(300000);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
            while (fromMessageHandler.pollMessage(message))
            {
                
            }
        }

        num = 0;

        //sleep(10);
		std::this_thread::sleep_for(std::chrono::seconds(10));

        while (num++ < GOON_NUM) {
            
            string payload3 = "Let's go on!";
            string packetId_sent1 = from->sendMessage(to->getAppAccount(), payload3);
            //usleep(300000);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
            while (toMessageHandler.pollMessage(message))
            {
                
            }

            string payload4 = "OK!";
            string packetId_sent2 = to->sendMessage(from->getAppAccount(), payload4);
            //usleep(300000);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
            while (fromMessageHandler.pollMessage(message))
            {
                
            }
        }

        from->logout();
        //usleep(400000);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));

        to->logout();
        //usleep(400000);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));

        delete from;
        from = NULL;
        delete to;
        to = NULL;
        curl_global_cleanup();
    }

};

int main(int argc, char **argv) {
    /* A用户发送单条消息给B用户 */
//    MimcDemo::testP2PSendOneMessage();
    /* A用户与B用户互发多条消息 */
    MimcDemo::testP2PSendMessages();
    return 0;
}