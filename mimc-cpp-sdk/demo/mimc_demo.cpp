#include <mimc/user.h>
#include <mimc/threadsafe_queue.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

string appId = "2882303761517613988";
string appKey = "5361761377988";
string appSecret = "2SZbrJOAL1xHRKb7L9AiRQ==";
string appAccount1 = "雷军";
string appAccount2 = "林斌";

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
        string result;

        if (this->appAccount == "林斌") {
            result = "{\"code\":200,\"message\":\"success\",\"data\":{\"appId\":\"2882303761517613988\",\"appPackage\":\"com.xiaomi.imc.test_app1\",\"appAccount\":\"林斌\",\"miChid\":9,\"miUserId\":\"10893549466025984\",\"miUserSecurityKey\":\"pkB4XDyu0WOmXQwDaWVRsQ==\",\"token\":\"bJRLeg7AgtSh0T13YjL/IDo1+cyozAK8oOvK1HLW8czyexdNLqpo2KtpTNLJqHRGDvILZ49lVpCt/RYO9mmygSsy24J5LYYL/VJt6t8iTa2ci8S77g1gID3bLgzkh5/HZ2lFtn9IGxOtZTdYuzlEzGCz8T6aNPdqe5J0/0UFGRqY1vZTJPO9oNOCm8WVERgNr3IddjZj+iubzezuQrCOBXInFdXG1ttM3ULb6oLPCyH6bNNY89n2dlb5Mwp2XJdxmHQ0osawIXf/fkI+Kkblz4wch9vdJg4R/w3oMUVIoqHXuE4jcTkf3lHQGbWACkXBXckM+TBWOn6B4fK0wT9oUA==\"}}";
        } else if (this->appAccount == "雷军") {
            result = "{\"code\":200,\"message\":\"success\",\"data\":{\"appId\":\"2882303761517613988\",\"appPackage\":\"com.xiaomi.imc.test_app1\",\"appAccount\":\"雷军\",\"miChid\":9,\"miUserId\":\"10893549197590528\",\"miUserSecurityKey\":\"ZLyXbJmh0Y6py8VPzJJiVw==\",\"token\":\"bJRLeg7AgtSh0T13YjL/IDo1+cyozAK8oOvK1HLW8cxK8ptUmNf4qlqbRx/8T6mBWoAoTJ8EE5dilN0Uvi1/TcC2voTpdRdOLfz7xEctJANpfIytTZ75lXYUPAYFk7ClpeE+AFKlKyn6xIAAIEVn4v3pR+7yhH9UCEc9ilB1neORg4pTvQOVlRoswAqF8Ag9unVVvBMYGQNEigt5hbpglirTillubQ8PmhwawG2ezdmkGPi0+60Xh31p0Crp0yEByjLO8ZydGlhUJ45bVdGYCkWPM93hjPZdDWzObvNOGUbDf9hvruHdVCJsmlqzA4i8B6Wlt+IxaeRB53YfvKsXxg==\"}}";
        }
        
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

        string msg1 = "你猜我是谁哈哈哈！";
        LOG4CPLUS_INFO(LOGGER, "message1 from " << appAccount1 << " is " << msg1);
        
        string packetId_sent1 = from->sendMessage(to->getAppAccount(), msg1);
        LOG4CPLUS_INFO(LOGGER, "packetId from " << appAccount1 << " is " << packetId_sent1);

        usleep(100000);

        string packetId_ack1 = fromMessageHandler->pollServerAck();
        LOG4CPLUS_INFO(LOGGER, "packetId_ack from " << appAccount1 << " is " << packetId_ack1);

        usleep(200000);

        MIMCMessage *message1 = toMessageHandler->pollMessage();
        LOG4CPLUS_INFO(LOGGER, "message to "<< appAccount2 << " is " << message1->getPayload());

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