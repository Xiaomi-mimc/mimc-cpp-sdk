#ifndef MIMC_CPP_SDK_ONLINESTATUSHANDLER_H
#define MIMC_CPP_SDK_ONLINESTATUSHANDLER_H

#include <mimc/constant.h>
#include <string>

using namespace std;

class OnlineStatusHandler {
public:
	virtual void statusChange(OnlineStatus status, string errType, string errReason, string errDescription) = 0;
	virtual ~OnlineStatusHandler() {};
};

#endif