#ifndef MIMC_CPP_SDK_ONLINESTATUSHANDLER_H
#define MIMC_CPP_SDK_ONLINESTATUSHANDLER_H

#include <mimc/constant.h>
#include <string>

class OnlineStatusHandler {
public:
	virtual void statusChange(OnlineStatus status, std::string errType, std::string errReason, std::string errDescription) = 0;
	virtual ~OnlineStatusHandler() {}
};

#endif