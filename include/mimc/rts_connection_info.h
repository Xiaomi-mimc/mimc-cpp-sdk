#ifndef MIMC_CPP_SDK_RTS_CONNECTION_INFO_H
#define MIMC_CPP_SDK_RTS_CONNECTION_INFO_H

#include <string>

enum RtsConnType {
	RELAY_CONN,
	INTRANET_CONN,
	INTERNET_CONN
};

class RtsConnectionInfo {
public:
	RtsConnectionInfo(std::string address, RtsConnType connType, long chatId = -1)
		: address(address), rtsConnType(connType), chatId(chatId) {
		
	}

	std::string getAddress() {
		return this->address;
	}

	RtsConnType getConnType() {
		return this->rtsConnType;
	}

	long getChatId() {
		return this->chatId;
	}
private:
	std::string address;
	RtsConnType rtsConnType;
	long chatId;
};

#endif