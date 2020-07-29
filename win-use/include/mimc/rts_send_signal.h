#ifndef MIMC_CPP_SDK_RTS_SEND_SIGNAL_H
#define MIMC_CPP_SDK_RTS_SEND_SIGNAL_H

#include <string>
#include <mimc/rts_signal.pb.h>
#include <mimc/constant.h>
#include <stdint.h>
#include "mimc/p2p_callsession.h"

class User;

class RtsSendSignal {
public:
	static bool sendCreateRequest(User* user, uint64_t callId,P2PCallSession& callSession);
	static bool sendInviteResponse(const User* user, uint64_t callId, mimc::CallType callType, mimc::RTSResult result, std::string errMsg);
	static bool sendByeRequest(const User* user, uint64_t callId, std::string byeReason,const P2PCallSession &callSession);
	static bool sendByeResponse(const User* user, uint64_t callId, mimc::RTSResult result,const P2PCallSession &callSession);
	static bool sendUpdateRequest(User* user, uint64_t callId,P2PCallSession& callSession);
	static bool sendUpdateResponse(const User* user, uint64_t callId, mimc::RTSResult result);
	static bool pingCallCenter(const User* user, uint64_t callId,const P2PCallSession &callSession);
	static std::string sendRtsMessage(const User* user, uint64_t callId, mimc::RTSMessageType messageType, mimc::CallType callType, std::string payload);
	static void sendExceptInviteResponse(User* user, mimc::RTSResult result, std::string errMsg);
private:

};

#endif