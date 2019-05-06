#ifndef MIMC_CPP_SDK_TOKENFETCHER_H
#define MIMC_CPP_SDK_TOKENFETCHER_H

#include <string>

class MIMCTokenFetcher {
	public:
		virtual std::string fetchToken() = 0;
		virtual ~MIMCTokenFetcher() {}
};

#endif