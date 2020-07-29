#ifndef MIMC_CPP_SDK_MESSAGEHANDLER_H
#define MIMC_CPP_SDK_MESSAGEHANDLER_H

#include <mimc/mimcmessage.h>
#include <mimc/mimc_group_message.h>
#include "mimc_server_ack.h"
#include <time.h>
#include <vector>

class MessageHandler {
public:
	virtual void handleOnlineMessage(const MIMCMessage& packets) = 0;
	virtual bool handleMessage(const std::vector<MIMCMessage>& packets) = 0;
	virtual bool handleGroupMessage(const std::vector<MIMCGroupMessage>& packets) = 0;
	virtual void handleServerAck(const MIMCServerAck &serverAck) = 0;
	virtual void handleSendMsgTimeout(const MIMCMessage& message) = 0;
	virtual void handleSendGroupMsgTimeout(const MIMCGroupMessage& groupMessage) = 0;
	virtual bool onPullNotification() = 0;
	virtual ~MessageHandler() {}
};

#endif