#ifndef MIMC_CPP_TEST_TOKENFETCHER_H
#define MIMC_CPP_TEST_TOKENFETCHER_H

#include <mimc/tokenfetcher.h>
#include <curl/curl.h>
using namespace std;

class TestTokenFetcher : public MIMCTokenFetcher {
public:
    string fetchToken() {
        curl_global_init(CURL_GLOBAL_ALL);
        CURL *curl = curl_easy_init();
        CURLcode res;
        const string url = "https://mimc.chat.xiaomi.net/api/account/token";
        const string body = "{\"appId\":\"" + this->appId + "\",\"appKey\":\"" + this->appKey + "\",\"appSecret\":\"" + this->appSecret + "\",\"appAccount\":\"" + this->appAccount + "\"}";
        string result;
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&result));

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {

            }
        }

        curl_easy_cleanup(curl);

        return result;
    }

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        string *bodyp = (string *)userp;
        bodyp->append((const char *)contents, size * nmemb);



        return bodyp->size();
    }

    TestTokenFetcher(string appId, string appKey, string appSecret, string appAccount)
    {
        this->appId = appId;
        this->appKey = appKey;
        this->appSecret = appSecret;
        this->appAccount = appAccount;
    }
private:
    string appId;
    string appKey;
    string appSecret;
    string appAccount;
};

#endif