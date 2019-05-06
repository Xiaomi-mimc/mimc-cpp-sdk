#include <gtest/gtest.h>
#include <mimc/user.h>
#include <mimc/utils.h>
#include <test/mimc_tokenfetcher.h>
#include <test/mimc_onlinestatus_handler.h>
#include <test/mimc_message_handler.h>
#include <test/rts_efficiency_handler.h>
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
string appAccount1 = "effi_MI153";
string appAccount2 = "effi_MI108";
string appAccount3 = "effi_MI110";
const int WAIT_TIME_FOR_MESSAGE = 1;
const int TIME_OUT = 5000;

class RtsEfficiency : public testing::Test {
protected:
	void construct() {
		rtsUser1 = new User(atoll(appId.c_str()), appAccount1);
		rtsUser2 = new User(atoll(appId.c_str()), appAccount2);
		rtsUser3 = new User(atoll(appId.c_str()), appAccount3);

		tokenFetcher1 = new TestTokenFetcher(appId, appKey, appSecret, appAccount1);
		tokenFetcher2 = new TestTokenFetcher(appId, appKey, appSecret, appAccount2);
		tokenFetcher3 = new TestTokenFetcher(appId, appKey, appSecret, appAccount3);

		onlineStatusHandler1 = new TestOnlineStatusHandler();
		onlineStatusHandler2 = new TestOnlineStatusHandler();
		onlineStatusHandler3 = new TestOnlineStatusHandler();

		msgHandler1 = new TestMessageHandler();
		msgHandler2 = new TestMessageHandler();
		msgHandler3 = new TestMessageHandler();

		callEventHandler1 = new RtsEfficiencyHandler();
		callEventHandler2 = new RtsEfficiencyHandler();
		callEventHandler3 = new RtsEfficiencyHandler();

		rtsUser1->registerTokenFetcher(tokenFetcher1);
		rtsUser1->registerOnlineStatusHandler(onlineStatusHandler1);
		rtsUser1->registerMessageHandler(msgHandler1);
		rtsUser1->registerRTSCallEventHandler(callEventHandler1);

		rtsUser2->registerTokenFetcher(tokenFetcher2);
		rtsUser2->registerOnlineStatusHandler(onlineStatusHandler2);
		rtsUser2->registerMessageHandler(msgHandler2);
		rtsUser2->registerRTSCallEventHandler(callEventHandler2);

		rtsUser3->registerTokenFetcher(tokenFetcher3);
		rtsUser3->registerOnlineStatusHandler(onlineStatusHandler3);
		rtsUser3->registerMessageHandler(msgHandler3);
		rtsUser3->registerRTSCallEventHandler(callEventHandler3);
	}

	void destruct() {
		rtsUser1->logout();
		rtsUser2->logout();
		rtsUser3->logout();

		usleep(100);

		delete rtsUser1;
		rtsUser1 = NULL;

		delete rtsUser2;
		rtsUser2 = NULL;

		delete rtsUser3;
		rtsUser3 = NULL;

		delete tokenFetcher1;
		tokenFetcher1 = NULL;

		delete tokenFetcher2;
		tokenFetcher2 = NULL;

		delete tokenFetcher3;
		tokenFetcher3 = NULL;

		delete onlineStatusHandler1;
		onlineStatusHandler1 = NULL;

		delete onlineStatusHandler2;
		onlineStatusHandler2 = NULL;

		delete onlineStatusHandler3;
		onlineStatusHandler3 = NULL;

		delete msgHandler1;
		msgHandler1 = NULL;

		delete msgHandler2;
		msgHandler2 = NULL;

		delete msgHandler3;
		msgHandler3 = NULL;

		delete callEventHandler1;
		callEventHandler1 = NULL;

		delete callEventHandler2;
		callEventHandler2 = NULL;

		delete callEventHandler3;
		callEventHandler3 = NULL;
	}

