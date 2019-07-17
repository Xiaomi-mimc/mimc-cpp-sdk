#include <gtest/gtest.h>
#include <mimc/user.h>
#include <mimc/utils.h>
#include <mimc/error.h>
#include <test/mimc_message_handler.h>
#include <test/mimc_onlinestatus_handler.h>
#include <test/mimc_tokenfetcher.h>
#include <test/rts_call_eventhandler.h>
#include <test/rts_call_delayresponse_eventhandler.h>
#include <test/rts_call_timeoutresponse_eventhandler.h>
#include <test/rts_message_data.h>
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


string appAccount1 = "MI153";
string appAccount2 = "MI108";
string appAccount3 = "MI110";
string appAccount4 = "MI111";
string appAccount5 = "MI112";


const int WAIT_TIME_FOR_MESSAGE = 1;
const int UDP_CONN_TIMEOUT = 5;

class RtsTest: public testing::Test
{
protected:
    void SetUp() {
        curl_global_init(CURL_GLOBAL_ALL);
        rtsUser1_r1 = new User(atoll(appId.c_str()), appAccount1, "rts_staging_resource1");
        rtsUser1_r2 = new User(atoll(appId.c_str()), appAccount1, "rts_staging_resource2");
        rtsUser2_r1 = new User(atoll(appId.c_str()), appAccount2, "rts_staging_resource1");
        rtsUser2_r2 = new User(atoll(appId.c_str()), appAccount2, "rts_staging_resource2");
        rtsUser3 = new User(atoll(appId.c_str()), appAccount3, "rts_staging_resource1");

        tokenFetcher1_r1 = new TestTokenFetcher(appId, appKey, appSecret, appAccount1);
        tokenFetcher1_r2 = new TestTokenFetcher(appId, appKey, appSecret, appAccount1);
        tokenFetcher2_r1 = new TestTokenFetcher(appId, appKey, appSecret, appAccount2);
        tokenFetcher2_r2 = new TestTokenFetcher(appId, appKey, appSecret, appAccount2);
        tokenFetcher3 = new TestTokenFetcher(appId, appKey, appSecret, appAccount3);

        onlineStatusHandler1_r1 = new TestOnlineStatusHandler();
        onlineStatusHandler1_r2 = new TestOnlineStatusHandler();
        onlineStatusHandler2_r1 = new TestOnlineStatusHandler();
        onlineStatusHandler2_r2 = new TestOnlineStatusHandler();
        onlineStatusHandler3 = new TestOnlineStatusHandler();

        msgHandler1_r1 = new TestMessageHandler();
        msgHandler1_r2 = new TestMessageHandler();
        msgHandler2_r1 = new TestMessageHandler();
        msgHandler2_r2 = new TestMessageHandler();
        msgHandler3 = new TestMessageHandler();

        callEventHandler1_r1 = new TestRTSCallEventHandler();
        callEventHandler1_r2 = new TestRTSCallEventHandler();
        callEventHandler2_r1 = new TestRTSCallEventHandler("ll123456");
        callEventHandler2_r2 = new TestRTSCallEventHandler("ll123456");
        callEventHandler3 = new TestRTSCallEventHandler();
        callEventHandlerDelayResponse = new TestRTSCallDelayResponseEventHandler();
        callEventHandlerTimeoutResponse = new TestRTSCallTimeoutResponseEventHandler();

        rtsUser1_r1->registerTokenFetcher(tokenFetcher1_r1);
        rtsUser1_r1->registerOnlineStatusHandler(onlineStatusHandler1_r1);
        rtsUser1_r1->registerMessageHandler(msgHandler1_r1);
        rtsUser1_r1->registerRTSCallEventHandler(callEventHandler1_r1);

        rtsUser1_r2->registerTokenFetcher(tokenFetcher1_r2);
        rtsUser1_r2->registerOnlineStatusHandler(onlineStatusHandler1_r2);
        rtsUser1_r2->registerMessageHandler(msgHandler1_r2);
        rtsUser1_r2->registerRTSCallEventHandler(callEventHandler1_r2);

        rtsUser2_r1->registerTokenFetcher(tokenFetcher2_r1);
        rtsUser2_r1->registerOnlineStatusHandler(onlineStatusHandler2_r1);
        rtsUser2_r1->registerMessageHandler(msgHandler2_r1);
        rtsUser2_r1->registerRTSCallEventHandler(callEventHandler2_r1);

        rtsUser2_r2->registerTokenFetcher(tokenFetcher2_r2);
        rtsUser2_r2->registerOnlineStatusHandler(onlineStatusHandler2_r2);
        rtsUser2_r2->registerMessageHandler(msgHandler2_r2);
        rtsUser2_r2->registerRTSCallEventHandler(callEventHandler2_r2);

        rtsUser3->registerTokenFetcher(tokenFetcher3);
        rtsUser3->registerOnlineStatusHandler(onlineStatusHandler3);
        rtsUser3->registerMessageHandler(msgHandler3);
        rtsUser3->registerRTSCallEventHandler(callEventHandler3);
    }

