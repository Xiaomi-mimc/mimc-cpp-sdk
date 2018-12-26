#ifndef MIMC_CPP_SDK_SERVERFETCHER_H
#define MIMC_CPP_SDK_SERVERFETCHER_H

#include <string>
#include <curl/curl.h>
#include <json-c/json.h>

class ServerFetcher {
public:
	static std::string fetchServerAddr(const char* const &url, std::string list);

private:
	static size_t writeRecData(char* ptr, size_t size, size_t nmemb, void* userdata);
};

#endif