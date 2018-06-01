#include <gtest/gtest.h>
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
string appAccount1 = "mi153";
string appAccount2 = "mi108";

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

class MIMCTest: public testing::Test
{
protected:
    void SetUp() {
        from = new User(appAccount1);
        to = new User(appAccount2);
        fromMessageHandler = new TestMessageHandler();
        toMessageHandler = new TestMessageHandler();

        from->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount1));
        from->registerOnlineStatusHandler(new TestOnlineStatusHandler());
        from->registerMessageHandler(fromMessageHandler);

        to->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount2));
        to->registerOnlineStatusHandler(new TestOnlineStatusHandler());
        to->registerMessageHandler(toMessageHandler);
    }

    void TearDown() {
        delete from;
        from = NULL;
        delete to;
        to = NULL;
    }

    void testP2PSendOneMessage(User *from, TestMessageHandler *fromMessageHandler, User *to, TestMessageHandler *toMessageHandler) {
        from->login();
        usleep(500000);
        ASSERT_EQ(Online, from->getOnlineStatus());

        to->login();
        usleep(500000);
        ASSERT_EQ(Online, to->getOnlineStatus());

        string msg1 = "你猜我是谁哈哈哈";
        string packetId = from->sendMessage(to->getAppAccount(), msg1);
        usleep(100000);
        ASSERT_EQ(packetId, fromMessageHandler->pollServerAck());
        usleep(200000);

        MIMCMessage *message = toMessageHandler->pollMessage();
        ASSERT_TRUE(message != NULL);
        ASSERT_EQ(packetId, message->getPacketId());
        ASSERT_TRUE(message->getSequence() > 0);
        ASSERT_EQ(from->getAppAccount(), message->getFromAccount());
        LOG4CPLUS_INFO(LOGGER, "messageContent is " << message->getPayload());
        ASSERT_EQ(msg1, message->getPayload());
        ASSERT_TRUE(toMessageHandler->pollMessage() == NULL);

        sleep(20);
        ASSERT_TRUE(from->logout());
        usleep(400000);
        ASSERT_EQ(Offline, from->getOnlineStatus());

        ASSERT_TRUE(to->logout());
        usleep(400000);
        ASSERT_EQ(Offline, to->getOnlineStatus());
    }

protected:
    User *from;
    User *to;
    TestMessageHandler * fromMessageHandler;
    TestMessageHandler * toMessageHandler;
};

TEST_F(MIMCTest, testP2PSendOneMsg) {
    testP2PSendOneMessage(from, fromMessageHandler, to, toMessageHandler);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}