#ifndef MIMC_CPP_SDK_MESSAGEHANDLER_H
#define MIMC_CPP_SDK_MESSAGEHANDLER_H

#include <mimc/mimcmessage.h>
#include <vector>

class MessageHandler {
public:
	virtual void handleMessage(std::vector<MIMCMessage> packets) = 0;
	virtual void handleServerAck(std::string packetId, long sequence, long timestamp, std::string errorMsg) = 0;
	virtual void handleSendMsgTimeout(MIMCMessage message) = 0;
	virtual ~MessageHandler() {}
};

#endif