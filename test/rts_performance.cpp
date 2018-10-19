#include <gtest/gtest.h>
#include <mimc/user.h>
#include <mimc/utils.h>
#include <mimc/error.h>
#include <test/mimc_tokenfetcher.h>
#include <test/mimc_onlinestatus_handler.h>
#include <test/mimc_message_handler.h>
#include <test/rts_performance_handler.h>
#include <map>

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
string appAccount1 = "perf_MI153";
string appAccount2 = "perf_MI108";
const int WAIT_TIME_FOR_MESSAGE = 1;

class RtsPerformance: public testing::Test {
protected:
	void SetUp() {
		rtsUser1 = new User(appAccount1);
		msgHandler1 = new TestMessageHandler();
		callEventHandler1 = new RtsPerformanceHandler();

		rtsUser2 = new User(appAccount2);
		msgHandler2 = new TestMessageHandler();
		callEventHandler2 = new RtsPerformanceHandler();

		rtsUser1->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount1));
		rtsUser1->registerOnlineStatusHandler(new TestOnlineStatusHandler());
		rtsUser1->registerMessageHandler(msgHandler1);
		rtsUser1->registerRTSCallEventHandler(callEventHandler1);

		rtsUser2->registerTokenFetcher(new TestTokenFetcher(appId, appKey, appSecret, appAccount2));
		rtsUser2->registerOnlineStatusHandler(new TestOnlineStatusHandler());
		rtsUser2->registerMessageHandler(msgHandler2);
		rtsUser2->registerRTSCallEventHandler(callEventHandler2);
	}

	void TearDown() {
		rtsUser1->logout();
		rtsUser2->logout();

		usleep(100);

		delete rtsUser1;
		rtsUser1 = NULL;

		delete rtsUser2;
		rtsUser2 = NULL;
	}

	//@test
	void performanceTest() {
		const int dataSizeKB = 150;
		const int speedKB = 300;
		const int durationSec = 100;
		testPerformance(rtsUser1, callEventHandler1, rtsUser2, callEventHandler2, dataSizeKB, speedKB, durationSec);
	}

	void testPerformance(User* user1, RtsPerformanceHandler* callEventHandler1, User* user2, RtsPerformanceHandler* callEventHandler2, int dataSizeKB, int dataSpeedKB, int durationSec) {
		int sendFailed = 0;
		int lost = 0;
		int dataError = 0;
		int time0To50 = 0;
		int time51To100 = 0;
		int time101To150 = 0;
		int time151To200 = 0;
		int time201To300 = 0;
		int time301To400 = 0;
		int time401To500 = 0;
		int time501To1000 = 0;
		int time1001More = 0;

		map<int, RtsPerformanceData> sendDatas;

		logIn(user1, callEventHandler1);
		logIn(user2, callEventHandler2);

		long chatId = createCall(user1, callEventHandler1, user2, callEventHandler2);

		const int COUNT = (durationSec * dataSpeedKB) / dataSizeKB;
		const int TIMEVAL_US = 1000 * (1000 * dataSizeKB / dataSpeedKB);
		for (int dataId = 0; dataId < COUNT; dataId++) {
			string sendData = Utils::generateRandomString(dataSizeKB * 1024 - 4);
			unsigned char dataId_char[4];
			int2char(dataId, dataId_char, 0);
			sendData.insert(0, (char *)dataId_char, 4);
			if (user1->sendRtsData(chatId, sendData, AUDIO)) {
				sendDatas.insert(pair<int, RtsPerformanceData>(dataId, RtsPerformanceData(sendData, Utils::currentTimeMillis())));
			}
			usleep(TIMEVAL_US);
		}
		sleep(5);
		const map<int, RtsPerformanceData>& recvDatas = callEventHandler2->pollDataInfo();
		closeCall(chatId, user1, callEventHandler1, callEventHandler2);

		for (int i = 0; i < COUNT; i++) {
			if (sendDatas.count(i) == 0) {
				XMDLoggerWrapper::instance()->warn("DATA_ID:%d, SEND FAILED", i);
				sendFailed++;
			} else if (recvDatas.count(i) == 0) {
				XMDLoggerWrapper::instance()->warn("DATA_ID:%d, LOST", i);
				lost++;
			} else if (recvDatas.at(i).getData() != sendDatas.at(i).getData()) {
				XMDLoggerWrapper::instance()->warn("DATA_ID:%d, RECEIVE DATA NOT MATCH", i);
				dataError++;
			} else if ((int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()) <= 50) {
				XMDLoggerWrapper::instance()->info("DATA_ID:%d, %ld - %ld = %d", i, recvDatas.at(i).getDataTime(), sendDatas.at(i).getDataTime(), (int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()));
				time0To50++;
			} else if ((int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()) <= 100) {
				XMDLoggerWrapper::instance()->info("DATA_ID:%d, %ld - %ld = %d", i, recvDatas.at(i).getDataTime(), sendDatas.at(i).getDataTime(), (int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()));
				time51To100++;
			} else if ((int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()) <= 150) {
				XMDLoggerWrapper::instance()->info("DATA_ID:%d, %ld - %ld = %d", i, recvDatas.at(i).getDataTime(), sendDatas.at(i).getDataTime(), (int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()));
				time101To150++;
			} else if ((int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()) <= 200) {
				XMDLoggerWrapper::instance()->info("DATA_ID:%d, %ld - %ld = %d", i, recvDatas.at(i).getDataTime(), sendDatas.at(i).getDataTime(), (int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()));
				time151To200++;
			} else if ((int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()) <= 300) {
				XMDLoggerWrapper::instance()->info("DATA_ID:%d, %ld - %ld = %d", i, recvDatas.at(i).getDataTime(), sendDatas.at(i).getDataTime(), (int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()));
				time201To300++;
			} else if ((int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()) <= 400) {
				XMDLoggerWrapper::instance()->info("DATA_ID:%d, %ld - %ld = %d", i, recvDatas.at(i).getDataTime(), sendDatas.at(i).getDataTime(), (int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()));
				time301To400++;
			} else if ((int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()) <= 500) {
				XMDLoggerWrapper::instance()->info("DATA_ID:%d, %ld - %ld = %d", i, recvDatas.at(i).getDataTime(), sendDatas.at(i).getDataTime(), (int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()));
				time401To500++;
			} else if ((int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()) <= 1000) {
				XMDLoggerWrapper::instance()->info("DATA_ID:%d, %ld - %ld = %d", i, recvDatas.at(i).getDataTime(), sendDatas.at(i).getDataTime(), (int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()));
				time501To1000++;
			} else {
				XMDLoggerWrapper::instance()->info("DATA_ID:%d, %ld - %ld = %d", i, recvDatas.at(i).getDataTime(), sendDatas.at(i).getDataTime(), (int)(recvDatas.at(i).getDataTime() - sendDatas.at(i).getDataTime()));
				time1001More++;
			}
		}

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

	void logIn(User* user, RtsPerformanceHandler* callEventHandler) {
		long loginTs = time(NULL);
		user->login();
		while (time(NULL) - loginTs < LOGIN_TIMEOUT && user->getOnlineStatus() == Offline) {
			usleep(50000);
		}
		ASSERT_EQ(Online, user->getOnlineStatus());
		if (callEventHandler != NULL) {
			XMDLoggerWrapper::instance()->info("CLEAR CALLEVENTHANDLER OF UUID:%ld", user->getUuid());
			callEventHandler->clear();
		}
	}

	long createCall(User* from, RtsPerformanceHandler* callEventHandlerFrom, User* to, RtsPerformanceHandler* callEventHandlerTo, const string& appContent = "") {
		long chatId = from->dialCall(to->getAppAccount(), appContent);
		EXPECT_NE(chatId, -1);
		sleep(1);

		RtsMessageData* inviteRequest = callEventHandlerTo->pollInviteRequest(WAIT_TIME_FOR_MESSAGE);
		EXPECT_FALSE(inviteRequest == NULL);
		if (inviteRequest != NULL) {
			EXPECT_EQ(chatId, inviteRequest->getChatId());
			EXPECT_EQ(from->getAppAccount(), inviteRequest->getFromAccount());
			EXPECT_EQ(from->getResource(), inviteRequest->getFromResource());
			EXPECT_EQ(appContent, inviteRequest->getAppContent());
		}

		RtsMessageData* createResponse = callEventHandlerFrom->pollCreateResponse(WAIT_TIME_FOR_MESSAGE);
		EXPECT_FALSE(createResponse == NULL);
		if (createResponse != NULL) {
			EXPECT_EQ(chatId, createResponse->getChatId());
			EXPECT_EQ(true, createResponse->isAccepted());
			EXPECT_EQ(callEventHandlerFrom->LAUNCH_OK, createResponse->getErrmsg());
		}

		return chatId;
	}

	void closeCall(long chatId, User* from, RtsPerformanceHandler* callEventHandlerFrom, RtsPerformanceHandler* callEventHandlerTo) {
		from->closeCall(chatId);
		sleep(1);

		RtsMessageData* byeRequest = callEventHandlerTo->pollBye(WAIT_TIME_FOR_MESSAGE);
		ASSERT_FALSE(byeRequest == NULL);
		ASSERT_EQ(chatId, byeRequest->getChatId());
		ASSERT_EQ("", byeRequest->getErrmsg());

		RtsMessageData* byeResponse = callEventHandlerFrom->pollBye(WAIT_TIME_FOR_MESSAGE);
		ASSERT_FALSE(byeResponse == NULL);
		ASSERT_EQ(chatId, byeResponse->getChatId());
		ASSERT_EQ("CLOSED_INITIATIVELY", byeResponse->getErrmsg());
	}

	void int2char(int data, unsigned char* result, int index) {
		unsigned char firstByte = data & (0xff);
		unsigned char secondByte = (data >> 8) & (0xff);
		unsigned char thirdByte = (data >> 16) & (0xff);
		unsigned char fourthByte = (data >> 24) & (0xff);
		result[index] = fourthByte;
		result[index + 1] = thirdByte;
		result[index + 2] = secondByte;
		result[index + 3] = firstByte;
	}



protected:
	User* rtsUser1;
	User* rtsUser2;

	TestMessageHandler* msgHandler1;
	TestMessageHandler* msgHandler2;

	RtsPerformanceHandler* callEventHandler1;
	RtsPerformanceHandler* callEventHandler2;
};


TEST_F(RtsPerformance, performanceTest) {
	performanceTest();
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}