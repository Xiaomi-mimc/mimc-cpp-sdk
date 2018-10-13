#ifndef MIMC_CPP_TEST_MESSAGE_HANDLER_H
#define MIMC_CPP_TEST_MESSAGE_HANDLER_H

#include <mimc/message_handler.h>
#include <mimc/threadsafe_queue.h>

class TestMessageHandler : public MessageHandler {
public:
    void handleMessage(std::vector<MIMCMessage> packets) {
        std::vector<MIMCMessage>::iterator it = packets.begin();
        for (; it != packets.end(); ++it) {
            messages.push(*it);
        }
    }

    void handleServerAck(std::string packetId, long sequence, long timestamp, std::string errorMsg) {
        packetIds.push(packetId);
    }

    void handleSendMsgTimeout(MIMCMessage message) {

    }

    MIMCMessage* pollMessage() {
        MIMCMessage *messagePtr;
        messages.pop(&messagePtr);
        return messagePtr;
    }

    std::string* pollServerAck() {
        std::string *packetIdPtr;
        packetIds.pop(&packetIdPtr);
        return packetIdPtr;
    }
private:
    ThreadSafeQueue<MIMCMessage> messages;
    ThreadSafeQueue<std::string> packetIds;
};

#endif