#include <mimc/user_c.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

#ifndef STAGING
const char* app_id = "2882303761517669588";
const char* app_key = "5111766983588";
const char* app_secret = "b0L3IOz/9Ob809v8H2FbVg==";
#else
const char* app_id = "2882303761517479657";
const char* app_key = "5221747911657";
const char* app_secret = "PtfBeZyC+H8SIM/UXhZx1w==";
#endif

const char* username1 = "mimc_rtc_user1";
const char* username2 = "mimc_rtc_user2";

static const int appcontent_1_len = 3;
static const char appcontent_1[3] = {0xF0,0xF0,0xF0};

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

const char* fetch_token(const char* appaccount) {
	curl_global_init(CURL_GLOBAL_ALL);
        CURL *curl = curl_easy_init();
        CURLcode res;
#ifndef STAGING
        const char* url = "https://mimc.chat.xiaomi.net/api/account/token";
#else
        const char* url = "http://10.38.162.149/api/account/token";
#endif
        char body[130];
        memset(body, 0, 130);
        strcat(body, "{\"appId\":\""); 
        strcat(body, app_id);
        strcat(body, "\",\"appKey\":\"");
        strcat(body, app_key);
        strcat(body, "\",\"appSecret\":\"");
        strcat(body, app_secret);
        strcat(body, "\",\"appAccount\":\"");
        strcat(body, appaccount);
        strcat(body, "\"}");

        printf("In fetch_token, body is %s, body.size is %d\n", body, strlen(body));
        char* result = (char *)calloc(1000, 1);
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)result);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                printf("curl perform error, error code is %d\n", res);
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();

        printf("In fetch_token, result is %s\n", result);

        return result;
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    char* bodyp = (char *)(userp);
    memmove(bodyp + strlen(bodyp), (char *)contents, size * nmemb);

    return size * nmemb;
}

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
}

void on_send_data_success(uint64_t callid, int dataid, const char* ctx, const int ctx_len) {
    printf("In handle_send_data_succ, callid is %llu, dataid is %d, ctx_len is %d\n", callid, dataid, ctx_len);
}

void on_send_data_failure(uint64_t callid, int dataid, const char* ctx, const int ctx_len) {
    printf("In handle_send_data_fail, callid is %llu, dataid is %d, ctx_len is %d\n", callid, dataid, ctx_len);
}

static token_fetcher_t token_fetcher = {
    .fetch_token = fetch_token
};

static online_status_handler_t online_status_handler = {
    .status_change = status_change
};

static rtscall_event_handler_t rtscall_event_handler = {
    .on_launched = on_launched,
    .on_answered = on_answered,
    .on_closed = on_closed,
    .on_data = on_data,
    .on_send_data_success = on_send_data_success,
    .on_send_data_failure = on_send_data_failure
};

int main() {
    user_t user1_obj;
	user_t* user1 = &user1_obj;
	mimc_rtc_init(user1, atoll(app_id), username1, "camera01", NULL);
    mimc_rtc_register_token_fetcher(user1, &token_fetcher, username1);
    mimc_rtc_register_online_status_handler(user1, &online_status_handler);
    mimc_rtc_register_rtscall_event_handler(user1, &rtscall_event_handler);
    mimc_rtc_login(user1);

    user_t user2_obj;
    user_t* user2 = &user2_obj;
    mimc_rtc_init(user2, atoll(app_id), username2, "camera02", NULL);
    mimc_rtc_register_token_fetcher(user2, &token_fetcher, username2);
    mimc_rtc_register_online_status_handler(user2, &online_status_handler);
    mimc_rtc_register_rtscall_event_handler(user2, &rtscall_event_handler);
    mimc_rtc_login(user2);

    sleep(4);

    mimc_rtc_set_max_callnum(user1, 2);
    mimc_rtc_set_max_callnum(user2, 1);

    stream_config_t audiostream_config_user1 = {.type = FEC_T, .ackstream_waittime_ms = 500, .is_encrypt = false};
    mimc_rtc_init_audiostream_config(user1, &audiostream_config_user1);

    stream_config_t videostream_config_user1 = {.type = ACK_T, .ackstream_waittime_ms = 500, .is_encrypt = false};
    mimc_rtc_init_videostream_config(user1, &videostream_config_user1);

    stream_config_t audiostream_config_user2 = {.type = ACK_T, .ackstream_waittime_ms = 200, .is_encrypt = false};
    mimc_rtc_init_audiostream_config(user2, &audiostream_config_user2);

    stream_config_t videostream_config_user2 = {.type = FEC_T, .ackstream_waittime_ms = 500, .is_encrypt = false};
    mimc_rtc_init_videostream_config(user2, &videostream_config_user2);

    mimc_rtc_set_sendbuffer_size(user1, 1000);

    mimc_rtc_set_sendbuffer_size(user2, 800);

    uint64_t callid = mimc_rtc_dial_call(user1, username2, appcontent_1, appcontent_1_len, "camera02");
    printf("user1 sendbuffer size is %d\n", mimc_rtc_get_sendbuffer_size(user1));

    sleep(2);
    printf("user2 sendbuffer size is %d\n", mimc_rtc_get_sendbuffer_size(user2));
    const char* data = "12323131323";
    const char* ctx = "dasda";
    mimc_rtc_send_data(user1, callid, data, strlen(data), AUD, RELAY_T, ctx, strlen(ctx), false, P1_T, 2);
    printf("user1 sendbuffer usagerate is %f\n", mimc_rtc_get_sendbuffer_usagerate(user1));
    sleep(1);
    mimc_rtc_close_call(user1, callid, "i don't wanna talk");
    sleep(1);
    mimc_rtc_logout(user1);
    mimc_rtc_logout(user2);
    sleep(1);
    mimc_rtc_fini(user1);
    mimc_rtc_fini(user2);
}