    void TearDown() {
        rtsUser1_r1->logout();
        rtsUser1_r2->logout();
        rtsUser2_r1->logout();
        rtsUser2_r2->logout();
        rtsUser3->logout();
        //sleep(2);
		std::this_thread::sleep_for(std::chrono::seconds(2));

        delete rtsUser1_r1;
        rtsUser1_r1 = NULL;
        delete rtsUser1_r2;
        rtsUser1_r2 = NULL;
        delete rtsUser2_r1;
        rtsUser2_r1 = NULL;
        delete rtsUser2_r2;
        rtsUser2_r2 = NULL;
        delete rtsUser3;
        rtsUser3 = NULL;

        delete tokenFetcher1_r1;
        tokenFetcher1_r1 = NULL;
        delete tokenFetcher1_r2;
        tokenFetcher1_r2 = NULL;
        delete tokenFetcher2_r1;
        tokenFetcher2_r1 = NULL;
        delete tokenFetcher2_r2;
        tokenFetcher2_r2 = NULL;
        delete tokenFetcher3;
        tokenFetcher3 = NULL;

        delete onlineStatusHandler1_r1;
        onlineStatusHandler1_r1 = NULL;
        delete onlineStatusHandler1_r2;
        onlineStatusHandler1_r2 = NULL;
        delete onlineStatusHandler2_r1;
        onlineStatusHandler2_r1 = NULL;
        delete onlineStatusHandler2_r2;
        onlineStatusHandler2_r2 = NULL;
        delete onlineStatusHandler3;
        onlineStatusHandler3 = NULL;

        delete msgHandler1_r1;
        msgHandler1_r1 = NULL;
        delete msgHandler1_r2;
        msgHandler1_r2 = NULL;
        delete msgHandler2_r1;
        msgHandler2_r1 = NULL;
        delete msgHandler2_r2;
        msgHandler2_r2 = NULL;
        delete msgHandler3;
        msgHandler3 = NULL;

        delete callEventHandler1_r1;
        callEventHandler1_r1 = NULL;
        delete callEventHandler1_r2;
        callEventHandler1_r2 = NULL;
        delete callEventHandler2_r1;
        callEventHandler2_r1 = NULL;
        delete callEventHandler2_r2;
        callEventHandler2_r2 = NULL;
        delete callEventHandler3;
        callEventHandler3 = NULL;
        delete callEventHandlerDelayResponse;
        callEventHandlerDelayResponse = NULL;
        delete callEventHandlerTimeoutResponse;
        callEventHandlerTimeoutResponse = NULL;
        curl_global_cleanup();
    }

    //@test
    void testCreateCall() {
        testCreateCall(rtsUser1_r1, callEventHandler1_r1, rtsUser3, callEventHandler3);
    }

    //@test
    void testCreateCallTimeout() {
        rtsUser2_r1->registerRTSCallEventHandler(callEventHandlerTimeoutResponse);
        createCallTimeout(rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandlerTimeoutResponse);
    }

    //@test
    void testP2PSendOneRtsSignalToOffline() {
        testP2PSendOneRtsSignalToOffline(rtsUser1_r1, callEventHandler1_r1, rtsUser3, callEventHandler3, callEventHandler1_r1->getAppContent());
    }

    //@test
    void testP2PSendOneRtsSignalFromOffline() {
        testP2PSendOneRtsSignalFromOffline(rtsUser1_r1, callEventHandler1_r1, rtsUser3, callEventHandler3, callEventHandler1_r1->getAppContent());
    }

    //@test
    void testCreateCallRefuse() {
        testCreateCallRefuse(rtsUser1_r1, callEventHandler1_r1, rtsUser2_r2, callEventHandler2_r2, callEventHandler1_r1->getAppContent());
    }

    //@test
    void testCreateCallWithSelf() {
        testCreateCallWithSelf(rtsUser1_r1, callEventHandler1_r1, rtsUser1_r2, callEventHandler1_r2);
    }

    //@test
    void testCreateCallFromMulResources() {
        testCreateCallFromMulResources(rtsUser1_r1, callEventHandler1_r1, rtsUser1_r2, callEventHandler1_r2, rtsUser2_r1, callEventHandler2_r1);
    }

    //@test
    void testCreateCallWithMulResources() {
        testCreateCallWithMulResources(rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1, rtsUser2_r2, callEventHandlerDelayResponse);
    }

    //@test
    void testCreateCallWhenCalling() {
        testCreateCallWhenCalling(rtsUser1_r1, callEventHandler1_r1, rtsUser1_r2, callEventHandler1_r2, rtsUser2_r1, callEventHandler2_r1, rtsUser2_r2, callEventHandler2_r2);
    }

    //@test
    void testRecvCallWhenCalling() {
        testRecvCallWhenCalling(rtsUser1_r1, callEventHandler1_r1, rtsUser1_r2, callEventHandler1_r2, rtsUser2_r1, callEventHandler2_r1, rtsUser2_r2, callEventHandler2_r2);
    }

