#include <mimc/user_c.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>

#ifndef STAGING
const char* app_id = "2882303761517669588";
const char* app_key = "5111766983588";
const char* app_secret = "b0L3IOz/9Ob809v8H2FbVg==";
#else
const char* app_id = "2882303761517479657";
const char* app_key = "5221747911657";
const char* app_secret = "PtfBeZyC+H8SIM/UXhZx1w==";
#endif

const char* username1 = "mimc_rtc_user1_demo";
const char* username2 = "mimc_rtc_user2_demo";

static const int appcontent_1_len = 3;
static const char appcontent_1[3] = {0xF0,0xF0,0xF0};

void status_change(online_status_t online_status, const char* type, const char* reason, const char* desc) {
    printf("In statusChange, status is %d, type is %s, reason is %s, desc is %s\n", online_status, type, reason, desc);
}

launched_response_t on_launched(uint64_t callid, const char* from_account, const char* appcontent, const int appcontent_len, const char* from_resource) {
    printf("In on_launched, callid is %llu, from_account is %s, appcontent_len is %d, from_resource is %s\n", callid, from_account, appcontent_len, from_resource);
    launched_response_t launched_response;
    launched_response.accepted = false;
    launched_response.desc = "illegal appcontent";
    if (appcontent_len != appcontent_1_len) {
        return launched_response;
    }

    int i;
    for (i = 0; i < appcontent_len; i++) {
        if (appcontent[i] != appcontent_1[i]) {
            return launched_response;
        }
    }

    launched_response.accepted = true;
    launched_response.desc = "ok";
    return launched_response;
}

void on_answered(uint64_t callid, bool accepted, const char* desc) {
    printf("In on_answered, callid is %llu, accepted is %d, desc is %s\n", callid, accepted, desc);
}

void on_closed(uint64_t callid, const char* desc) {
    printf("In on_closed, callid is %llu, desc is %s\n", callid, desc);
}

void on_data(uint64_t callid, const char* from_account, const char* resource, const char* data, const int data_len, data_type_t data_type, channel_type_t channel_type) {
    printf("In handle_data, callid is %llu, from_account is %s, resource is %s, data_len is %d, data_type is %d, channel_type is %d\n", callid, from_account, resource, data_len, data_type, channel_type);
    printf("In handle_data,recerive data is %s\n",data);
}

void on_send_data_success(uint64_t callid, int dataid, const char* ctx, const int ctx_len) {
    printf("In handle_send_data_succ, callid is %llu, dataid is %d, ctx_len is %d\n", callid, dataid, ctx_len);
}

void on_send_data_failure(uint64_t callid, int dataid, const char* ctx, const int ctx_len) {
    printf("In handle_send_data_fail, callid is %llu, dataid is %d, ctx_len is %d\n", callid, dataid, ctx_len);
}

void on_p2p_result(uint64_t callId, int result, int selfNatType, int peerNatType) {

}

static online_status_handler_t online_status_handler = {
    .status_change = status_change
};

static rtscall_event_handler_t rtscall_event_handler = {
    .on_launched = on_launched,
    .on_answered = on_answered,
    .on_closed = on_closed,
    .on_data = on_data,
    .on_send_data_success = on_send_data_success,
    .on_send_data_failure = on_send_data_failure,
    .on_p2p_result = on_p2p_result
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    memcpy(userp, contents, size * nmemb);
    return size * nmemb;
}

void fetch_token(void *args, char *out_buffer, int out_buffer_len)
{
    CURL *curl = curl_easy_init();
    CURLcode res;
#ifndef STAGING
    const char *url = "https://mimc.chat.xiaomi.net/api/account/token";
#else
    const std::string url = "http://10.38.162.149/api/account/token";
#endif

    char body[512];
    memset(body, 0, 512);
    sprintf(body, "{\"appId\":\"%s\",\"appKey\":\"%s\",\"appSecret\":\"%s\",\"appAccount\":\"%s\"}", app_id, app_key, app_secret, args);
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)out_buffer);

        char errbuf[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
        errbuf[0] = 0;

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            printf("curl perform error, error code is %d, error msg is %s", res, errbuf);
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    printf("user:%s token is %s\n", (char *)args, out_buffer);
}

void free_args(void *args)
{
}

int main() {
    user_t user2_obj;
    user_t* user2 = &user2_obj;
    mimc_init(user2, atoll(app_id), username2, "camera02", false, NULL);
    mimc_register_token_fetcher(user2, (void*)username2, fetch_token, free_args);
    mimc_register_online_status_handler(user2, &online_status_handler);
    mimc_rtc_register_rtscall_event_handler(user2, &rtscall_event_handler);
    time_t user2_login_time = time(NULL);
    mimc_login(user2);

    while ((time(NULL) - user2_login_time < 5 && !mimc_isonline(user2))) {
        usleep(100000);
    }

    mimc_rtc_set_max_callnum(user2, 3);

    while (true) {
        sleep(1);
    }

    sleep(1);
    mimc_logout(user2);
    sleep(1);
    mimc_fini(user2);
}

