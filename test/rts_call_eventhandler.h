#ifndef MIMC_CPP_TEST_RTSCALLEVENTHANDLER_H
#define MIMC_CPP_TEST_RTSCALLEVENTHANDLER_H

#include <mimc/rts_callevent_handler.h>
#include <mimc/threadsafe_queue.h>
#include <XMDLoggerWrapper.h>
#include <test/rts_message_data.h>

class TestRTSCallEventHandler : public RTSCallEventHandler {
public:
    LaunchedResponse onLaunched(uint64_t callId, const std::string fromAccount, const std::string appContent, const std::string fromResource) {
        XMDLoggerWrapper::instance()->info("In onLaunched, callId is %llu, fromAccount is %s, appContent is %s, fromResource is %s", callId, fromAccount.c_str(), appContent.c_str(), fromResource.c_str());
        inviteRequests.push(RtsMessageData(callId, fromAccount, appContent, fromResource));
        if (this->appContent != "" && appContent != this->appContent) {
            return LaunchedResponse(false, LAUNCH_ERR_ILLEGALAPPCONTENT);
        }
        XMDLoggerWrapper::instance()->info("In onLaunched, appContent is equal to this->appContent");
        return LaunchedResponse(true, LAUNCH_OK);
    }

    void onAnswered(uint64_t callId, bool accepted, const std::string desc) {
        XMDLoggerWrapper::instance()->info("In onAnswered, callId is %llu, accepted is %d, desc is %s", callId, accepted, desc.c_str());
        createResponses.push(RtsMessageData(callId, desc, accepted));
    }

    void onClosed(uint64_t callId, const std::string desc) {
        XMDLoggerWrapper::instance()->info("In onClosed, callId is %llu, desc is %s", callId, desc.c_str());
        byes.push(RtsMessageData(callId, desc));
    }

    void handleData(uint64_t callId, const std::string data, RtsDataType dataType, RtsChannelType channelType) {
        XMDLoggerWrapper::instance()->info("In handleData, callId is %llu, dataLen is %d, data is %s, dataType is %d", callId, data.length(), data.c_str(), dataType);
        recvDatas.push(RtsMessageData(callId, data, dataType, channelType));
    }

    void handleSendDataSucc(uint64_t callId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->info("In handleSendDataSucc, callId is %llu, groupId is %d, ctx is %s", callId, groupId, ctx.c_str());
    }

    void handleSendDataFail(uint64_t callId, int groupId, const std::string ctx) {
        XMDLoggerWrapper::instance()->warn("In handleSendDataFail, callId is %llu, groupId is %d, ctx is %s", callId, groupId, ctx.c_str());
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