    //@test
    void testCreateCalls() {
        testCreateCalls(rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1, rtsUser3, callEventHandler3);
    }

    //@test
    void testCreateCallWithOffline() {
        testCreateCallWithOffline(rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1);
    }

    //@test
    void testCloseCall() {
        logIn(rtsUser1_r1, callEventHandler1_r1);
        logIn(rtsUser2_r1, callEventHandler2_r1);

        uint64_t callId = 0;
        createCall(callId, rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1, "ll123456");
        testCloseCall(callId, rtsUser1_r1, callEventHandler1_r1, callEventHandler2_r1);
    }

    //@test
    void testCloseCallSameTime() {
        logIn(rtsUser1_r1, callEventHandler1_r1);
        logIn(rtsUser2_r1, callEventHandler2_r1);

        uint64_t callId = 0; 
        createCall(callId, rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1, "ll123456");
        closeCallSameTime(callId, rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1);
    }

    //@test
    void testSendDatas() {
        logIn(rtsUser1_r1, callEventHandler1_r1);
        logIn(rtsUser2_r1, callEventHandler2_r1);

        uint64_t callId = 0;
        createCall(callId, rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1, "ll123456");

        sendDatas(callId, rtsUser1_r1, callEventHandler2_r1, RELAY);

        closeCall(callId, rtsUser1_r1, callEventHandler1_r1, callEventHandler2_r1);
    }

    //@test
    void testSendDataToEachOther() {
        logIn(rtsUser1_r1, callEventHandler1_r1);
        logIn(rtsUser2_r1, callEventHandler2_r1);

        uint64_t callId = 0;
        createCall(callId, rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1, "ll123456");

        sendDataToEachOther(callId, rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1, RELAY);

        closeCall(callId, rtsUser1_r1, callEventHandler1_r1, callEventHandler2_r1);
    }

    //@test
    void testSendDataAfterClose() {
        sendDataAfterClose(rtsUser1_r1, callEventHandler1_r1, rtsUser2_r1, callEventHandler2_r1, "ll123456");
    }

