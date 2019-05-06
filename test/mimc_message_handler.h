#ifndef MIMC_CPP_TEST_MESSAGE_HANDLER_H
#define MIMC_CPP_TEST_MESSAGE_HANDLER_H

#include <mimc/message_handler.h>
#include <mimc/threadsafe_queue.h>
#include <mimc/mimc_group_message.h>

class TestMessageHandler : public MessageHandler {
public:
    void handleMessage(std::vector<MIMCMessage> packets) {
        std::vector<MIMCMessage>::iterator it = packets.begin();
        for (; it != packets.end(); ++it) {
            messages.push(*it);
            MIMCMessage& message = *it;
            printf("recv message, payload is %s, bizType is %s\n", message.getPayload().c_str(), message.getBizType().c_str());
        }
    }

	void handleGroupMessage(std::vector<MIMCGroupMessage> packets) {
		std::vector<MIMCGroupMessage>::iterator it = packets.begin();
		for (; it != packets.end(); ++it) {
			groupMessages.push(*it);
			MIMCGroupMessage& groupMessage = *it;
			printf("recv group message, payload is %s, bizType is %s\n", groupMessage.getPayload().c_str(), groupMessage.getBizType().c_str());
		}
	}
    void handleServerAck(std::string packetId, int64_t sequence, time_t timestamp, std::string desc) {
        packetIds.push(packetId);
    }

    void handleSendMsgTimeout(MIMCMessage message) {

    }

	void handleSendGroupMsgTimeout(MIMCGroupMessage groupMessage) {
	
	}

    bool pollMessage(MIMCMessage& message) {
        return messages.pop(message);
    }

	bool pollGroupMessage(MIMCGroupMessage& groupMessage) {
		return groupMessages.pop(groupMessage);
	}

    bool pollServerAck(std::string& packetId) {
        return packetIds.pop(packetId);
    }
private:
    ThreadSafeQueue<MIMCMessage> messages;
	ThreadSafeQueue<MIMCGroupMessage> groupMessages;
    ThreadSafeQueue<std::string> packetIds;
};

#endif