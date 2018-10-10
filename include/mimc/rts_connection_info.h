#ifndef MIMC_CPP_SDK_RTS_CONNECTION_INFO_H
#define MIMC_CPP_SDK_RTS_CONNECTION_INFO_H

enum RtsConnType {
	RELAY_CONN,
	INTRANET_CONN,
	INTERNET_CONN
};

class RtsConnectionInfo {
public:
	RtsConnectionInfo(RtsConnType connType, long chatId = -1)
		: rtsConnType(connType), chatId(chatId) {
		
	}

	RtsConnType getConnType() {
		return this->rtsConnType;
	}

	long getChatId() {
		return this->chatId;
	}
private:
	RtsConnType rtsConnType;
	long chatId;
};

#endif