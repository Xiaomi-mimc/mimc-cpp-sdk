#ifndef MIMC_CPP_TEST_TOKENFETCHER_H
#define MIMC_CPP_TEST_TOKENFETCHER_H

#include <mimc/tokenfetcher.h>
#include <curl/curl.h>
#include <XMDLoggerWrapper.h>
#include <mutex>

using namespace std;

class TestTokenFetcher : public MIMCTokenFetcher {
public:
	static std::mutex s_mutex;

	string fetchToken(); 

	static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
	

	TestTokenFetcher(string appId, string appKey, string appSecret, string appAccount);

private:
    string appId;
    string appKey;
    string appSecret;
    string appAccount;
};

#endif