	//@test
	void efficiencyTest() {
		curl_global_init(CURL_GLOBAL_ALL);
		const int runCount = 5;
		const int dataSize = 100*1024;
		map<int, RtsPerformanceData> timeRecords;

		for (int i = 0; i < runCount; i++) {
			construct();
			testEfficiency(rtsUser1, callEventHandler1, rtsUser2, callEventHandler2, rtsUser3, callEventHandler3, i, dataSize, timeRecords);
			destruct();
		}

		int sumLoginTime1 = 0;
		int sumLoginTime2 = 0;
		int sumLoginTime3 = 0;
		int sumDialCallTime = 0;
		int sumRecvDataTime = 0;
		int timeRecordsSize = timeRecords.size();
		for (int i = 0; i < timeRecordsSize; i++) {
			XMDLoggerWrapper::instance()->info("TEST_TIME_%d, ", i);
			XMDLoggerWrapper::instance()->info("DATA SIZE: %dKB", dataSize/1024);
			XMDLoggerWrapper::instance()->info("loginTime1: %dms", timeRecords[i].getLoginTime1());
			XMDLoggerWrapper::instance()->info("loginTime2: %dms", timeRecords[i].getLoginTime2());
			XMDLoggerWrapper::instance()->info("loginTime3: %dms", timeRecords[i].getLoginTime3());
			XMDLoggerWrapper::instance()->info("dialCallTime1: %dms", timeRecords[i].getDialCallTime1());
			XMDLoggerWrapper::instance()->info("dialCallTime2: %dms", timeRecords[i].getDialCallTime2());
			XMDLoggerWrapper::instance()->info("dialCallTime3: %dms", timeRecords[i].getDialCallTime3());
			XMDLoggerWrapper::instance()->info("recvDataTime1: %dms", timeRecords[i].getRecvDataTime1());
			XMDLoggerWrapper::instance()->info("recvDataTime2: %dms", timeRecords[i].getRecvDataTime2());
			XMDLoggerWrapper::instance()->info("recvDataTime3: %dms", timeRecords[i].getRecvDataTime3());
			sumLoginTime1 += timeRecords[i].getLoginTime1();
			sumLoginTime2 += timeRecords[i].getLoginTime2();
			sumLoginTime3 += timeRecords[i].getLoginTime3();
			sumDialCallTime += timeRecords[i].getDialCallTime1() + timeRecords[i].getDialCallTime2() + timeRecords[i].getDialCallTime3();
			sumRecvDataTime += timeRecords[i].getRecvDataTime1() + timeRecords[i].getRecvDataTime2() + timeRecords[i].getRecvDataTime3();
		}

		XMDLoggerWrapper::instance()->info("TEST %d TIMES, ", timeRecordsSize);
		XMDLoggerWrapper::instance()->info("DATA SIZE: %dKB,", dataSize/1024);
		XMDLoggerWrapper::instance()->info("avgLoginTime1: %dms", sumLoginTime1/timeRecordsSize);
		XMDLoggerWrapper::instance()->info("avgLoginTime2: %dms", sumLoginTime2/timeRecordsSize);
		XMDLoggerWrapper::instance()->info("avgLoginTime3: %dms", sumLoginTime3/timeRecordsSize);
		XMDLoggerWrapper::instance()->info("avgDiaCallTime: %dms", sumDialCallTime/3/timeRecordsSize);
		XMDLoggerWrapper::instance()->info("avgRecvDataTime: %dms", sumRecvDataTime/3/timeRecordsSize);
		curl_global_cleanup();
	}

