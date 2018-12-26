#ifndef MIMC_CPP_TEST_RTSMESSAGEDATA_H
#define MIMC_CPP_TEST_RTSMESSAGEDATA_H

#include <string>
#include <mimc/constant.h>
using namespace std;

class RtsMessageData {
private:
	string fromAccount;
	string fromResource;
	uint64_t chatId;
	string appContent;
	bool accepted = false;
	string errmsg;
	string recvData;
	RtsDataType dataType;
	RtsChannelType channelType;
public:
	RtsMessageData(){}
	RtsMessageData(uint64_t chatId, string fromAccount, string appContent, string fromResource) {
		this->chatId = chatId;
		this->fromAccount = fromAccount;
		this->appContent = appContent;
		this->fromResource = fromResource;
	}

	RtsMessageData(uint64_t chatId, string errmsg, bool accepted = false) {
		this->chatId = chatId;
		this->errmsg = errmsg;
		this->accepted = accepted;
	}

	RtsMessageData(uint64_t chatId, string recvData, RtsDataType dataType, RtsChannelType channelType) {
		this->chatId = chatId;
		this->recvData = recvData;
		this->dataType = dataType;
		this->channelType = channelType;
	}

	string getFromAccount() {return this->fromAccount;}
	string getFromResource() {return this->fromResource;}
	uint64_t getChatId() {return this->chatId;}
	string getAppContent() {return this->appContent;}
	bool isAccepted() {return this->accepted;}
	string getErrmsg() {return this->errmsg;}
	string getRecvData() {return this->recvData;}
	RtsDataType getDataType() {return this->dataType;}
	RtsChannelType getChannelType() {return this->channelType;}
};


#endif