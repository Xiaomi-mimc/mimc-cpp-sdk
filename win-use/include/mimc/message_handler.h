#ifndef MIMC_CPP_SDK_MESSAGEHANDLER_H
#define MIMC_CPP_SDK_MESSAGEHANDLER_H

#include <mimc/mimcmessage.h>
#include <mimc/mimc_group_message.h>
#include <vector>

class MessageHandler {
public:
	virtual void handleMessage(std::vector<MIMCMessage> packets) = 0;
	virtual void handleGroupMessage(std::vector<MIMCGroupMessage> packets) = 0;
	virtual void handleServerAck(std::string packetId, int64_t sequence, time_t timestamp, std::string desc) = 0;
	virtual void handleSendMsgTimeout(MIMCMessage message) = 0;
	virtual void handleSendGroupMsgTimeout(MIMCGroupMessage groupMessage) = 0;
	virtual ~MessageHandler() {}
};

#endif