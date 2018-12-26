#ifndef MIMC_CPP_TEST_RTSMESSAGEDATA_H
#define MIMC_CPP_TEST_RTSMESSAGEDATA_H

#include <string>
#include <mimc/constant.h>
using namespace std;

class RtsMessageData {
private:
	string fromAccount;
	string fromResource;
	uint64_t callId;
	string appContent;
	bool accepted = false;
	string errMsg;
	string recvData;
	RtsDataType dataType;
	RtsChannelType channelType;
public:
	RtsMessageData(){}
	RtsMessageData(uint64_t callId, string fromAccount, string appContent, string fromResource) {
		this->callId = callId;
		this->fromAccount = fromAccount;
		this->appContent = appContent;
		this->fromResource = fromResource;
	}

	RtsMessageData(uint64_t callId, string errMsg, bool accepted = false) {
		this->callId = callId;
		this->errMsg = errMsg;
		this->accepted = accepted;
	}

	RtsMessageData(uint64_t callId, string recvData, RtsDataType dataType, RtsChannelType channelType) {
		this->callId = callId;
		this->recvData = recvData;
		this->dataType = dataType;
		this->channelType = channelType;
	}

	string getFromAccount() {return this->fromAccount;}
	string getFromResource() {return this->fromResource;}
	uint64_t getCallId() {return this->callId;}
	string getAppContent() {return this->appContent;}
	bool isAccepted() {return this->accepted;}
	string getErrMsg() {return this->errMsg;}
	string getRecvData() {return this->recvData;}
	RtsDataType getDataType() {return this->dataType;}
	RtsChannelType getChannelType() {return this->channelType;}
};


#endif