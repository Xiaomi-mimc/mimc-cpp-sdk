#ifndef MIMC_CPP_TEST_ONLINESTATUS_HANDLER_H
#define MIMC_CPP_TEST_ONLINESTATUS_HANDLER_H

#include <mimc/onlinestatus_handler.h>
#include <XMDLoggerWrapper.h>

class TestOnlineStatusHandler : public OnlineStatusHandler {
public:
    void statusChange(OnlineStatus status, std::string errType, std::string errReason, std::string errDescription) {
        XMDLoggerWrapper::instance()->info("In statusChange, status is %d, errType is %s, errReason is %s, errDescription is %s", status, errType.c_str(), errReason.c_str(), errDescription.c_str());
    }
};

#endif