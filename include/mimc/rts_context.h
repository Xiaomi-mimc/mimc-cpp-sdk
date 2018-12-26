#ifndef MIMC_CPP_SDK_RTS_CONTEXT_H
#define MIMC_CPP_SDK_RTS_CONTEXT_H

#include <string>
#include <stdint.h>
class RtsContext {
public:
	RtsContext(uint64_t callId, std::string ctx)
	 : callId_(callId), ctx_(ctx) 
	 {

	 }

	 uint64_t getCallId() const {return this->callId_;}

	 std::string getCtx() const {return this->ctx_;}

private:
	uint64_t callId_;
	std::string ctx_;
};

#endif