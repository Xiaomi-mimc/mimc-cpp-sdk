#ifndef MIMC_CPP_SDK_RTS_CONTEXT_H
#define MIMC_CPP_SDK_RTS_CONTEXT_H

#include <string>
class RtsContext {
public:
	RtsContext(long chatId, std::string ctx)
	 : chatId_(chatId), ctx_(ctx) 
	 {

	 }

	 long getChatId() const {return this->chatId_;}

	 std::string getCtx() const {return this->ctx_;}

private:
	long chatId_;
	std::string ctx_;
};

#endif