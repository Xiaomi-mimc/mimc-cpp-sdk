#ifndef MIMC_CPP_SDK_RTS_SEND_SIGNAL_H
#define MIMC_CPP_SDK_RTS_SEND_SIGNAL_H

#include <string>
#include <mimc/rts_signal.pb.h>
#include <mimc/constant.h>

class User;

class RtsSendSignal {
public:
	static bool sendCreateRequest(const User* user, long chatId);
	static bool sendInviteResponse(const User* user, long chatId, mimc::ChatType chatType, mimc::RTSResult result, std::string errmsg);
	static bool sendByeRequest(const User* user, long chatId, std::string byeReason);
	static bool sendByeResponse(const User* user, long chatId, mimc::RTSResult result);
	static bool sendUpdateRequest(const User* user, long chatId);
	static bool sendUpdateResponse(const User* user, long chatId, mimc::RTSResult result);
	static bool pingChatManager(const User* user, long chatId);
	static std::string sendRtsMessage(const User* user, long chatId, mimc::RTSMessageType messageType, mimc::ChatType chatType, std::string payload);
private:

};

#endif