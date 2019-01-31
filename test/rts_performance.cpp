#include <gtest/gtest.h>
#include <test/rts_performance.h>
#include <test/rts_performance_handler.h>

void RtsPerformance::SetUp() {
	rtsUser1 = new User(atoll(appId.c_str()), appAccount1);
	msgHandler1 = new TestMessageHandler();
	callEventHandler1 = new RtsPerformanceHandler(this);

	rtsUser2 = new User(atoll(appId.c_str()), appAccount2);
	msgHandler2 = new TestMessageHandler();
	callEventHandler2 = new RtsPerformanceHandler(this);

	rtsUser1->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount1));
	rtsUser1->registerOnlineStatusHandler(new TestOnlineStatusHandler());
	rtsUser1->registerMessageHandler(msgHandler1);
	rtsUser1->registerRTSCallEventHandler(callEventHandler1);

	rtsUser2->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount2));
	rtsUser2->registerOnlineStatusHandler(new TestOnlineStatusHandler());
	rtsUser2->registerMessageHandler(msgHandler2);
	rtsUser2->registerRTSCallEventHandler(callEventHandler2);
}

void RtsPerformance::TearDown() {
	rtsUser1->logout();
	rtsUser2->logout();

	usleep(100);

	delete rtsUser1;
	rtsUser1 = NULL;

	delete rtsUser2;
	rtsUser2 = NULL;
}

void RtsPerformance::performanceTest() {
	const int dataSizeKB = 150;
	const int speedKB = 300;
	const int durationSec = 100;
	testPerformance(rtsUser1, callEventHandler1, rtsUser2, callEventHandler2, dataSizeKB, speedKB, durationSec);
}

void RtsPerformance::testPerformance(User* user1, RtsPerformanceHandler* callEventHandler1, User* user2, RtsPerformanceHandler* callEventHandler2, int dataSizeKB, int dataSpeedKB, int durationSec) {
	sendFailed = 0;
	lost = 0;
	dataError = 0;
	time0To50 = 0;
	time51To100 = 0;
	time101To150 = 0;
	time151To200 = 0;
	time201To300 = 0;
	time301To400 = 0;
	time401To500 = 0;
	time501To1000 = 0;
	time1001More = 0;

	logIn(user1, callEventHandler1);
	logIn(user2, callEventHandler2);

	uint64_t callId = 0;
	createCall(callId, user1, callEventHandler1, user2, callEventHandler2);

	const int COUNT = (durationSec * dataSpeedKB) / dataSizeKB;
	const int TIMEVAL_US = 1000 * (1000 * dataSizeKB / dataSpeedKB);
	for (int i = 0; i < COUNT; i++) {
		string sendData = Utils::generateRandomString(dataSizeKB * 1024 - 8);
		unsigned char timestamp[8];
		long2char(Utils::currentTimeMillis(), timestamp, 0);
		sendData.insert(0, (char *)timestamp, 8);
		if (user1->sendRtsData(callId, sendData, AUDIO) == -1) {
			XMDLoggerWrapper::instance()->warn("DATA_ID:%d, SEND FAILED", i);
			sendFailed++;
		}
		usleep(TIMEVAL_US);
	}
	sleep(5);
	closeCall(callId, user1, callEventHandler1, callEventHandler2);

	lost = COUNT - sendFailed - time0To50 - time51To100 - time101To150 - time151To200 - time201To300 - time301To400 - time401To500 - time501To1000 - time1001More;

	XMDLoggerWrapper::instance()->warn("PERFORMANCE TEST RESULT:");
	XMDLoggerWrapper::instance()->warn("SEND %d DATAS DURING %ds, DATA SIZE: %dKB, SPEED: %dKB/S", COUNT, durationSec, dataSizeKB, dataSpeedKB);
	XMDLoggerWrapper::instance()->warn("FAILED: %d", sendFailed);
	XMDLoggerWrapper::instance()->warn("LOST: %d", lost);
	XMDLoggerWrapper::instance()->warn("DATA ERROR: %d", dataError);
	XMDLoggerWrapper::instance()->warn("RECV_TIME(0, 50ms]: %d", time0To50);
	XMDLoggerWrapper::instance()->warn("RECV_TIME(50, 100ms]: %d", time51To100);
	XMDLoggerWrapper::instance()->warn("RECV_TIME(100, 150ms]: %d", time101To150);
	XMDLoggerWrapper::instance()->warn("RECV_TIME(150, 200ms]: %d", time151To200);
	XMDLoggerWrapper::instance()->warn("RECV_TIME(200, 300ms]: %d", time201To300);
	XMDLoggerWrapper::instance()->warn("RECV_TIME(300, 400ms]: %d", time301To400);
	XMDLoggerWrapper::instance()->warn("RECV_TIME(400, 500ms]: %d", time401To500);
	XMDLoggerWrapper::instance()->warn("RECV_TIME(500, 1000ms]: %d", time501To1000);
	XMDLoggerWrapper::instance()->warn("RECV_TIME 1000+ms: %d", time1001More);
}

