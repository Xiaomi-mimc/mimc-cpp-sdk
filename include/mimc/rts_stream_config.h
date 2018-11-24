#ifndef MIMC_CPP_SDK_RTS_STREAM_CONFIG_H
#define MIMC_CPP_SDK_RTS_STREAM_CONFIG_H

#include <mimc/constant.h>

class RtsStreamConfig {
public:
	RtsStreamConfig(RtsStreamType type, unsigned int ackStreamWaitTimeMs, bool isEncrypt) 
	: _type(type), _ackStreamWaitTimeMs(ackStreamWaitTimeMs), _isEncrypt(isEncrypt) {

	}

	RtsStreamType getType() const {return _type;}
	unsigned int getAckStreamWaitTimeMs() const {return _ackStreamWaitTimeMs;}
	bool getEncrypt() const {return _isEncrypt;}

private:
	RtsStreamType _type;
	unsigned int _ackStreamWaitTimeMs;
	bool _isEncrypt;
};

#endif