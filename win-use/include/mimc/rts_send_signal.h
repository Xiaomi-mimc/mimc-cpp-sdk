#ifndef MIMC_CPP_SDK_RTS_SEND_SIGNAL_H
#define MIMC_CPP_SDK_RTS_SEND_SIGNAL_H

#include <string>
#include <mimc/rts_signal.pb.h>
#include <mimc/constant.h>
#include <stdint.h>

class User;

class RtsSendSignal {
public:
	static bool sendCreateRequest(const User* user, uint64_t callId);
	static bool sendInviteResponse(const User* user, uint64_t callId, mimc::CallType callType, mimc::RTSResult result, std::string errMsg);
	static bool sendByeRequest(const User* user, uint64_t callId, std::string byeReason);
	static bool sendByeResponse(const User* user, uint64_t callId, mimc::RTSResult result);
	static bool sendUpdateRequest(const User* user, uint64_t callId);
	static bool sendUpdateResponse(const User* user, uint64_t callId, mimc::RTSResult result);
	static bool pingCallCenter(const User* user, uint64_t callId);
	static std::string sendRtsMessage(const User* user, uint64_t callId, mimc::RTSMessageType messageType, mimc::CallType callType, std::string payload);
private:

};

#endif