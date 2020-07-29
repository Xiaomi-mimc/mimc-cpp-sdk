#ifndef MIMC_CPP_SDK_MIMCGROUPMESSAGE_H
#define MIMC_CPP_SDK_MIMCGROUPMESSAGE_H

#include <string>
#include <time.h>

class MIMCGroupMessage {

public:
	MIMCGroupMessage() {}

	MIMCGroupMessage(const std::string &packetId, int64_t sequence, const std::string &fromAccount, const std::string &fromResource, uint64_t topicId, const std::string &payload, const std::string &bizType, time_t timestamp, int64_t convIndex = 0)
	{
		this->packetId = packetId;
		this->sequence = sequence;
		this->fromAccount = fromAccount;
		this->fromResource = fromResource;
		this->topicId = topicId;
		this->payload = payload;
		this->bizType = bizType;
		this->timestamp = timestamp;
		this->convIndex = convIndex;
	}
	std::string getPacketId() const { return this->packetId; }
	int64_t getSequence() const { return this->sequence; }
	std::string getFromAccount() const { return this->fromAccount; }
	std::string getFromResource() const { return this->fromResource; }
	std::string getPayload() const { return this->payload; }
	std::string getBizType() const { return this->bizType; }
	time_t getTimeStamp() const { return this->timestamp; }
	uint64_t getTopicId() const { return this->topicId; }
	static bool sortBySequence(const MIMCGroupMessage &m1, const MIMCGroupMessage &m2) {
		return m1.sequence < m2.sequence;
	}
	int64_t getConvIndex() const { return convIndex; }

private:
	std::string packetId;
	int64_t sequence;
	time_t timestamp;
	std::string fromAccount;
	std::string fromResource;
	uint64_t topicId;
	std::string payload;
	std::string bizType;
	int64_t convIndex;
};
#endif