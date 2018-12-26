#ifndef MIMC_CPP_SDK_MIMCMESSAGE_H
#define MIMC_CPP_SDK_MIMCMESSAGE_H

#include <string>

class MIMCMessage {
public:
	MIMCMessage(){}
	MIMCMessage(const std::string& packetId, int64_t sequence, const std::string& fromAccount, const std::string& fromResource, const std::string& toAccount, const std::string& toResource, const std::string& payload, const std::string& bizType, time_t timestamp) {
		this->packetId = packetId;
		this->sequence = sequence;
		this->fromAccount = fromAccount;
		this->fromResource = fromResource;
		this->toAccount = toAccount;
		this->toResource = toResource;
		this->payload = payload;
		this->bizType = bizType;
		this->timestamp = timestamp;
	}
	std::string getPacketId() const { return this->packetId; }
	int64_t getSequence() const { return this->sequence; }
	std::string getFromAccount() const { return this->fromAccount; }
	std::string getFromResource() const { return this->fromResource; }
	std::string getToAccount() const { return this->toAccount; }
	std::string getToResource() const { return this->toResource; }
	std::string getPayload() const { return this->payload; }
	std::string getBizType() const { return this->bizType; }
	time_t getTimeStamp() const { return this->timestamp; }
	static bool sortBySequence(const MIMCMessage &m1, const MIMCMessage &m2) {
		return m1.sequence < m2.sequence;
	}
private:
	std::string packetId;
	int64_t sequence;
	std::string fromAccount;
	std::string fromResource;
	std::string toAccount;
	std::string toResource;
	std::string payload;
	std::string bizType;
	time_t timestamp;
};

#endif