    void logIn(User* user, TestRTSCallEventHandler* callEventHandler) {
        time_t loginTs = time(NULL);
        user->login();
        while (time(NULL) - loginTs < LOGIN_TIMEOUT && user->getOnlineStatus() == Offline) {
            //usleep(100000);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        ASSERT_EQ(Online, user->getOnlineStatus());
        if (callEventHandler != NULL) {
            XMDLoggerWrapper::instance()->info("CLEAR CALLEVENTHANDLER OF UUID:%lld", user->getUuid());
            callEventHandler->clear();
        }
    }

    void testCreateCall(User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo) {
        string appContent = "appinfo";
        logIn(from, callEventHandlerFrom);
        logIn(to, callEventHandlerTo);
        //A_STREAM

        uint64_t callId1 = from->dialCall(to->getAppAccount(), appContent);
        ASSERT_NE(callId1, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_TRUE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId1, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ(appContent, inviteRequest.getAppContent());

        RtsMessageData createResponse;
        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId1, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse.getDesc());

        closeCall(callId1, from, callEventHandlerFrom, callEventHandlerTo);
        //V_STREAM

        uint64_t callId2 = from->dialCall(to->getAppAccount());
        ASSERT_NE(callId2, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        ASSERT_TRUE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId2, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("", inviteRequest.getAppContent());

        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId2, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse.getDesc());

        closeCall(callId2, from, callEventHandlerFrom, callEventHandlerTo);
        //AV_STREAM

        uint64_t callId3 = from->dialCall(to->getAppAccount());
        ASSERT_NE(callId3, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        ASSERT_TRUE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId3, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("", inviteRequest.getAppContent());

        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId3, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse.getDesc());

        closeCall(callId3, from, callEventHandlerFrom, callEventHandlerTo);
    }

    void createCall(uint64_t& callId, User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo, const string& appContent) {
	    callId = from->dialCall(to->getAppAccount(), appContent);
        ASSERT_NE(callId, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_TRUE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ(appContent, inviteRequest.getAppContent());

        RtsMessageData createResponse;
        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse.getDesc());
    }

    void closeCall(uint64_t callId, User* from, TestRTSCallEventHandler* callEventHandlerFrom, TestRTSCallEventHandler* callEventHandlerTo) {
        from->closeCall(callId);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData byeRequest;
        ASSERT_TRUE(callEventHandlerTo->pollBye(WAIT_TIME_FOR_MESSAGE, byeRequest));
        ASSERT_EQ(callId, byeRequest.getCallId());
        ASSERT_EQ("", byeRequest.getDesc());

        RtsMessageData byeResponse;
        ASSERT_TRUE(callEventHandlerFrom->pollBye(WAIT_TIME_FOR_MESSAGE, byeResponse));
        ASSERT_EQ(callId, byeResponse.getCallId());
        ASSERT_EQ("CLOSED_INITIATIVELY", byeResponse.getDesc());
    }

    void sendDatas(uint64_t callId, User* from, TestRTSCallEventHandler* callEventHandlerTo, RtsChannelType channel_type) {
        vector<string> sendDatas;
        for (int size = 0; size <= 500; size += 50) {
            string sendData = Utils::generateRandomString(size * 1024);
            ASSERT_NE(-1, from->sendRtsData(callId, sendData, AUDIO, channel_type));
            sendDatas.push_back(sendData);
            //sleep(size / 200);
			std::this_thread::sleep_for(std::chrono::seconds(size / 200));
        }

        for (int i = 0; i < sendDatas.size(); i++) {
            XMDLoggerWrapper::instance()->info("current i is %d", i);
            RtsMessageData recvData;
            ASSERT_TRUE(callEventHandlerTo->pollData(WAIT_TIME_FOR_MESSAGE, recvData));
            ASSERT_EQ(callId, recvData.getCallId());
            ASSERT_EQ(AUDIO, recvData.getDataType());
            ASSERT_EQ(channel_type, recvData.getChannelType());
            const string& data = recvData.getRecvData();
            int index = data.size() / (50 * 1024);
            XMDLoggerWrapper::instance()->info("recv data index is %d", index);
            ASSERT_EQ(sendDatas.at(index), data);
        }

        string sendData0 = Utils::generateRandomString(RTS_MAX_PAYLOAD_SIZE + 1);
        ASSERT_EQ(-1, from->sendRtsData(callId, sendData0, AUDIO, channel_type));
    }

    void sendDataToEachOther(uint64_t callId, User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo, RtsChannelType channel_type) {
        string sendData1 = "hello to";
        string sendData2 = "hello from";

        ASSERT_NE(-1, from->sendRtsData(callId, sendData1, AUDIO, channel_type));
        ASSERT_NE(-1, to->sendRtsData(callId, sendData2, VIDEO, channel_type));

        RtsMessageData recvData;
        ASSERT_TRUE(callEventHandlerTo->pollData(WAIT_TIME_FOR_MESSAGE, recvData));
        ASSERT_EQ(callId, recvData.getCallId());
        ASSERT_EQ(AUDIO, recvData.getDataType());
        ASSERT_EQ(channel_type, recvData.getChannelType());
        ASSERT_EQ(sendData1, recvData.getRecvData());
        ASSERT_FALSE(callEventHandlerTo->pollData(WAIT_TIME_FOR_MESSAGE, recvData));

        ASSERT_TRUE(callEventHandlerFrom->pollData(WAIT_TIME_FOR_MESSAGE, recvData));
        ASSERT_EQ(callId, recvData.getCallId());
        ASSERT_EQ(VIDEO, recvData.getDataType());
        ASSERT_EQ(channel_type, recvData.getChannelType());
        ASSERT_EQ(sendData2, recvData.getRecvData());
        ASSERT_FALSE(callEventHandlerFrom->pollData(WAIT_TIME_FOR_MESSAGE, recvData));
    }

    void sendDataAfterClose(User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo, const string& appContent) {
        logIn(from, callEventHandlerFrom);
        logIn(to, callEventHandlerTo);

        string sendData = Utils::generateRandomString(50);

        uint64_t callId = 0;
        createCall(callId, from, callEventHandlerFrom, to, callEventHandlerTo, appContent);
        from->closeCall(callId);
        ASSERT_EQ(-1, from->sendRtsData(callId, sendData, AUDIO));
        RtsMessageData recvData;
        ASSERT_FALSE(callEventHandlerTo->pollData(WAIT_TIME_FOR_MESSAGE, recvData));

        RtsMessageData byeRequest;
        ASSERT_TRUE(callEventHandlerTo->pollBye(WAIT_TIME_FOR_MESSAGE, byeRequest));
        ASSERT_EQ(callId, byeRequest.getCallId());
        ASSERT_EQ("", byeRequest.getDesc());

        RtsMessageData byeResponse;
        ASSERT_TRUE(callEventHandlerFrom->pollBye(WAIT_TIME_FOR_MESSAGE, byeResponse));
        ASSERT_EQ(callId, byeResponse.getCallId());
        ASSERT_EQ("CLOSED_INITIATIVELY", byeResponse.getDesc());

        createCall(callId, from, callEventHandlerFrom, to, callEventHandlerTo, appContent);
        to->closeCall(callId);
        ASSERT_NE(-1, from->sendRtsData(callId, sendData, VIDEO));
        ASSERT_FALSE(callEventHandlerTo->pollData(WAIT_TIME_FOR_MESSAGE, recvData));

        ASSERT_TRUE(callEventHandlerFrom->pollBye(WAIT_TIME_FOR_MESSAGE, byeRequest));
        ASSERT_EQ(callId, byeRequest.getCallId());
        ASSERT_EQ("", byeRequest.getDesc());

        ASSERT_TRUE(callEventHandlerTo->pollBye(WAIT_TIME_FOR_MESSAGE, byeResponse));
        ASSERT_EQ(callId, byeResponse.getCallId());
        ASSERT_EQ("CLOSED_INITIATIVELY", byeResponse.getDesc());

        ASSERT_EQ(-1, from->sendRtsData(callId, sendData, AUDIO));
        ASSERT_FALSE(callEventHandlerTo->pollData(WAIT_TIME_FOR_MESSAGE, recvData));
    }

    void createCallTimeout(User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallTimeoutResponseEventHandler* callEventHandlerTo) {
        logIn(from, callEventHandlerFrom);
        logIn(to, NULL);
        callEventHandlerTo->clear();

        uint64_t callId = from->dialCall(to->getAppAccount());
        ASSERT_NE(callId, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_TRUE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("", inviteRequest.getAppContent());

        RtsMessageData createResponse;
        ASSERT_FALSE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));

        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(RTS_CALL_TIMEOUT, createResponse));
        ASSERT_EQ(callId, createResponse.getCallId());
        ASSERT_EQ(false, createResponse.isAccepted());
        ASSERT_EQ(DIAL_CALL_TIMEOUT, createResponse.getDesc());

