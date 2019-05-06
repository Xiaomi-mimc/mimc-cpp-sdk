#ifndef MIMC_CPP_TEST_RTSEFFICIENCY_HANDLER_H
#define MIMC_CPP_TEST_RTSEFFICIENCY_HANDLER_H

#include <map>
#include <mimc/utils.h>
#include <mimc/rts_callevent_handler.h>
#include <mimc/threadsafe_queue.h>
#include <XMDLoggerWrapper.h>
#include <test/rts_message_data.h>
#include <test/rts_performance_data.h>

using namespace std;

class RtsEfficiencyHandler : public RTSCallEventHandler {
public:
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
        int dataId = char2int((unsigned char*)data.c_str(), 0);
        recvDatas.insert(pair<int, RtsPerformanceData>(dataId, RtsPerformanceData(data, Utils::currentTimeMillis())));
    }

    void onSendDataSuccess(uint64_t callId, int dataId, const std::string ctx) {
        XMDLoggerWrapper::instance()->info("In onSendDataSuccess, callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
    }

    void onSendDataFailure(uint64_t callId, int dataId, const std::string ctx) {
        XMDLoggerWrapper::instance()->warn("In onSendDataFailure, callId is %llu, dataId is %d, ctx is %s", callId, dataId, ctx.c_str());
    }

    bool pollInviteRequest(long timeout_s, RtsMessageData& inviteRequest) {
        return inviteRequests.pop(timeout_s, inviteRequest);
    }

    bool pollCreateResponse(long timeout_s, RtsMessageData& createResponse) {
        return createResponses.pop(timeout_s, createResponse);
    }

    bool pollBye(long timeout_s, RtsMessageData& bye) {
        return byes.pop(timeout_s, bye);
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