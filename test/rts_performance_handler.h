#ifndef MIMC_CPP_TEST_RTSPERFORMANCE_HANDLER_H
#define MIMC_CPP_TEST_RTSPERFORMANCE_HANDLER_H

#include <map>
#include <mimc/utils.h>
#include <mimc/rts_callevent_handler.h>
#include <mimc/threadsafe_queue.h>
#include <XMDLoggerWrapper.h>
#include <test/rts_message_data.h>
#include <test/rts_performance.h>
#include <test/rts_performance_data.h>

using namespace std;

class RtsPerformanceHandler : public RTSCallEventHandler {
public:
    RtsPerformanceHandler(RtsPerformance* rtsPerformance) {
        this->rtsPerformance = rtsPerformance;
    }

    LaunchedResponse onLaunched(uint64_t callId, const std::string fromAccount, const std::string appContent, const std::string fromResource) {
        XMDLoggerWrapper::instance()->info("In onLaunched, callId is %llu, fromAccount is %s, appContent is %s, fromResource is %s", callId, fromAccount.c_str(), appContent.c_str(), fromResource.c_str());
        inviteRequests.push(RtsMessageData(callId, fromAccount, appContent, fromResource));
        return LaunchedResponse(true, LAUNCH_OK);
    }

    void onAnswered(uint64_t callId, bool accepted, const string desc) {
        XMDLoggerWrapper::instance()->info("In onAnswered, callId is %llu, accepted is %d, desc is %s", callId, accepted, desc.c_str());
        createResponses.push(RtsMessageData(callId, desc, accepted));
    }

    void onClosed(uint64_t callId, const string desc) {
        XMDLoggerWrapper::instance()->info("In onClosed, callId is %llu, desc is %s", callId, desc.c_str());
        byes.push(RtsMessageData(callId, desc));
    }

    void onData(uint64_t callId, const std::string fromAccount, const std::string resource, const string data, RtsDataType dataType, RtsChannelType channelType) {
        XMDLoggerWrapper::instance()->info("In onData, callId is %llu, fromAccount is %s, resource is %s, dataLen is %d, data is %s, dataType is %d", callId, fromAccount.c_str(), resource.c_str(), data.length(), data.c_str(), dataType);
        rtsPerformance->checkRecvDataTime(data, Utils::currentTimeMillis());
    }

    void onSendDataSuccess(uint64_t callId, int dataId, const std::string ctx) {
        XMDLoggerWrapper::instance()->info("In onSendDataSuccess, callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
    }

    void onSendDataFailure(uint64_t callId, int dataId, const std::string ctx) {
        XMDLoggerWrapper::instance()->warn("In onSendDataFailure, callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
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

    void clear() {
        inviteRequests.clear();
        createResponses.clear();
        byes.clear();
    }
public:
    const string LAUNCH_OK = "OK";
private:
    ThreadSafeQueue<RtsMessageData> inviteRequests;
    ThreadSafeQueue<RtsMessageData> createResponses;
    ThreadSafeQueue<RtsMessageData> byes;
    RtsPerformance* rtsPerformance;
};

#endif