	void testEfficiency(User* user1, RtsEfficiencyHandler* callEventHandler1, User* user2, RtsEfficiencyHandler* callEventHandler2, User* user3, RtsEfficiencyHandler* callEventHandler3, int idx, int dataSize, map<int, RtsPerformanceData>& timeRecords) {
		long t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0, t7 = 0, t8 = 0, t9 = 0, t10 = 0, t11 = 0, t12 = 0, t13 = 0, t14 = 0, t15 = 0;

		set<string> offlineUsers;
		offlineUsers.insert(user1->getAppAccount());
		offlineUsers.insert(user2->getAppAccount());
		offlineUsers.insert(user3->getAppAccount());

		t0 = Utils::currentTimeMillis();
		user1->login();
		user2->login();
		user3->login();
		for (int j = 0; j < TIME_OUT; j++) {
			if (offlineUsers.size() == 0) {
				break;
			}
			if (offlineUsers.count(user1->getAppAccount()) != 0 && user1->getOnlineStatus() == Online) {
				offlineUsers.erase(user1->getAppAccount());
				t1 = Utils::currentTimeMillis();
			}

			if (offlineUsers.count(user2->getAppAccount()) != 0 && user2->getOnlineStatus() == Online) {
				offlineUsers.erase(user2->getAppAccount());
				t2 = Utils::currentTimeMillis();
			}

			if (offlineUsers.count(user3->getAppAccount()) != 0 && user3->getOnlineStatus() == Online) {
				offlineUsers.erase(user3->getAppAccount());
				t3 = Utils::currentTimeMillis();
			}

			usleep(1000);
		}

		ASSERT_EQ(Online, user1->getOnlineStatus());
		ASSERT_EQ(Online, user2->getOnlineStatus());
		ASSERT_EQ(Online, user3->getOnlineStatus());

		if (t1 == 0) {
			t1 = Utils::currentTimeMillis();
			XMDLoggerWrapper::instance()->error("user1 login timeout");
		}
		if (t2 == 0) {
			t2 = Utils::currentTimeMillis();
			XMDLoggerWrapper::instance()->error("user2 login timeout");
		}
		if (t3 == 0) {
			t3 = Utils::currentTimeMillis();
			XMDLoggerWrapper::instance()->error("user3 login timeout");
		}
		callEventHandler1->clear();
		callEventHandler2->clear();
		callEventHandler3->clear();
		t4 = Utils::currentTimeMillis();
		uint64_t callId = user1->dialCall(user2->getAppAccount());
		ASSERT_NE(callId, 0);
		for (int j = 0; j < TIME_OUT; j++) {
			if (callEventHandler1->getMsgSize(2) > 0) {
				break;
			}
			usleep(1000);
		}
		t5 = Utils::currentTimeMillis();
		RtsMessageData createResponse;
		ASSERT_TRUE(callEventHandler1->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
		ASSERT_EQ(callEventHandler1->LAUNCH_OK, createResponse.getDesc());

		string sendData = Utils::generateRandomString(dataSize);
		t6 = Utils::currentTimeMillis();
		ASSERT_NE(-1, user1->sendRtsData(callId, sendData, AUDIO));
		for (int j = 0; j < TIME_OUT; j++) {
			if (callEventHandler2->getMsgSize(4) > 0) {
				break;
			}
			usleep(1000);
		}
		t7 = Utils::currentTimeMillis();
		ASSERT_NE(callEventHandler2->getMsgSize(4), 0);

		closeCall(callId, user1, callEventHandler1, callEventHandler2);

		callEventHandler1->clear();
		callEventHandler2->clear();
		callEventHandler3->clear();
		t8 = Utils::currentTimeMillis();
		callId = user1->dialCall(user3->getAppAccount());
		ASSERT_NE(callId, 0);
		for (int j = 0; j < TIME_OUT; j++) {
			if (callEventHandler1->getMsgSize(2) > 0) {
				break;
			}
			usleep(1000);
		}
		t9 = Utils::currentTimeMillis();
		ASSERT_TRUE(callEventHandler1->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
		ASSERT_EQ(callEventHandler1->LAUNCH_OK, createResponse.getDesc());

		sendData = Utils::generateRandomString(dataSize);
		t10 = Utils::currentTimeMillis();
		ASSERT_NE(-1, user1->sendRtsData(callId, sendData, AUDIO));
		for (int j = 0; j < TIME_OUT; j++) {
			if (callEventHandler3->getMsgSize(4) > 0) {
				break;
			}
			usleep(1000);
		}
		t11 = Utils::currentTimeMillis();
		ASSERT_NE(callEventHandler3->getMsgSize(4), 0);

		closeCall(callId, user1, callEventHandler1, callEventHandler3);

		callEventHandler1->clear();
		callEventHandler2->clear();
		callEventHandler3->clear();
		t12 = Utils::currentTimeMillis();
		callId = user2->dialCall(user3->getAppAccount());
		ASSERT_NE(callId, 0);
		for (int j = 0; j < TIME_OUT; j++) {
			if (callEventHandler2->getMsgSize(2) > 0) {
				break;
			}
			usleep(1000);
		}
		t13 = Utils::currentTimeMillis();
		ASSERT_TRUE(callEventHandler2->pollCreateResponse(WAIT_TIME_FOR_MESSAGE, createResponse));
		ASSERT_EQ(callEventHandler2->LAUNCH_OK, createResponse.getDesc());

		sendData = Utils::generateRandomString(dataSize);
		t14 = Utils::currentTimeMillis();
		ASSERT_NE(-1, user2->sendRtsData(callId, sendData, AUDIO));
		for (int j = 0; j < TIME_OUT; j++) {
			if (callEventHandler3->getMsgSize(4) > 0) {
				break;
			}
			usleep(1000);
		}
		t15 = Utils::currentTimeMillis();
		ASSERT_NE(callEventHandler3->getMsgSize(4), 0);

		closeCall(callId, user2, callEventHandler2, callEventHandler3);
		XMDLoggerWrapper::instance()->info("t1-t0 is %ld , t2-t0 is %ld, t3-t0 is %ld, t5-t4 is %ld, t9-t8 is %ld, t13-t12 is %ld, t7-t6 is %ld, t11-t10 is %ld, t15-t14 is %ld", t1-t0, t2-t0, t3-t0, t5-t4, t9-t8, t13-t12, t7-t6, t11-t10, t15-t14);
		timeRecords.insert(pair<int, RtsPerformanceData>(idx, RtsPerformanceData((int)(t1-t0), (int)(t2-t0), (int)(t3-t0), (int)(t5-t4), (int)(t9-t8), (int)(t13-t12), (int)(t7-t6), (int)(t11-t10), (int)(t15-t14))));
	}

	void closeCall(uint64_t callId, User* from, RtsEfficiencyHandler* callEventHandlerFrom, RtsEfficiencyHandler* callEventHandlerTo) {
		from->closeCall(callId);
		sleep(1);

		RtsMessageData byeRequest;
		ASSERT_TRUE(callEventHandlerTo->pollBye(WAIT_TIME_FOR_MESSAGE, byeRequest));
		ASSERT_EQ(callId, byeRequest.getCallId());
		ASSERT_EQ("", byeRequest.getDesc());

		RtsMessageData byeResponse;
		ASSERT_TRUE(callEventHandlerFrom->pollBye(WAIT_TIME_FOR_MESSAGE, byeResponse));
		ASSERT_EQ(callId, byeResponse.getCallId());
		ASSERT_EQ("CLOSED_INITIATIVELY", byeResponse.getDesc());
	}
protected:
	User* rtsUser1;
	User* rtsUser2;
	User* rtsUser3;

	TestTokenFetcher* tokenFetcher1;
	TestTokenFetcher* tokenFetcher2;
	TestTokenFetcher* tokenFetcher3;

	TestOnlineStatusHandler* onlineStatusHandler1;
	TestOnlineStatusHandler* onlineStatusHandler2;
	TestOnlineStatusHandler* onlineStatusHandler3;

	TestMessageHandler* msgHandler1;
	TestMessageHandler* msgHandler2;
	TestMessageHandler* msgHandler3;

	RtsEfficiencyHandler* callEventHandler1;
	RtsEfficiencyHandler* callEventHandler2;
	RtsEfficiencyHandler* callEventHandler3;
};

TEST_F(RtsEfficiency, efficiencyTest) {
	efficiencyTest();
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}