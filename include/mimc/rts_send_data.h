#ifndef MIMC_CPP_SDK_SEND_RTS_DATA_H
#define MIMC_CPP_SDK_SEND_RTS_DATA_H

#include <string>
#include <mimc/rts_data.pb.h>
class User;

class RtsSendData {
public:
	static unsigned long createRelayConn(User* user);
	static bool sendBindRelayRequest(User* user);
	static bool sendPingRelayRequest(User* user);
	static bool sendRtsDataByRelay(User* user, long chatId, const std::string& data, mimc::PKT_TYPE pktType);
	static bool closeRelayConnWhenNoChat(User* user);
};

#endif