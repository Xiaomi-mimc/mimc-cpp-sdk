#ifndef MIMC_CPP_SDK_RTS_SEND_SIGNAL_H
#define MIMC_CPP_SDK_RTS_SEND_SIGNAL_H

#include <string>
#include <mimc/rts_signal.pb.h>
#include <mimc/constant.h>
#include <stdint.h>

class User;

class RtsSendSignal {
public:
	static bool sendCreateRequest(const User* user, uint64_t chatId);
	static bool sendInviteResponse(const User* user, uint64_t chatId, mimc::ChatType chatType, mimc::RTSResult result, std::string errmsg);
	static bool sendByeRequest(const User* user, uint64_t chatId, std::string byeReason);
	static bool sendByeResponse(const User* user, uint64_t chatId, mimc::RTSResult result);
	static bool sendUpdateRequest(const User* user, uint64_t chatId);
	static bool sendUpdateResponse(const User* user, uint64_t chatId, mimc::RTSResult result);
	static bool pingChatManager(const User* user, uint64_t chatId);
	static std::string sendRtsMessage(const User* user, uint64_t chatId, mimc::RTSMessageType messageType, mimc::ChatType chatType, std::string payload);
private:

};

#endif