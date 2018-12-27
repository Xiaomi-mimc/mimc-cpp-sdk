#include <gtest/gtest.h>
#include <mimc/user.h>
#include <test/mimc_message_handler.h>
#include <test/mimc_onlinestatus_handler.h>
#include <test/mimc_tokenfetcher.h>

using namespace std;

#ifndef STAGING
string appId = "2882303761517613988";
string appKey = "5361761377988";
string appSecret = "2SZbrJOAL1xHRKb7L9AiRQ==";
#else
string appId = "2882303761517479657";
string appKey = "5221747911657";
string appSecret = "PtfBeZyC+H8SIM/UXhZx1w==";
#endif

string appAccount1 = "mi153";
string appAccount2 = "mi108";

class MIMCTest: public testing::Test
{
protected:
    void SetUp() {
        from = new User(atoll(appId.c_str()), appAccount1);
        to = new User(atoll(appId.c_str()), appAccount2);
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

    void testP2PSendOneMessage() {
        from->login();
        sleep(2);
        ASSERT_EQ(Online, from->getOnlineStatus());

        to->login();
        sleep(2);
        ASSERT_EQ(Online, to->getOnlineStatus());

        string payload1 = "你猜我是谁哈哈哈";
        string packetId = from->sendMessage(to->getAppAccount(), payload1);
        usleep(200000);
        string* recvPacketIdPtr = fromMessageHandler->pollServerAck();
        ASSERT_TRUE(recvPacketIdPtr != NULL);
        ASSERT_EQ(packetId, *recvPacketIdPtr);
        usleep(400000);

        MIMCMessage *message = toMessageHandler->pollMessage();
        ASSERT_TRUE(message != NULL);
        ASSERT_TRUE(message->getSequence() > 0);
        ASSERT_EQ(from->getAppAccount(), message->getFromAccount());
        ASSERT_EQ(payload1, message->getPayload());

        ASSERT_TRUE(toMessageHandler->pollMessage() == NULL);

        sleep(10);

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
    testP2PSendOneMessage();
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
