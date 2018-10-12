#ifndef MIMC_CPP_TEST_RTSMESSAGEDATA_H
#define MIMC_CPP_TEST_RTSMESSAGEDATA_H

#include <string>
#include <mimc/constant.h>
using namespace std;

class RtsMessageData {
private:
	string fromAccount;
	string fromResource;
	long chatId;
	string appContent;
	bool accepted = false;
	string errmsg;
	string recvData;
	RtsDataType dataType;
	RtsChannelType channelType;
public:
	RtsMessageData(){}
	RtsMessageData(string fromAccount, string fromResource, long chatId, string appContent) {
		this->fromAccount = fromAccount;
		this->fromResource = fromResource;
		this->chatId = chatId;
		this->appContent = appContent;
	}

	RtsMessageData(long chatId, string errmsg, bool accepted = false) {
		this->chatId = chatId;
		this->errmsg = errmsg;
		this->accepted = accepted;
	}

	RtsMessageData(long chatId, string recvData, RtsDataType dataType, RtsChannelType channelType) {
		this->chatId = chatId;
		this->recvData = recvData;
		this->dataType = dataType;
		this->channelType = channelType;
	}

	string getFromAccount() {return this->fromAccount;}
	string getFromResource() {return this->fromResource;}
	long getChatId() {return this->chatId;}
	string getAppContent() {return this->appContent;}
	bool isAccepted() {return this->accepted;}
	string getErrmsg() {return this->errmsg;}
	string getRecvData() {return this->recvData;}
	RtsDataType getDataType() {return this->dataType;}
	RtsChannelType getChannelType() {return this->channelType;}
};


#endif