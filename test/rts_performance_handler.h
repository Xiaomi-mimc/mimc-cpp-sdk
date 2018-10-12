#ifndef MIMC_CPP_TEST_RTSPERFORMANCE_HANDLER_H
#define MIMC_CPP_TEST_RTSPERFORMANCE_HANDLER_H

#include <map>
#include <mimc/rts_callevent_handler.h>
#include <mimc/threadsafe_queue.h>
#include <LoggerWrapper.h>
#include "rts_message_data.h"
#include "rts_performance_data.h"

using namespace std;

class RtsPerformanceHandler : public RTSCallEventHandler {
public:
    LaunchedResponse onLaunched(string fromAccount, string fromResource, long chatId, const string& appContent) {
        LoggerWrapper::instance()->info("In onLaunched, chatId is %ld, fromAccount is %s, fromResource is %s, appContent is %s", chatId, fromAccount.c_str(), fromResource.c_str(), appContent.c_str());
        inviteRequests.push(RtsMessageData(fromAccount, fromResource, chatId, appContent));
        return LaunchedResponse(true, LAUNCH_OK);
    }

    void onAnswered(long chatId, bool accepted, const string& errmsg) {
        LoggerWrapper::instance()->info("In onAnswered, chatId is %ld, accepted is %d, errmsg is %s", chatId, accepted, errmsg.c_str());
        createResponses.push(RtsMessageData(chatId, errmsg, accepted));
    }

    void onClosed(long chatId, const string& errmsg) {
        LoggerWrapper::instance()->info("In onClosed, chatId is %ld, errmsg is %s", chatId, errmsg.c_str());
        byes.push(RtsMessageData(chatId, errmsg));
    }

    void handleData(long chatId, const string& data, RtsDataType dataType, RtsChannelType channelType) {
        LoggerWrapper::instance()->info("In handleData, chatId is %ld, dataLen is %d, data is %s, dataType is %d", chatId, data.length(), data.c_str(), dataType);
        int dataId = 0;
        recvDatas.insert(pair<int, RtsPerformanceData>(dataId, RtsPerformanceData(data, time(NULL))));
    }

    RtsMessageData* pollInviteRequest(long timeout_s) {
        RtsMessageData* inviteRequestPtr;
        inviteRequests.pop(timeout_s, &inviteRequestPtr);
        return inviteRequestPtr;
    }

    RtsMessageData* pollCreateResponse(long timeout_s) {
        RtsMessageData* createResponsePtr;
        createResponses.pop(timeout_s, &createResponsePtr);
        return createResponsePtr;
    }

    RtsMessageData* pollBye(long timeout_s) {
        RtsMessageData* byePtr;
        byes.pop(timeout_s, &byePtr);
        return byePtr;
    }

    const map<int, RtsPerformanceData>& pollData() {
        return recvDatas;
    }

    int getMsgSize(int msgType) {
    	int size = 0;

    	switch(msgType) {
    		case 1:
    			size = inviteRequests.size();
    			break;
    		case 2:
    			size = createResponses.size();
    			break;
    		case 3:
    			size = byes.size();
    			break;
    		case 4:
    			size = recvDatas.size();
    			break;
    	}

        return size;
    }

    void clear() {
        inviteRequests.clear();
        createResponses.clear();
        byes.clear();
        recvDatas.clear();
    }
public:
    const string LAUNCH_OK = "OK";
private:
    ThreadSafeQueue<RtsMessageData> inviteRequests;
    ThreadSafeQueue<RtsMessageData> createResponses;
    ThreadSafeQueue<RtsMessageData> byes;
    map<int, RtsPerformanceData> recvDatas;
};

#endif