#ifndef MIMC_CPP_TEST_RTSPERFORMANCE_HANDLER_H
#define MIMC_CPP_TEST_RTSPERFORMANCE_HANDLER_H

#include <map>
#include <mimc/utils.h>
#include <mimc/rts_callevent_handler.h>
#include <mimc/threadsafe_queue.h>
#include <XMDLoggerWrapper.h>
#include <test/rts_message_data.h>
#include <test/rts_performance_data.h>

using namespace std;

class RtsPerformanceHandler : public RTSCallEventHandler {
public:
    LaunchedResponse onLaunched(uint64_t chatId, const std::string fromAccount, const std::string appContent, const std::string fromResource) {
        XMDLoggerWrapper::instance()->info("In onLaunched, chatId is %llu, fromAccount is %s, appContent is %s, fromResource is %s", chatId, fromAccount.c_str(), appContent.c_str(), fromResource.c_str());
        inviteRequests.push(RtsMessageData(chatId, fromAccount, appContent, fromResource));
        return LaunchedResponse(true, LAUNCH_OK);
    }

    void onAnswered(uint64_t chatId, bool accepted, const string errmsg) {
        XMDLoggerWrapper::instance()->info("In onAnswered, chatId is %llu, accepted is %d, errmsg is %s", chatId, accepted, errmsg.c_str());
        createResponses.push(RtsMessageData(chatId, errmsg, accepted));
    }

    void onClosed(uint64_t chatId, const string errmsg) {
        XMDLoggerWrapper::instance()->info("In onClosed, chatId is %llu, errmsg is %s", chatId, errmsg.c_str());
        byes.push(RtsMessageData(chatId, errmsg));
    }

    void handleData(uint64_t chatId, const string data, RtsDataType dataType, RtsChannelType channelType) {
        XMDLoggerWrapper::instance()->info("In handleData, chatId is %llu, dataLen is %d, data is %s, dataType is %d", chatId, data.length(), data.c_str(), dataType);
        int dataId = char2int((unsigned char*)data.c_str(), 0);
        recvDatas.insert(pair<int, RtsPerformanceData>(dataId, RtsPerformanceData(data, Utils::currentTimeMillis())));
    }

    void handleSendDataSucc(uint64_t chatId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->info("In handleSendDataSucc, chatId is %llu, groupId is %d, ctx is %s", chatId, groupId, ctx.c_str());
    }

    void handleSendDataFail(uint64_t chatId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->warn("In handleSendDataFail, chatId is %llu, groupId is %d, ctx is %s", chatId, groupId, ctx.c_str());
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

    const map<int, RtsPerformanceData>& pollDataInfo() {
        return recvDatas;
    }

    int getMsgSize(int msgType) {
        int size = 0;

        switch (msgType) {
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
private:
    int char2int(const unsigned char* result, int index) {
        unsigned char fourthByte = result[index];
        unsigned char thirdByte = result[index + 1];
        unsigned char secondByte = result[index + 2];
        unsigned char firstByte = result[index + 3];
        int ret = (fourthByte << 24) | (thirdByte << 16) | (secondByte << 8) | firstByte;
        return ret;
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