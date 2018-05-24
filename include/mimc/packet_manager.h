#ifndef MIMC_CPP_SDK_PACKET_H
#define MIMC_CPP_SDK_PACKET_H

#include <map>
#include <set>
#include <mimc/connection.h>
#include <mimc/utils.h>
#include <mimc/ims_push_service.pb.h>
#include <mimc/mimc.pb.h>
#include <mimc/threadsafe_queue.h>
#include <mimc/mimcmessage.h>
#include <crypto/base64.h>
#include <pthread.h>

class User;

class PacketManager {
public:
	int encodeConnectionPacket(unsigned char * &packet, const Connection * connection);
	int encodeBindPacket(unsigned char * &packet, const Connection * connection);
	int encodeSecMsgPacket(unsigned char * &packet, const Connection * connection, const google::protobuf::Message * message);
	int encodeUnBindPacket(unsigned char * &packet, const Connection * connection);
	int encodePingPacket(unsigned char * &packet, const Connection * connection);
	int decodePacketAndHandle(unsigned char * packet, Connection * connection);
	short char2short(const unsigned char* result, int index);
	int char2int(const unsigned char* result, int index);
	std::string createPacketId();
	void checkMessageSendTimeout(const User * user);
private:
	ims::ClientHeader * createClientHeader(const User * user, std::string cmd, int cipher);
	int encodePacket(unsigned char * &packet, const ims::ClientHeader * header, const google::protobuf::Message * message, const std::string &body_key="", const std::string &payload_key="");
	void short2char(short data, unsigned char* result, int index);
	void int2char(int data, unsigned char* result, int index);

	uint32_t compute_crc32(const unsigned char *data, size_t len); 
	std::string generateSig(const ims::ClientHeader * header, const ims::XMMsgBind * bindmsg, const Connection * connection);
	std::string generatePayloadKey(const std::string &securityKey, const std::string &headerId);
private:
	std::string packetIdPrefix = Utils::generateRandomString(15);
	int packetIdSeq = 0;
	pthread_mutex_t packetIdMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t packetsTimeoutMutex = PTHREAD_MUTEX_INITIALIZER;
public:
	ThreadSafeQueue<struct waitToSendContent> packetsWaitToSend;
	std::map<std::string, struct waitToTimeoutContent> packetsWaitToTimeout;
	std::set<long> sequencesReceived;
};

#endif
