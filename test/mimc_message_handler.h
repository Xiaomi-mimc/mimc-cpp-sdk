#ifndef MIMC_CPP_TEST_MESSAGE_HANDLER_H
#define MIMC_CPP_TEST_MESSAGE_HANDLER_H

#include "mimc/message_handler.h"
#include "include/ring_queue.h"
#include "mimc/mimc_group_message.h"

class TestMessageHandler : public MessageHandler {
public:
    bool handleMessage(const std::vector<MIMCMessage>& packets) {
        std::vector<MIMCMessage>::const_iterator it = packets.begin();
        for (; it != packets.end(); ++it) {
            messages.push(*it);
            const MIMCMessage& message = *it;
            printf("recv message, payload is %s, bizType is %s\n", message.getPayload().c_str(), message.getBizType().c_str());
        }

        return true;
    }
    void handleOnlineMessage(const MIMCMessage& packets) {
        messages.push(packets);
        printf("recv online message, payload is %s, bizType is %s\n", packets.getPayload().c_str(), packets.getBizType().c_str());
    }

	bool handleGroupMessage(const std::vector<MIMCGroupMessage>& packets) {
		std::vector<MIMCGroupMessage>::const_iterator it = packets.begin();
		for (; it != packets.end(); ++it) {
			groupMessages.push(*it);
			const MIMCGroupMessage& groupMessage = *it;
			printf("recv group message, payload is %s, bizType is %s\n", groupMessage.getPayload().c_str(), groupMessage.getBizType().c_str());
		}

        return true;
    }
    void handleServerAck(const MIMCServerAck &serverAck) {
        packetIds.push(serverAck.getPacketId());
    }

    void handleSendMsgTimeout(const MIMCMessage& message) {

    }

	void handleSendGroupMsgTimeout(const MIMCGroupMessage& groupMessage) {
	
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

    bool onPullNotification() {
        return true;
    }
private:
    ThreadSafeQueue<MIMCMessage> messages;
	ThreadSafeQueue<MIMCGroupMessage> groupMessages;
    ThreadSafeQueue<std::string> packetIds;
};

#endif