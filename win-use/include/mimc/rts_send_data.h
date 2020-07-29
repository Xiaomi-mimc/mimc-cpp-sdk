#ifndef MIMC_CPP_SDK_SEND_RTS_DATA_H
#define MIMC_CPP_SDK_SEND_RTS_DATA_H

#include <string>
#include <mimc/rts_data.pb.h>
#include <XMDCommonData.h>

class User;

class RtsSendData {
public:
    static uint64_t createRelayConn(User* user);
    static bool sendBindRelayRequest(User* user);
    static bool sendPingRelayRequest(User* user);
    static bool sendPingRelayRequestByCallId(User* user,uint64_t callId);
    static int sendRtsDataByRelay(User* user, uint64_t callId, const std::string& data, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount);
    static int sendRtsDataByRelay(User* user, uint64_t callId, const char* data, const unsigned int dataSize, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount);
    static int sendRtsDataByP2PIntranet(User* user, uint64_t callId, const std::string& data, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount);
    static int sendRtsDataByP2PIntranet(User* user, uint64_t callId, const char* data, const unsigned int dataSize, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount);
    static int sendRtsDataByP2PInternet(User* user, uint64_t callId, const std::string& data, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount);
    static int sendRtsDataByP2PInternet(User* user, uint64_t callId, const char* data, const unsigned int dataSize, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount);
    static bool closeRelayConnWhenNoCall(User* user);
private:
    static void* fetchServerAddr(void* arg);
};

#endif