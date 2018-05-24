#include <mimc/user.h>
#include <mimc/threadsafe_queue.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <curl/curl.h>

using namespace std;

string appId = "2882303761517613988";
string appKey = "5361761377988";
string appSecret = "2SZbrJOAL1xHRKb7L9AiRQ==";
string appAccount1 = "LeiJun";
string appAccount2 = "LinBin";

class TestOnlineStatusHandler : public OnlineStatusHandler {
public:
    void statusChange(OnlineStatus status, string errType, string errReason, string errDescription) {
        LOG4CPLUS_INFO(LOGGER, "status is " << status << ", errType is " << errType << ", errReason is " << errReason << ", errDescription is " << errDescription);
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
        packetIds.push(packetId);
    }

    void handleSendMsgTimeout(MIMCMessage message) {
        LOG4CPLUS_ERROR(LOGGER, "message send timeout! packetId is " << message.getPacketId() << ", sequence is " << message.getSequence() << ", timestamp is " << message.getTimeStamp());
    }

    MIMCMessage* pollMessage() {
        MIMCMessage *messagePtr;
        messages.pop(&messagePtr);
        return messagePtr;
    }

    std::string pollServerAck() {
        std::string *packetIdPtr;
        packetIds.pop(&packetIdPtr);
        if (packetIdPtr == NULL) {
            return "";
        }
        return *packetIdPtr;
    }
private:
    ThreadSafeQueue<MIMCMessage> messages;
    ThreadSafeQueue<std::string> packetIds;
};

class TestTokenFetcher : public MIMCTokenFetcher {
public:
    string fetchToken() {
        curl_global_init(CURL_GLOBAL_ALL);
        CURL *curl = curl_easy_init();
        CURLcode res;
        const string url = "https://mimc.chat.xiaomi.net/api/account/token";
        const string body = "{\"appId\":\"" + this->appId + "\",\"appKey\":\"" + this->appKey + "\",\"appSecret\":\"" + this->appSecret + "\",\"appAccount\":\"" + this->appAccount + "\"}";
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
                LOG4CPLUS_ERROR(LOGGER, "curl_easy_perform() failed: " + string(curl_easy_strerror(res)));
            }
        }

        curl_easy_cleanup(curl);

        return result;
    }

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        string *bodyp = (string *)userp;
        bodyp->append((const char *)contents, size * nmemb);
        LOG4CPLUS_INFO(LOGGER, "WriteCallback size is " << size);
        LOG4CPLUS_INFO(LOGGER, "WriteCallback nmemb is " << nmemb);
        LOG4CPLUS_INFO(LOGGER, *bodyp);
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
        User* from = new User();
        User* to = new User();
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

        string msg1 = "WITH MIMC,WE CAN FLY HIGHER！";
        LOG4CPLUS_INFO(LOGGER, "message1 from " << appAccount1 << " is " << msg1);
        
        string packetId_sent1 = from->sendMessage(to->getAppAccount(), msg1);
        LOG4CPLUS_INFO(LOGGER, "packetId from " << appAccount1 << " is " << packetId_sent1);

        usleep(100000);

        string packetId_ack1 = fromMessageHandler->pollServerAck();
        LOG4CPLUS_INFO(LOGGER, "packetId_ack from " << appAccount1 << " is " << packetId_ack1);

        usleep(200000);

        MIMCMessage *message1 = toMessageHandler->pollMessage();
        if (message1 != NULL) {
            LOG4CPLUS_INFO(LOGGER, "message to "<< appAccount2 << " is " << message1->getPayload());
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
    MimcDemo::testP2PSendOneMessage();
    return 0;
}