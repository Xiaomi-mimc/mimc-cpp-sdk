#include <mimc/user.h>
#include <mimc/threadsafe_queue.h>
#include <string.h>
#include <unistd.h>
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
string appAccount1 = "LeiJun222";
string appAccount2 = "LinBin333";
string appAccount3 = "Jerry";

class TestOnlineStatusHandler : public OnlineStatusHandler {
public:
    void statusChange(OnlineStatus status, std::string errType, std::string errReason, std::string errDescription) {
        XMDLoggerWrapper::instance()->info("In statusChange, status is %d, errType is %s, errReason is %s, errDescription is %s", status, errType.c_str(), errReason.c_str(), errDescription.c_str());
    }
};

class TestMessageHandler : public MessageHandler {
public:
    void handleMessage(std::vector<MIMCMessage> packets) {
        std::vector<MIMCMessage>::iterator it = packets.begin();
        for (; it != packets.end(); ++it) {
            messages.push(*it);
        }
    }

    void handleServerAck(std::string packetId, long sequence, long timestamp, std::string errorMsg) {
        
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

class TestRTSCallEventHandler : public RTSCallEventHandler {
public:
    LaunchedResponse onLaunched(long chatId, const std::string fromAccount, const std::string appContent, const std::string fromResource) {
        XMDLoggerWrapper::instance()->info("In onLaunched, chatId is %ld, fromAccount is %s, appContent is %s, fromResource is %s", chatId, fromAccount.c_str(), appContent.c_str(), fromResource.c_str());
        if (appContent != this->appContent) {
            return LaunchedResponse(false, LAUNCH_ERR_ILLEGALSIG);
        }
        XMDLoggerWrapper::instance()->info("In onLaunched, appContent is equal to this->appContent");
        chatIds.push_back(chatId);
        return LaunchedResponse(true, LAUNCH_OK);
    }

    void onAnswered(long chatId, bool accepted, const std::string errmsg) {
        XMDLoggerWrapper::instance()->info("In onAnswered, chatId is %ld, accepted is %d, errmsg is %s", chatId, accepted, errmsg.c_str());
        if (accepted) {
            chatIds.push_back(chatId);
        }
    }

    void onClosed(long chatId, const std::string errmsg) {
        XMDLoggerWrapper::instance()->info("In onClosed, chatId is %ld, errmsg is %s", chatId, errmsg.c_str());
        std::list<long>::iterator iter;
        for (iter = chatIds.begin(); iter != chatIds.end();) {
            if(*iter == chatId) {
                iter = chatIds.erase(iter);
                break;
            } else {
                iter++;
            }
        }
    }

    void handleData(long chatId, const std::string data, RtsDataType dataType, RtsChannelType channelType) {
        XMDLoggerWrapper::instance()->info("In handleData, chatId is %ld, data is %s, dataType is %d", chatId, data.c_str(), dataType);
    }

    void handleSendDataSucc(long chatId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->info("In handleSendDataSucc, chatId is %ld, groupId is %d, ctx is %s", chatId, groupId, ctx.c_str());
    }

    void handleSendDataFail(long chatId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->warn("In handleSendDataFail, chatId is %ld, groupId is %d, ctx is %s", chatId, groupId, ctx.c_str());
    }

    std::list<long>& getChatIds() {return this->chatIds;}

    const std::string& getAppContent() {return this->appContent;}

    TestRTSCallEventHandler(std::string appContent) {
        this->appContent = appContent;
    }
private:
    std::string appContent;
    const std::string LAUNCH_OK = "OK";
    const std::string LAUNCH_ERR_ILLEGALSIG = "ILLEGALSIG";
    std::list<long> chatIds;
};

class TestTokenFetcher : public MIMCTokenFetcher {
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
        User* from = new User(atol(appId.c_str()), appAccount1);
        User* to = new User(atol(appId.c_str()), appAccount2);
        TestMessageHandler* fromMessageHandler = new TestMessageHandler();
        TestMessageHandler* toMessageHandler = new TestMessageHandler();

        from->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount1));
        from->registerOnlineStatusHandler(new TestOnlineStatusHandler());
        from->registerMessageHandler(fromMessageHandler);

        to->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount2));
        to->registerOnlineStatusHandler(new TestOnlineStatusHandler());
        to->registerMessageHandler(toMessageHandler);

        if (!from->login()) {
            return;
        }

        if (!to->login()) {
            return;
        }

        usleep(500000);

        string msg1 = "WITH MIMC,WE CAN FLY HIGHER！";
        string packetId_sent1 = from->sendMessage(to->getAppAccount(), msg1);
        

        usleep(300000);

        MIMCMessage *message1 = toMessageHandler->pollMessage();
        if (message1 != NULL) {
            
        }

        from->logout();
        usleep(400000);

        to->logout();
        usleep(400000);

        delete from;
        from = NULL;
        delete to;
        to = NULL;
    }

    static void testP2PSendMessages() {
        User* from = new User(atol(appId.c_str()), appAccount1);
        User* to = new User(atol(appId.c_str()), appAccount2);
        TestMessageHandler* fromMessageHandler = new TestMessageHandler();
        TestMessageHandler* toMessageHandler = new TestMessageHandler();


        from->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount1));
        from->registerOnlineStatusHandler(new TestOnlineStatusHandler());
        from->registerMessageHandler(fromMessageHandler);

        to->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount2));
        to->registerOnlineStatusHandler(new TestOnlineStatusHandler());
        to->registerMessageHandler(toMessageHandler);

        from->login();
        usleep(500000);

        to->login();
        usleep(500000);

        MIMCMessage *message;
        const int MAX_NUM = 100;
        const int GOON_NUM = 20;
        int num = 0;
        while (num++ < MAX_NUM) {
            
            string msg1 = "With mimc,we can communicate much easier！";
            string packetId_sent1 = from->sendMessage(to->getAppAccount(), msg1);
            usleep(300000);
            while (message = toMessageHandler->pollMessage())
            {
                
            }

            string msg2 = "Yes~I feel it";
            string packetId_sent2 = to->sendMessage(from->getAppAccount(), msg2);
            usleep(300000);
            while (message = fromMessageHandler->pollMessage())
            {
                
            }
        }

        num = 0;
        sleep(60);
        while (num++ < GOON_NUM) {
            
            string msg3 = "Let's go on!";
            string packetId_sent1 = from->sendMessage(to->getAppAccount(), msg3);
            usleep(300000);
            while (message = toMessageHandler->pollMessage())
            {
                
            }

            string msg4 = "OK!";
            string packetId_sent2 = to->sendMessage(from->getAppAccount(), msg4);
            usleep(300000);
            while (message = fromMessageHandler->pollMessage())
            {
                
            }
        }

        from->logout();
        usleep(400000);

        to->logout();
        usleep(400000);

        delete from;
        from = NULL;
        delete to;
        to = NULL;
    }

};

int main(int argc, char **argv) {
    /* A用户发送单条消息给B用户 */
//    MimcDemo::testP2PSendOneMessage();
    /* A用户与B用户互发多条消息 */
    MimcDemo::testP2PSendMessages();
    return 0;
}