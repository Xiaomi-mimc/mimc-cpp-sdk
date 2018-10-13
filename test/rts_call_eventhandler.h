#ifndef MIMC_CPP_TEST_RTSCALLEVENTHANDLER_H
#define MIMC_CPP_TEST_RTSCALLEVENTHANDLER_H

#include <mimc/rts_callevent_handler.h>
#include <mimc/threadsafe_queue.h>
#include <LoggerWrapper.h>
#include <test/rts_message_data.h>

class TestRTSCallEventHandler : public RTSCallEventHandler {
public:
    LaunchedResponse onLaunched(std::string fromAccount, std::string fromResource, long chatId, const std::string& appContent) {
        LoggerWrapper::instance()->info("In onLaunched, chatId is %ld, fromAccount is %s, fromResource is %s, appContent is %s", chatId, fromAccount.c_str(), fromResource.c_str(), appContent.c_str());
        inviteRequests.push(RtsMessageData(fromAccount, fromResource, chatId, appContent));
        if (this->appContent != "" && appContent != this->appContent) {
            return LaunchedResponse(false, LAUNCH_ERR_ILLEGALAPPCONTENT);
        }
        LoggerWrapper::instance()->info("In onLaunched, appContent is equal to this->appContent");
        return LaunchedResponse(true, LAUNCH_OK);
    }

    void onAnswered(long chatId, bool accepted, const std::string& errmsg) {
        LoggerWrapper::instance()->info("In onAnswered, chatId is %ld, accepted is %d, errmsg is %s", chatId, accepted, errmsg.c_str());
        createResponses.push(RtsMessageData(chatId, errmsg, accepted));
    }

    void onClosed(long chatId, const std::string& errmsg) {
        LoggerWrapper::instance()->info("In onClosed, chatId is %ld, errmsg is %s", chatId, errmsg.c_str());
        byes.push(RtsMessageData(chatId, errmsg));
    }

    void handleData(long chatId, const std::string& data, RtsDataType dataType, RtsChannelType channelType) {
        LoggerWrapper::instance()->info("In handleData, chatId is %ld, dataLen is %d, data is %s, dataType is %d", chatId, data.length(), data.c_str(), dataType);
        recvDatas.push(RtsMessageData(chatId, data, dataType, channelType));
    }

    const std::string& getAppContent() {return this->appContent;}

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

    RtsMessageData* pollData(long timeout_s) {
        RtsMessageData* recvDataPtr;
        recvDatas.pop(timeout_s, &recvDataPtr);
        return recvDataPtr;
    }

    int getDataSize() {
        return recvDatas.size();
    }

    void clear() {
        inviteRequests.clear();
        createResponses.clear();
        byes.clear();
        recvDatas.clear();
    }

    TestRTSCallEventHandler(std::string appContent) {
        this->appContent = appContent;
    }
    TestRTSCallEventHandler() {}
public:
    const std::string LAUNCH_OK = "OK";
    const std::string LAUNCH_ERR_ILLEGALAPPCONTENT = "ILLEGALAPPCONTENT";
private:
    std::string appContent;
    ThreadSafeQueue<RtsMessageData> inviteRequests;
    ThreadSafeQueue<RtsMessageData> createResponses;
    ThreadSafeQueue<RtsMessageData> byes;
    ThreadSafeQueue<RtsMessageData> recvDatas;
};

#endif