#ifndef MIMC_CPP_SDK_MIMCMESSAGE_H
#define MIMC_CPP_SDK_MIMCMESSAGE_H

#include <string>
#include <time.h>

#ifdef WIN_USE_DLL
#ifdef MIMCAPI_EXPORTS
#define MIMCAPI __declspec(dllexport)
#else
#define MIMCAPI __declspec(dllimport)
#endif // MIMCAPI_EXPORTS
#else
#define MIMCAPI
#endif

#ifdef _WIN32
class MIMCAPI MIMCMessage {
#else
class MIMCMessage {
#endif // _WIN32

public:
	MIMCMessage(){}
	MIMCMessage(const std::string &packetId, int64_t sequence, const std::string &fromAccount, const std::string &fromResource, const std::string &toAccount, const std::string &toResource, const std::string &payload, const std::string &bizType, time_t timestamp, int64_t convIndex = 0)
	{
		this->packetId = packetId;
		this->sequence = sequence;
		this->fromAccount = fromAccount;
		this->fromResource = fromResource;
		this->toAccount = toAccount;
		this->toResource = toResource;
		this->payload = payload;
		this->bizType = bizType;
		this->timestamp = timestamp;
		this->convIndex = convIndex;
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
	int64_t getConvIndex() const { return convIndex; }
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
	int64_t convIndex;
};

#endif