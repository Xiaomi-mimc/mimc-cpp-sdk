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
            MIMCMessage& message = *it;
            printf("recv message, payload is %s, bizType is %s\n", message.getPayload().c_str(), message.getBizType().c_str());
        }
    }

    void handleServerAck(std::string packetId, int64_t sequence, time_t timestamp, std::string errMsg) {
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