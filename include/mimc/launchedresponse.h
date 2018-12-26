#ifndef MIMC_CPP_SDK_LAUNCHEDRESPONSE_H
#define MIMC_CPP_SDK_LAUNCHEDRESPONSE_H

#include <string>

class LaunchedResponse {
	public:
		LaunchedResponse(bool accepted, std::string msg) {
			this->accepted = accepted;
			this->errMsg = msg;
		}
		~LaunchedResponse(){}
		bool isAccepted() { return this->accepted; }
		std::string getErrMsg() { return this->errMsg; }
	private:
		bool accepted;
		std::string errMsg;
};

#endif