        RtsMessageData byeResponse_from;
        ASSERT_FALSE(callEventHandlerFrom->pollBye(WAIT_TIME_FOR_MESSAGE, byeResponse_from));

        RtsMessageData byeResponse_to;
        ASSERT_TRUE(callEventHandlerTo->pollBye(WAIT_TIME_FOR_MESSAGE, byeResponse_to));
        ASSERT_EQ(callId, byeResponse_to.getCallId());
    }

    void testP2PSendOneRtsSignalToOffline(User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo, string appContent) {
        logIn(from, callEventHandlerFrom);

        uint64_t callId = from->dialCall(to->getAppAccount(), appContent);
        ASSERT_NE(callId, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_FALSE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));

        RtsMessageData createResponse;
        ASSERT_FALSE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));

        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(RTS_CALL_TIMEOUT, createResponse));
        ASSERT_EQ(callId, createResponse.getCallId());
        ASSERT_EQ(false, createResponse.isAccepted());
        ASSERT_EQ(DIAL_CALL_TIMEOUT, createResponse.getDesc());
    }

    void testP2PSendOneRtsSignalFromOffline(User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo, string appContent) {
        logIn(to, callEventHandlerTo);

        uint64_t callId = from->dialCall(to->getAppAccount(), appContent);
        ASSERT_EQ(callId, 0);
    }

    void testCreateCallRefuse(User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo, string appContent) {
        logIn(from, callEventHandlerFrom);
        logIn(to, callEventHandlerTo);

        uint64_t callId = from->dialCall(to->getAppAccount(), appContent);
        ASSERT_NE(callId, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_TRUE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ(appContent, inviteRequest.getAppContent());

        RtsMessageData createResponse;
        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId, createResponse.getCallId());
        ASSERT_EQ(false, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom->LAUNCH_ERR_ILLEGALAPPCONTENT, createResponse.getDesc());
    }

    void testCreateCallWithSelf(User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo) {
        logIn(from, callEventHandlerFrom);
        logIn(to, callEventHandlerTo);

        uint64_t callId1 = from->dialCall(from->getAppAccount(), "", from->getResource());
        ASSERT_NE(callId1, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_FALSE(callEventHandlerFrom->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));

        RtsMessageData createResponse;
        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(RTS_CALL_TIMEOUT, createResponse));
        ASSERT_EQ(callId1, createResponse.getCallId());
        ASSERT_EQ(false, createResponse.isAccepted());
        ASSERT_EQ(DIAL_CALL_TIMEOUT, createResponse.getDesc());

        uint64_t callId2 = from->dialCall(from->getAppAccount(), "", to->getResource());
        ASSERT_NE(callId2, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        ASSERT_TRUE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId2, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());

        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(RTS_CALL_TIMEOUT, createResponse));
        ASSERT_EQ(callId2, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse.getDesc());

        closeCall(callId2, from, callEventHandlerFrom, callEventHandlerTo);
    }

    void testCreateCallFromMulResources(User* from_r1, TestRTSCallEventHandler* callEventHandlerFrom1, User* from_r2, TestRTSCallEventHandler* callEventHandlerFrom2, User* to, TestRTSCallEventHandler* callEventHandlerTo) {
        logIn(from_r1, callEventHandlerFrom1);
        logIn(from_r2, callEventHandlerFrom2);
        logIn(to, callEventHandlerTo);

        uint64_t callId = from_r1->dialCall(to->getAppAccount(), "ll123456");
        ASSERT_NE(callId, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_TRUE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId, inviteRequest.getCallId());
        ASSERT_EQ(from_r1->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from_r1->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        RtsMessageData createResponse;
        ASSERT_FALSE(callEventHandlerFrom2->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));

        ASSERT_TRUE(callEventHandlerFrom1->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom1->LAUNCH_OK, createResponse.getDesc());

        closeCall(callId, from_r1, callEventHandlerFrom1, callEventHandlerTo);
    }

    void testCreateCallWithMulResources(User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to_r1, TestRTSCallEventHandler* callEventHandlerTo1, User* to_r2, TestRTSCallDelayResponseEventHandler* callEventHandlerTo2) {
        logIn(from, callEventHandlerFrom);
        logIn(to_r1, callEventHandlerTo1);
        logIn(to_r2, NULL);
        callEventHandlerTo2->clear();
        to_r2->registerRTSCallEventHandler(callEventHandlerTo2);

        uint64_t callId = from->dialCall(to_r1->getAppAccount(), "ll123456", to_r1->getResource());
        ASSERT_NE(callId, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_FALSE(callEventHandlerTo2->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));

        ASSERT_TRUE(callEventHandlerTo1->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        RtsMessageData createResponse;
        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse.getDesc());

        closeCall(callId, from, callEventHandlerFrom, callEventHandlerTo1);

        //call with no given resource, success with the resource answer first
		callId = from->dialCall(to_r1->getAppAccount(), "ll123456");

        ASSERT_NE(callId, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        ASSERT_TRUE(callEventHandlerTo1->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        ASSERT_TRUE(callEventHandlerTo2->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse.getDesc());
        ASSERT_FALSE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));

        RtsMessageData byeRequest;
        ASSERT_FALSE(callEventHandlerTo1->pollBye(WAIT_TIME_FOR_MESSAGE, byeRequest));
        ASSERT_TRUE(callEventHandlerTo2->pollBye(WAIT_TIME_FOR_MESSAGE * 4, byeRequest));
        ASSERT_EQ(callId, byeRequest.getCallId());
        ASSERT_EQ("already answered", byeRequest.getDesc());

        closeCall(callId, from, callEventHandlerFrom, callEventHandlerTo1);

        //call with no given resource, resource2 answer after call close
		callId = from->dialCall(to_r1->getAppAccount(), "ll123456");

        ASSERT_NE(callId, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        ASSERT_TRUE(callEventHandlerTo1->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        ASSERT_TRUE(callEventHandlerTo2->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId, inviteRequest.getCallId());
        ASSERT_EQ(from->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(from->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse.getDesc());
        ASSERT_FALSE(callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));

        closeCall(callId, from, callEventHandlerFrom, callEventHandlerTo1);

        ASSERT_TRUE(callEventHandlerTo2->pollBye(WAIT_TIME_FOR_MESSAGE * 4, byeRequest));
        ASSERT_EQ(callId, byeRequest.getCallId());
        ASSERT_EQ("illegal request", byeRequest.getDesc());
    }

    //同一个用户不能接通超过maxcallnum的电话
    void testCreateCallWhenCalling(User* user1_r1, TestRTSCallEventHandler* callEventHandler1_r1, User* user1_r2, TestRTSCallEventHandler* callEventHandler1_r2, User* user2_r1, TestRTSCallEventHandler* callEventHandler2_r1, User* user2_r2, TestRTSCallEventHandler* callEventHandler2_r2) {
        logIn(user1_r1, callEventHandler1_r1);
        logIn(user1_r2, callEventHandler1_r2);
        logIn(user2_r1, callEventHandler2_r1);
        logIn(user2_r2, callEventHandler2_r2);

        uint64_t callId1 = user1_r1->dialCall(user2_r1->getAppAccount(), "ll123456", user2_r1->getResource());
        ASSERT_NE(callId1, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_TRUE(callEventHandler2_r1->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId1, inviteRequest.getCallId());
        ASSERT_EQ(user1_r1->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(user1_r1->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        RtsMessageData createResponse;
        ASSERT_TRUE(callEventHandler1_r1->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId1, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandler1_r1->LAUNCH_OK, createResponse.getDesc());

        uint64_t callId2 = user1_r2->dialCall(user2_r1->getAppAccount(), "ll123456", user2_r1->getResource());
        ASSERT_NE(callId2, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        ASSERT_FALSE(callEventHandler2_r1->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));

        ASSERT_TRUE(callEventHandler1_r2->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId2, createResponse.getCallId());
        ASSERT_EQ(false, createResponse.isAccepted());
        ASSERT_EQ("USER_BUSY", createResponse.getDesc());

        uint64_t callId3 = user1_r2->dialCall(user2_r1->getAppAccount(), "ll123456", user2_r2->getResource());
        ASSERT_NE(callId3, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        ASSERT_TRUE(callEventHandler2_r2->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId3, inviteRequest.getCallId());
        ASSERT_EQ(user1_r2->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(user1_r2->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        ASSERT_TRUE(callEventHandler1_r2->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId3, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandler1_r2->LAUNCH_OK, createResponse.getDesc());

        closeCall(callId1, user1_r1, callEventHandler1_r1, callEventHandler2_r1);
        closeCall(callId3, user1_r2, callEventHandler1_r2, callEventHandler2_r2);
    }

    void testRecvCallWhenCalling(User* user1_r1, TestRTSCallEventHandler* callEventHandler1_r1, User* user1_r2, TestRTSCallEventHandler* callEventHandler1_r2, User* user2_r1, TestRTSCallEventHandler* callEventHandler2_r1, User* user2_r2, TestRTSCallEventHandler* callEventHandler2_r2) {
        logIn(user1_r1, callEventHandler1_r1);
        logIn(user1_r2, callEventHandler1_r2);
        logIn(user2_r1, callEventHandler2_r1);
        logIn(user2_r2, callEventHandler2_r2);

        uint64_t callId1 = user1_r1->dialCall(user2_r1->getAppAccount(), "ll123456", user2_r1->getResource());
        ASSERT_NE(callId1, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData inviteRequest;
        ASSERT_TRUE(callEventHandler2_r1->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId1, inviteRequest.getCallId());
        ASSERT_EQ(user1_r1->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(user1_r1->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        RtsMessageData createResponse;
        ASSERT_TRUE(callEventHandler1_r1->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId1, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandler1_r1->LAUNCH_OK, createResponse.getDesc());

        uint64_t callId2 = user2_r2->dialCall(user1_r1->getAppAccount());
        ASSERT_NE(callId2, 0);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        ASSERT_FALSE(callEventHandler1_r1->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));

        ASSERT_TRUE(callEventHandler1_r2->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(callId2, inviteRequest.getCallId());
        ASSERT_EQ(user2_r2->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(user2_r2->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("", inviteRequest.getAppContent());

        ASSERT_TRUE(callEventHandler2_r2->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId2, createResponse.getCallId());
        if ("USER_BUSY" == createResponse.getDesc()) {
            ASSERT_EQ(false, createResponse.isAccepted());
        } else if (callEventHandler2_r2->LAUNCH_OK == createResponse.getDesc()) {
            ASSERT_EQ(true, createResponse.isAccepted());
            closeCall(callId2, user2_r2, callEventHandler2_r2, callEventHandler1_r2);
        }
        closeCall(callId1, user1_r1, callEventHandler1_r1, callEventHandler2_r1);
    }

    void testCreateCalls(User* user1, TestRTSCallEventHandler* callEventHandler1, User* user2, TestRTSCallEventHandler* callEventHandler2, User* user3, TestRTSCallEventHandler* callEventHandler3) {
        logIn(user1, callEventHandler1);
        logIn(user2, callEventHandler2);
        logIn(user3, callEventHandler3);

        //user1 call user2 and user3
        uint64_t callId1 = user1->dialCall(user2->getAppAccount(), "ll123456");
        uint64_t callId2 = user1->dialCall(user3->getAppAccount());
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));
        ASSERT_NE(callId1, 0);
        ASSERT_EQ(callId2, 0);

        RtsMessageData inviteRequest;
        ASSERT_TRUE(callEventHandler2->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));
        ASSERT_EQ(inviteRequest.getCallId(), callId1);
        ASSERT_EQ(user1->getAppAccount(), inviteRequest.getFromAccount());
        ASSERT_EQ(user1->getResource(), inviteRequest.getFromResource());
        ASSERT_EQ("ll123456", inviteRequest.getAppContent());

        ASSERT_FALSE(callEventHandler3->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));

        RtsMessageData createResponse;
        ASSERT_TRUE(callEventHandler1->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
        ASSERT_EQ(callId1, createResponse.getCallId());
        ASSERT_EQ(true, createResponse.isAccepted());
        ASSERT_EQ(callEventHandler1->LAUNCH_OK, createResponse.getDesc());

        closeCall(callId1, user1, callEventHandler1, callEventHandler2);
    }

    void testCreateCallWithOffline(User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo) {
        logIn(from, callEventHandlerFrom);
        logIn(to, callEventHandlerTo);
        ASSERT_TRUE(to->logout());

        //sleep(1);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        uint64_t callId = from->dialCall(to->getAppAccount(), "ll123456");
        ASSERT_NE(0, callId);

        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        logIn(to, NULL);
        RtsMessageData inviteRequest;
        ASSERT_FALSE(callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE, inviteRequest));

        RtsMessageData createResponse;
        ASSERT_TRUE(callEventHandlerFrom->pollCreateResponse(RTS_CALL_TIMEOUT, createResponse));
        ASSERT_EQ(callId, createResponse.getCallId());
        ASSERT_EQ(false, createResponse.isAccepted());
        ASSERT_EQ(DIAL_CALL_TIMEOUT, createResponse.getDesc());
    }

    void testCloseCall(uint64_t callId, User* from, TestRTSCallEventHandler* callEventHandlerFrom, TestRTSCallEventHandler* callEventHandlerTo) {
        string byeReason = "i don't wanna talk";

        from->closeCall(callId, byeReason);
        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData byeRequest;
        ASSERT_TRUE(callEventHandlerTo->pollBye(WAIT_TIME_FOR_MESSAGE, byeRequest));
        ASSERT_EQ(callId, byeRequest.getCallId());
        ASSERT_EQ(byeReason, byeRequest.getDesc());

        RtsMessageData byeResponse;
        ASSERT_TRUE(callEventHandlerFrom->pollBye(WAIT_TIME_FOR_MESSAGE, byeResponse));
        ASSERT_EQ(callId, byeResponse.getCallId());
        ASSERT_EQ("CLOSED_INITIATIVELY", byeResponse.getDesc());
    }

    void closeCallSameTime(uint64_t callId, User* from, TestRTSCallEventHandler* callEventHandlerFrom, User* to, TestRTSCallEventHandler* callEventHandlerTo) {
        string byeReasonFrom = "close by from";
        string byeReasonTo = "close by to";

        from->closeCall(callId, byeReasonFrom);
        to->closeCall(callId, byeReasonTo);

        //sleep(1);
		std::this_thread::sleep_for(std::chrono::seconds(1));

        RtsMessageData byeRequest;
        ASSERT_TRUE(callEventHandlerTo->pollBye(WAIT_TIME_FOR_MESSAGE, byeRequest));
        ASSERT_EQ(callId, byeRequest.getCallId());
        ASSERT_EQ("CLOSED_INITIATIVELY", byeRequest.getDesc());

        ASSERT_TRUE(callEventHandlerFrom->pollBye(WAIT_TIME_FOR_MESSAGE, byeRequest));
        ASSERT_EQ(callId, byeRequest.getCallId());
        ASSERT_EQ("CLOSED_INITIATIVELY", byeRequest.getDesc());
    }
protected:
    User* rtsUser1_r1;
    User* rtsUser1_r2;
    User* rtsUser2_r1;
    User* rtsUser2_r2;
    User* rtsUser3;

    TestTokenFetcher* tokenFetcher1_r1;
    TestTokenFetcher* tokenFetcher1_r2;
    TestTokenFetcher* tokenFetcher2_r1;
    TestTokenFetcher* tokenFetcher2_r2;
    TestTokenFetcher* tokenFetcher3;

    TestOnlineStatusHandler* onlineStatusHandler1_r1;
    TestOnlineStatusHandler* onlineStatusHandler1_r2;
    TestOnlineStatusHandler* onlineStatusHandler2_r1;
    TestOnlineStatusHandler* onlineStatusHandler2_r2;
    TestOnlineStatusHandler* onlineStatusHandler3;

    TestMessageHandler* msgHandler1_r1;
    TestMessageHandler* msgHandler1_r2;
    TestMessageHandler* msgHandler2_r1;
    TestMessageHandler* msgHandler2_r2;
    TestMessageHandler* msgHandler3;

    TestRTSCallEventHandler* callEventHandler1_r1;
    TestRTSCallEventHandler* callEventHandler1_r2;
    TestRTSCallEventHandler* callEventHandler2_r1;
    TestRTSCallEventHandler* callEventHandler2_r2;
    TestRTSCallEventHandler* callEventHandler3;
    TestRTSCallDelayResponseEventHandler* callEventHandlerDelayResponse;
    TestRTSCallTimeoutResponseEventHandler* callEventHandlerTimeoutResponse;
};

TEST_F(RtsTest, testP2PSendOneRtsSignalToOffline) {
    testP2PSendOneRtsSignalToOffline();
}

TEST_F(RtsTest, testP2PSendOneRtsSignalFromOffline) {
    testP2PSendOneRtsSignalFromOffline();
}

TEST_F(RtsTest, testCreateCallRefuse) {
    testCreateCallRefuse();
}

TEST_F(RtsTest, testCreateCall) {
    testCreateCall();
}

TEST_F(RtsTest, testCreateCallTimeout) {
    testCreateCallTimeout();
}

TEST_F(RtsTest, testCreateCallWithSelf) {
    testCreateCallWithSelf();
}

TEST_F(RtsTest, testCreateCallFromMulResources) {
    testCreateCallFromMulResources();
}

TEST_F(RtsTest, testCreateCallWithMulResources) {
    testCreateCallWithMulResources();
}

TEST_F(RtsTest, testCreateCallWhenCalling) {
    testCreateCallWhenCalling();
}

TEST_F(RtsTest, testRecvCallWhenCalling) {
    testRecvCallWhenCalling();
}

TEST_F(RtsTest, testCreateCalls) {
    testCreateCalls();
}

TEST_F(RtsTest, testCreateCallWithOffline) {
    testCreateCallWithOffline();
}

TEST_F(RtsTest, testCloseCall) {
    testCloseCall();
}

TEST_F(RtsTest, testCloseCallSameTime) {
    testCloseCallSameTime();
}

TEST_F(RtsTest, testSendDatas) {
    testSendDatas();
}

TEST_F(RtsTest, testSendDataToEachOther) {
    testSendDataToEachOther();
}

TEST_F(RtsTest, testSendDataAfterClose) {
    testSendDataAfterClose();
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
