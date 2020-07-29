#ifndef MIMC_CPP_TEST_RTSCALLEVENTHANDLER_H
#define MIMC_CPP_TEST_RTSCALLEVENTHANDLER_H

#include <mimc/rts_callevent_handler.h>
#include "include/ring_queue.h"
#include <XMDLoggerWrapper.h>
#include <test/rts_message_data.h>

class TestRTSCallEventHandler : public RTSCallEventHandler {
public:
    LaunchedResponse onLaunched(uint64_t callId, const std::string& fromAccount, const std::string& appContent, const std::string& fromResource) {
        XMDLoggerWrapper::instance()->info("In onLaunched, callId is %llu, fromAccount is %s, appContent is %s, fromResource is %s", callId, fromAccount.c_str(), appContent.c_str(), fromResource.c_str());
        inviteRequests.push(RtsMessageData(callId, fromAccount, appContent, fromResource));
        if (this->appContent != "" && appContent != this->appContent) {
            return LaunchedResponse(false, LAUNCH_ERR_ILLEGALAPPCONTENT);
        }
        XMDLoggerWrapper::instance()->info("In onLaunched, appContent is equal to this->appContent");
        return LaunchedResponse(true, LAUNCH_OK);
    }

    void onAnswered(uint64_t callId, bool accepted, const std::string& desc) {
        XMDLoggerWrapper::instance()->info("In onAnswered, callId is %llu, accepted is %d, desc is %s", callId, accepted, desc.c_str());
        createResponses.push(RtsMessageData(callId, desc, accepted));
        XMDLoggerWrapper::instance()->info("In onAnswered, callId is %llu, accepted is %d, desc is %s push finished", callId, accepted, desc.c_str());
    }

    void onClosed(uint64_t callId, const std::string& desc) {
        XMDLoggerWrapper::instance()->info("In onClosed, callId is %llu, desc is %s", callId, desc.c_str());
        byes.push(RtsMessageData(callId, desc));
    }

    void onData(uint64_t callId, const std::string& fromAccount, const std::string& resource, const std::string& data, RtsDataType dataType, RtsChannelType channelType) {
        XMDLoggerWrapper::instance()->info("In onData, callId is %llu, fromAccount is %s, resource is %s, dataLen is %d, data is %s, dataType is %d", callId, fromAccount.c_str(), resource.c_str(), data.length(), data.c_str(), dataType);
        recvDatas.push(RtsMessageData(callId, data, dataType, channelType));
    }

    void onSendDataSuccess(uint64_t callId, int dataId, const std::string& ctx) {
        XMDLoggerWrapper::instance()->info("In onSendDataSuccess, callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
    }

    void onSendDataFailure(uint64_t callId, int dataId, const std::string& ctx) {
        XMDLoggerWrapper::instance()->warn("In onSendDataFailure, callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
    }

    const std::string& getAppContent() {return this->appContent;}

    bool pollInviteRequest(long timeout_s, RtsMessageData& inviteRequest) {
        return inviteRequests.pop(timeout_s, inviteRequest);
    }

    bool pollCreateResponse(long timeout_s, RtsMessageData& createResponse) {
        return createResponses.pop(timeout_s, createResponse);
    }

    bool pollBye(long timeout_s, RtsMessageData& bye) {
        return byes.pop(timeout_s, bye);
    }

    bool pollData(long timeout_s, RtsMessageData& recvData) {
        return recvDatas.pop(timeout_s, recvData);
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