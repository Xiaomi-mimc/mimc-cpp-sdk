#ifndef MIMC_CPP_SDK_LAUNCHEDRESPONSE_H
#define MIMC_CPP_SDK_LAUNCHEDRESPONSE_H

#include <string>

class LaunchedResponse {
	public:
		LaunchedResponse(bool accepted, std::string desc) {
			this->accepted = accepted;
			this->desc = desc;
		}
		~LaunchedResponse(){}
		bool isAccepted() { return this->accepted; }
		std::string getDesc() { return this->desc; }
	private:
		bool accepted;
		std::string desc;
};

#endif