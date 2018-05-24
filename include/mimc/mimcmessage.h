#ifndef MIMC_CPP_SDK_MIMCMESSAGE_H
#define MIMC_CPP_SDK_MIMCMESSAGE_H

#include <string>

class MIMCMessage {
public:
	MIMCMessage(){}
	MIMCMessage(const std::string& packetId, long sequence, const std::string& fromAccount, const std::string& fromResource, const std::string& payload, long timestamp) {
		this->packetId = packetId;
		this->sequence = sequence;
		this->fromAccount = fromAccount;
		this->fromResource = fromResource;
		this->payload = payload;
		this->timestamp = timestamp;
	}
	const std::string &getPacketId() const { return this->packetId; }
	const long &getSequence() const { return this->sequence; }
	const long &getTimeStamp() const { return this->timestamp; }
	const std::string &getFromAccount() const { return this->fromAccount; }
	const std::string &getFromResource() const { return this->fromResource; }
	const std::string &getPayload() const { return this->payload; }
	static bool sortBySequence(const MIMCMessage &m1, const MIMCMessage &m2) {
		return m1.sequence < m2.sequence;
	}
private:
	std::string packetId;
	long sequence;
	long timestamp;
	std::string fromAccount;
	std::string fromResource;
	std::string payload;
};

#endif