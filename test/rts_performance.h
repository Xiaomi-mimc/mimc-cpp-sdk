#ifndef MIMC_CPP_TEST_RTSPERFORMANCE_H
#define MIMC_CPP_TEST_RTSPERFORMANCE_H

#include <gtest/gtest.h>
#include <mimc/user.h>
#include <mimc/utils.h>
#include <test/mimc_tokenfetcher.h>
#include <test/mimc_onlinestatus_handler.h>
#include <test/mimc_message_handler.h>

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
string appAccount1 = "perf_MI153";
string appAccount2 = "perf_MI108";
const int WAIT_TIME_FOR_MESSAGE = 1;

class RtsPerformanceHandler;

class RtsPerformance: public testing::Test {
protected:
	void SetUp();

	void TearDown();

	//@test
	void performanceTest();

	void testPerformance(User* user1, RtsPerformanceHandler* callEventHandler1, User* user2, RtsPerformanceHandler* callEventHandler2, int dataSizeKB, int dataSpeedKB, int durationSec);

	void logIn(User* user, RtsPerformanceHandler* callEventHandler);

	void createCall(uint64_t& callId, User* from, RtsPerformanceHandler* callEventHandlerFrom, User* to, RtsPerformanceHandler* callEventHandlerTo, const string& appContent = "");

	void closeCall(uint64_t callId, User* from, RtsPerformanceHandler* callEventHandlerFrom, RtsPerformanceHandler* callEventHandlerTo);

	void long2char(int64_t data, unsigned char* result, int index) {
		for (int i = index + 7; i >= index; i--) {
			result[i] = data & 0xFF;
			data >>= 8;
		}
	}

	int64_t char2long(const unsigned char* input, int index) {
		int64_t result = 0;
		for (int i = index; i < index + 8; i++) {
			result <<= 8;
			result |= (input[i] & 0xFF);
		}
		return result;
	}

public:
	void checkRecvDataTime(string recvData, int64_t recvTime) {
        long t = recvTime - char2long((unsigned char*)recvData.c_str(), 0);
        if (t <= 50) {
            time0To50++;
        } else if (t <= 100) {
            time51To100++;
        } else if (t <= 150) {
            time101To150++;
        } else if (t <= 200) {
            time151To200++;
        } else if (t <= 300) {
            time201To300++;
        } else if (t <= 400) {
            time301To400++;
        } else if (t <= 500) {
            time401To500++;
        } else if (t <= 1000) {
            time501To1000++;
        } else {
            time1001More++;
        }
    }

protected:
	User* rtsUser1;
	User* rtsUser2;

	TestTokenFetcher* tokenFetcher1;
	TestTokenFetcher* tokenFetcher2;

	TestOnlineStatusHandler* onlineStatusHandler1;
	TestOnlineStatusHandler* onlineStatusHandler2;

	TestMessageHandler* msgHandler1;
	TestMessageHandler* msgHandler2;

	RtsPerformanceHandler* callEventHandler1;
	RtsPerformanceHandler* callEventHandler2;

	int sendFailed;
	int lost;
	int dataError;
	int time0To50;
	int time51To100;
	int time101To150;
	int time151To200;
	int time201To300;
	int time301To400;
	int time401To500;
	int time501To1000;
	int time1001More;
};

#endif
