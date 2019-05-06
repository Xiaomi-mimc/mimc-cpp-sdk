#ifndef MIMC_CPP_SDK_RTS_CONNECTION_INFO_H
#define MIMC_CPP_SDK_RTS_CONNECTION_INFO_H

#include <string>
#include <stdint.h>

enum RtsConnType {
	RELAY_CONN,
	INTRANET_CONN,
	INTERNET_CONN
};

class RtsConnectionInfo {
public:
	RtsConnectionInfo(std::string address, RtsConnType connType, uint64_t callId = 0)
		: address(address), rtsConnType(connType), callId(callId) {
		
	}

	std::string getAddress() {
		return this->address;
	}

	RtsConnType getConnType() {
		return this->rtsConnType;
	}

	uint64_t getCallId() {
		return this->callId;
	}
private:
	std::string address;
	RtsConnType rtsConnType;
	uint64_t callId;
};

#endif