void RtsPerformance::logIn(User* user, RtsPerformanceHandler* callEventHandler) {
	time_t loginTs = time(NULL);
	user->login();
	XMDLoggerWrapper::instance()->info("user %s called login", user->getAppAccount().c_str());
	while (time(NULL) - loginTs < LOGIN_TIMEOUT && user->getOnlineStatus() == Offline) {
		usleep(50000);
	}
	ASSERT_EQ(Online, user->getOnlineStatus());
	if (callEventHandler != NULL) {
		XMDLoggerWrapper::instance()->info("CLEAR CALLEVENTHANDLER OF UUID:%lld", user->getUuid());
		callEventHandler->clear();
	}
}

void RtsPerformance::createCall(uint64_t& callId, User* from, RtsPerformanceHandler* callEventHandlerFrom, User* to, RtsPerformanceHandler* callEventHandlerTo, const string& appContent) {
	callId = from->dialCall(to->getAppAccount(), appContent);
	ASSERT_NE(callId, 0);
	sleep(1);

	RtsMessageData* inviteRequest = callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE);
	ASSERT_FALSE(inviteRequest == NULL);
	if (inviteRequest != NULL) {
		ASSERT_EQ(callId, inviteRequest->getCallId());
		ASSERT_EQ(from->getAppAccount(), inviteRequest->getFromAccount());
		ASSERT_EQ(from->getResource(), inviteRequest->getFromResource());
		ASSERT_EQ(appContent, inviteRequest->getAppContent());
	}

	RtsMessageData* createResponse = callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE);
	ASSERT_FALSE(createResponse == NULL);
	if (createResponse != NULL) {
		ASSERT_EQ(callId, createResponse->getCallId());
		ASSERT_EQ(true, createResponse->isAccepted());
		ASSERT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse->getDesc());
	}
}

void RtsPerformance::closeCall(uint64_t callId, User* from, RtsPerformanceHandler* callEventHandlerFrom, RtsPerformanceHandler* callEventHandlerTo) {
	from->closeCall(callId);
	sleep(1);

	RtsMessageData* byeRequest = callEventHandlerTo->pollBye(WAIT_TIME_FOR_MESSAGE);
	ASSERT_FALSE(byeRequest == NULL);
	ASSERT_EQ(callId, byeRequest->getCallId());
	ASSERT_EQ("", byeRequest->getDesc());

	RtsMessageData* byeResponse = callEventHandlerFrom->pollBye(WAIT_TIME_FOR_MESSAGE);
	ASSERT_FALSE(byeResponse == NULL);
	ASSERT_EQ(callId, byeResponse->getCallId());
	ASSERT_EQ("CLOSED_INITIATIVELY", byeResponse->getDesc());
}

TEST_F(RtsPerformance, performanceTest) {
	performanceTest();
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}