#ifndef MIMC_CPP_TEST_ONLINESTATUS_HANDLER_H
#define MIMC_CPP_TEST_ONLINESTATUS_HANDLER_H

#include "mimc/onlinestatus_handler.h"
#include "XMDLoggerWrapper.h"

class TestOnlineStatusHandler : public OnlineStatusHandler {
public:
    void statusChange(OnlineStatus status, const std::string& type, const std::string& reason, const std::string& desc) {
        XMDLoggerWrapper::instance()->info("In statusChange, status is %d, type is %s, reason is %s, desc is %s", status, type.c_str(), reason.c_str(), desc.c_str());
    }
};

#endif