#ifndef MIMC_CPP_SDK_LAUNCHEDRESPONSE_H
#define MIMC_CPP_SDK_LAUNCHEDRESPONSE_H

#include <string>

class LaunchedResponse {
	public:
		LaunchedResponse(bool accepted, std::string msg) {
			this->accepted = accepted;
			this->errmsg = msg;
		}
		~LaunchedResponse(){}
		bool isAccepted() { return this->accepted; }
		std::string getErrmsg() { return this->errmsg; }
	private:
		bool accepted;
		std::string errmsg;
};

#endif