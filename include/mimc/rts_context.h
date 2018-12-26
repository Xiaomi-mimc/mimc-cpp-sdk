#ifndef MIMC_CPP_SDK_RTS_CONTEXT_H
#define MIMC_CPP_SDK_RTS_CONTEXT_H

#include <string>
#include <stdint.h>
class RtsContext {
public:
	RtsContext(uint64_t chatId, std::string ctx)
	 : chatId_(chatId), ctx_(ctx) 
	 {

	 }

	 uint64_t getChatId() const {return this->chatId_;}

	 std::string getCtx() const {return this->ctx_;}

private:
	uint64_t chatId_;
	std::string ctx_;
};

#endif