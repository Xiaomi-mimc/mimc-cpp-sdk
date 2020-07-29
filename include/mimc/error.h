#ifndef MIMC_CPP_SDK_ERROR_H
#define MIMC_CPP_SDK_ERROR_H

const char* const DIAL_CALL_TIMEOUT = "DIAL_CALL_TIMEOUT";
const char* const INVITE_RESPONSE_TIMEOUT = "INVITE_RESPONSE_TIMEOUT";
const char* const CREATE_RELAY_TIMEOUT = "CREATE_RELAY_TIMEOUT";
const char* const CLOSE_CALL_TIMEOUT = "CLOSE_CALL_TIMEOUT";
const char* const UPDATE_TIMEOUT = "UPDATE_TIMEOUT";
const char* const COMMON_TIMEOUT = "TIMEOUT";

const char* const SEND_BIND_RELAY_REQUEST_FAIL = "SEND_BIND_RELAY_REQUEST_FAIL";
const char* const CREATE_RELAY_CONN_FAIL = "CREATE_RELAY_CONN_FAIL";
const char* const ALL_DATA_CHANNELS_CLOSED = "ALL_DATA_CHANNELS_CLOSED";

class MimcError {

public:
    static const int INPUT_PARAMS_ERROR            = -2001;
    static const int CALLSESSION_NOT_FOUND         = -2002;
    static const int CALLSESSION_NOT_RUNNING       = -2003;
    static const int DIAL_USER_NOT_LINE            = -2004;
    static const int RELAY_CONNID_ERROR            = -2005;
};


#endif