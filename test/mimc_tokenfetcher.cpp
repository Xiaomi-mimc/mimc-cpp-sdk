#include "mimc_tokenfetcher.h"
#include "curl/curl.h"

std::string TestTokenFetcher::fetchToken() {
        CURL *curl = curl_easy_init();
        CURLcode res;
#ifndef STAGING
        const string url = "https://mimc.chat.xiaomi.net/api/account/token";
#else
        const string url = "http://10.38.162.149/api/account/token";
#endif
        const string body = "{\"appId\":\"" + this->appId + "\",\"appKey\":\"" + this->appKey + "\",\"appSecret\":\"" + this->appSecret + "\",\"appAccount\":\"" + this->appAccount + "\"}";
        string result;
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&result));

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                XMDLoggerWrapper::instance()->error("curl perform error, error code is %d", res);
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        return result;
    }
	
    size_t TestTokenFetcher::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp){
        string *bodyp = (string *)userp;
        bodyp->append((const char *)contents, size * nmemb);

        return size * nmemb;
    }
	
	TestTokenFetcher::TestTokenFetcher(string appId, string appKey, string appSecret, string appAccount){
        this->appId = appId;
        this->appKey = appKey;
        this->appSecret = appSecret;
        this->appAccount = appAccount;
    }