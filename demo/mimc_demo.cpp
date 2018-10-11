#include <mimc/user.h>
#include <mimc/threadsafe_queue.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <list>

using namespace std;

#ifndef STAGING
/*string appId = "2882303761517613988";
string appKey = "5361761377988";
string appSecret = "2SZbrJOAL1xHRKb7L9AiRQ==";*/
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
        LoggerWrapper::instance()->info("In statusChange, status is %d, errType is %s, errReason is %s, errDescription is %s", status, errType.c_str(), errReason.c_str(), errDescription.c_str());
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
    LaunchedResponse onLaunched(std::string fromAccount, std::string fromResource, long chatId, const std::string& appContent) {
        LoggerWrapper::instance()->info("In onLaunched, chatId is %ld, fromAccount is %s, fromResource is %s, appContent is %s", chatId, fromAccount.c_str(), fromResource.c_str(), appContent.c_str());
        if (appContent != this->appContent) {
            return LaunchedResponse(false, LAUNCH_ERR_ILLEGALSIG);
        }
        LoggerWrapper::instance()->info("In onLaunched, appContent is equal to this->appContent");
        chatIds.push_back(chatId);
        return LaunchedResponse(true, LAUNCH_OK);
    }

    void onAnswered(long chatId, bool accepted, const std::string& errmsg) {
        LoggerWrapper::instance()->info("In onAnswered, chatId is %ld, accepted is %d, errmsg is %s", chatId, accepted, errmsg.c_str());
        if (accepted) {
            chatIds.push_back(chatId);
        }
    }

    void onClosed(long chatId, const std::string& errmsg) {
        LoggerWrapper::instance()->info("In onClosed, chatId is %ld, errmsg is %s", chatId, errmsg.c_str());
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

    void handleData(long chatId, const std::string& data, RtsDataType dataType, RtsChannelType channelType) {
        LoggerWrapper::instance()->info("In handleData, chatId is %ld, data is %s, dataType is %d", chatId, data.c_str(), dataType);
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
        
        LoggerWrapper::instance()->info("In fetchToken, body is %s", body.c_str());
        LoggerWrapper::instance()->info("In fetchToken, url is %s", url.c_str());

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
                
            }
        }

        curl_easy_cleanup(curl);

        return result;
    }

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        string *bodyp = (string *)userp;
        bodyp->append((const char *)contents, size * nmemb);
        
        
        
        return bodyp->size();
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
        User* from = new User(appAccount1);
        User* to = new User(appAccount2);
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
        User* from = new User(appAccount1);
        User* to = new User(appAccount2);
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

    static void testP2PDialCall() {
        User* from = new User(appAccount1);
        User* to1 = new User(appAccount2);
        User* to2 = new User(appAccount3);
        TestMessageHandler* fromMessageHandler = new TestMessageHandler();
        TestMessageHandler* to1MessageHandler = new TestMessageHandler();
        TestMessageHandler* to2MessageHandler = new TestMessageHandler();
        TestRTSCallEventHandler* fromRTSCallEventHandler = new TestRTSCallEventHandler("ll123456");
        TestRTSCallEventHandler* to1RTSCallEventHandler = new TestRTSCallEventHandler("ll123456");
        TestRTSCallEventHandler* to2RTSCallEventHandler = new TestRTSCallEventHandler("ll123456");

        from->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount1));
        from->registerOnlineStatusHandler(new TestOnlineStatusHandler());
        from->registerMessageHandler(fromMessageHandler);
        from->registerRTSCallEventHandler(fromRTSCallEventHandler);

        to1->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount2));
        to1->registerOnlineStatusHandler(new TestOnlineStatusHandler());
        to1->registerMessageHandler(to1MessageHandler);
        to1->registerRTSCallEventHandler(to1RTSCallEventHandler);

        to2->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount3));
        to2->registerOnlineStatusHandler(new TestOnlineStatusHandler());
        to2->registerMessageHandler(to2MessageHandler);
        to2->registerRTSCallEventHandler(to2RTSCallEventHandler);

        if (!from->login()) {
            LoggerWrapper::instance()->error("from login failed");
            return;
        }
        /*usleep(500000);
        from->dialCall("rts_demo_account1", "", mimc::A_STREAM, fromRTSCallEventHandler->getAppContent());
        sleep(2);

        std::list<long>& chatIds = fromRTSCallEventHandler->getChatIds();
        if (!chatIds.empty()) {
            std::list<long>::iterator iter;
            for (iter = chatIds.begin(); iter != chatIds.end(); iter++) {
                long chatId = *iter;
                std::cout << "from chatId is " << chatId << std::endl;
                std::string data = "焦奕天给马利静发消息";
                for(int i = 0; i < 100; i++) {
                    from->sendRtsData(chatId, data, mimc::USER_DATA_AUDIO);
                    sleep(1);
                }
            }
        }*/
        if (!to1->login()) {
            LoggerWrapper::instance()->error("to1 login failed");
            return;
        }

        if (!to2->login()) {
            LoggerWrapper::instance()->error("to2 login failed");
            return;
        }

        usleep(500000);

        string msg1 = "WITH MIMC,WE CAN FLY HIGHER！";
        string packetId_sent1 = from->sendMessage(to1->getAppAccount(), msg1);
        

        usleep(300000);

        MIMCMessage *message1 = to1MessageHandler->pollMessage();
        if (message1 != NULL) {
            
        }

        long chatId1 = from->dialCall(to1->getAppAccount(), "", fromRTSCallEventHandler->getAppContent());
        if (chatId1 == -1) {
            LoggerWrapper::instance()->error("from dialCall to to1 failed");
            return;
        }
        long chatId2 = from->dialCall(to2->getAppAccount(), "", fromRTSCallEventHandler->getAppContent());
        if (chatId2 == -1) {
            LoggerWrapper::instance()->error("from dialCall to to2 failed");
            return;
        }
        sleep(5);
        std::list<long>& chatIds = fromRTSCallEventHandler->getChatIds();
        if (!chatIds.empty()) {
            std::list<long>::iterator iter;
            for (iter = chatIds.begin(); iter != chatIds.end(); iter++) {
                long chatId = *iter;
                LoggerWrapper::instance()->info("from chatId is %ld", chatId);
                std::string data = "121211212121323231";
                from->sendRtsData(chatId, data, AUDIO);
            }
        }
        sleep(1);
        std::list<long>& to1_chatIds = to1RTSCallEventHandler->getChatIds();
        if (!to1_chatIds.empty()) {
            std::list<long>::iterator iter;
            for (iter = to1_chatIds.begin(); iter != to1_chatIds.end(); iter++) {
                long chatId = *iter;
                LoggerWrapper::instance()->info("to1 chatId is %ld", chatId);
                std::string data = "23233323233434423";
                to1->sendRtsData(chatId, data, AUDIO);
            }
        }
        sleep(1);
        std::list<long>& to2_chatIds = to2RTSCallEventHandler->getChatIds();
        if (!to2_chatIds.empty()) {
            std::list<long>::iterator iter;
            for (iter = to2_chatIds.begin(); iter != to2_chatIds.end(); iter++) {
                long chatId = *iter;
                LoggerWrapper::instance()->info("to2 chatId is %ld", chatId);
                std::string data = "daskldjlkjlkjflksjfl";
                to2->sendRtsData(chatId, data, AUDIO);
            }
        }
        sleep(1);
        if (!chatIds.empty()) {
            std::list<long>::iterator iter;
            for (iter = chatIds.begin(); iter != chatIds.end(); ) {
                long chatId = *iter++;
                LoggerWrapper::instance()->info("from chatId to close is %ld", chatId);
                std::string byeReason = "chat over";
                from->closeCall(chatId, byeReason);
            }
        }
        sleep(2);

        from->logout();
        to1->logout();
        to2->logout();

        usleep(500000);

        if (!from->login()) {
            LoggerWrapper::instance()->error("from second login failed");
            return;
        }

        if (!to1->login()) {
            LoggerWrapper::instance()->error("to1 second login failed");
            return;
        }

        sleep(1);

        chatId1 = from->dialCall(to1->getAppAccount(), "", fromRTSCallEventHandler->getAppContent());
        if (chatId1 == -1) {
            LoggerWrapper::instance()->error("from dialCall to to1 failed");
            return;
        }
        sleep(5);

        chatIds = fromRTSCallEventHandler->getChatIds();
        if (!chatIds.empty()) {
            std::list<long>::iterator iter;
            for (iter = chatIds.begin(); iter != chatIds.end(); iter++) {
                long chatId = *iter;
                LoggerWrapper::instance()->info("from chatId is %ld", chatId);
                std::string data = "12345678901234567890";
                from->sendRtsData(chatId, data, AUDIO);
            }
        }
        sleep(1);
        to1_chatIds = to1RTSCallEventHandler->getChatIds();
        if (!to1_chatIds.empty()) {
            std::list<long>::iterator iter;
            for (iter = to1_chatIds.begin(); iter != to1_chatIds.end(); iter++) {
                long chatId = *iter;
                LoggerWrapper::instance()->info("to1 chatId is %ld", chatId);
                std::string data = "09876543210987654321";
                to1->sendRtsData(chatId, data, AUDIO);
            }
        }
        sleep(1);
        if (!chatIds.empty()) {
            std::list<long>::iterator iter;
            for (iter = chatIds.begin(); iter != chatIds.end(); ) {
                long chatId = *iter++;
                LoggerWrapper::instance()->info("from chatId to close is %ld", chatId);
                std::string byeReason = "chat over";
                from->closeCall(chatId, byeReason);
            }
        } else {
            LoggerWrapper::instance()->info("chatIds are empty");
        }
        sleep(1);

        from->logout();
        to1->logout();

        sleep(2);

        delete from;
        delete to1;
        delete to2;
    }  
};

int main(int argc, char **argv) {
    /* A用户发送单条消息给B用户 */
//    MimcDemo::testP2PSendOneMessage();
    /* A用户与B用户互发多条消息 */
//    MimcDemo::testP2PSendMessages();
    MimcDemo::testP2PDialCall();
    return 0;
}