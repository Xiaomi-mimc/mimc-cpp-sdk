#ifndef MIMC_CPP_SDK_MIMCSERVERACK_H
#define MIMC_CPP_SDK_MIMCSERVERACK_H

#include <string>
#include <time.h>

class MIMCServerAck {
public:
	MIMCServerAck(const std::string &packetId, int64_t sequence, time_t timestamp, int32_t code, const std::string &desc, int64_t convIndex = 0)
	{
		this->packetId = packetId;
		this->sequence = sequence;
		this->timestamp = timestamp;
		this->code = code;
		this->desc = desc;
		this->convIndex = convIndex;
	}

	std::string getPacketId() const { return this->packetId; }
	int64_t getSequence() const { return this->sequence; }
	time_t getTimestamp() const { return this->timestamp; }
	int32_t getCode() const { return this->code; }
	std::string getDesc() const { return this->desc; }
	int64_t getConvIndex() const { return convIndex; }

private:
	std::string packetId;
	int64_t sequence;
	time_t timestamp;
	int32_t code;
	std::string desc;
	int64_t convIndex;
};
#endif