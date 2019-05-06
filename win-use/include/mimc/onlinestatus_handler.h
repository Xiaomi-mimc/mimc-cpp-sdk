#ifndef MIMC_CPP_SDK_ONLINESTATUSHANDLER_H
#define MIMC_CPP_SDK_ONLINESTATUSHANDLER_H

#include <mimc/constant.h>
#include <string>

class OnlineStatusHandler {
public:
	virtual void statusChange(OnlineStatus status, std::string type, std::string reason, std::string desc) = 0;
	virtual ~OnlineStatusHandler() {}
};

#endif