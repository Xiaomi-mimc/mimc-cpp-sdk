#include <mimc/user_c.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

const char* username1 = "mimc_rtc_user1";
const char* username2 = "mimc_rtc_user2";

#ifndef STAGING
const char* app_id = "2882303761517669588";
const char* app_key = "5111766983588";
const char* app_secret = "b0L3IOz/9Ob809v8H2FbVg==";
#else
const char* app_id = "2882303761517479657";
const char* app_key = "5221747911657";
const char* app_secret = "PtfBeZyC+H8SIM/UXhZx1w==";
#endif

static const int appcontent_1_len = 3;
static const char appcontent_1[3] = {0xF0,0xF0,0xF0};
static const int appcontent_2_len = 3;
static const char appcontent_2[3] = {0xF0,0xF0,0xF0};

size_t WriteCallback_1(void *contents, size_t size, size_t nmemb, void *userp);
size_t WriteCallback_2(void *contents, size_t size, size_t nmemb, void *userp);

const char* fetch_token_1() {
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
        strcat(body, username1);
        strcat(body, "\"}");

        printf("In fetch_token_1, body is %s, body.size is %d\n", body, strlen(body));
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

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback_1);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)result);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                printf("curl perform error, error code is %d\n", res);
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();

        printf("In fetch_token_1, result is %s\n", result);

        return result;
}

size_t WriteCallback_1(void *contents, size_t size, size_t nmemb, void *userp) {
    char* bodyp = (char *)(userp);
    memmove(bodyp + strlen(bodyp), (char *)contents, size * nmemb);

    return size * nmemb;
}

const char* fetch_token_2() {
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
        strcat(body, username2);
        strcat(body, "\"}");

        printf("In fetch_token_2, body is %s, body.size is %d\n", body, strlen(body));
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

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback_2);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)result);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                printf("curl perform error, error code is %d\n", res);
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();

        printf("In fetch_token_2, result is %s\n", result);

        return result;
}

size_t WriteCallback_2(void *contents, size_t size, size_t nmemb, void *userp) {
    char* bodyp = (char *)(userp);
    memmove(bodyp + strlen(bodyp), (char *)contents, size * nmemb);

    return size * nmemb;
}

void status_change(online_status_t online_status, const char* err_type, const char* err_reason, const char* err_description) {
    printf("In statusChange, status is %d, errType is %s, errReason is %s, errDescription is %s\n", online_status, err_type, err_reason, err_description);
}

launched_response_t on_launched_1(long chatid, const char* from_account, const char* appcontent, const int appcontent_len, const char* from_resource) {
    printf("In on_launched_1, chatid is %ld, from_account is %s, appcontent_len is %d, from_resource is %s\n", chatid, from_account, appcontent_len, from_resource);
    launched_response_t launched_response;
    launched_response.accepted = false;
    launched_response.errmsg = "illegal signature";
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
    launched_response.errmsg = "ok";
    return launched_response;
}

launched_response_t on_launched_2(long chatid, const char* from_account, const char* appcontent, const int appcontent_len, const char* from_resource) {
    printf("In on_launched_2, chatid is %ld, from_account is %s, appcontent_len is %d, from_resource is %s\n", chatid, from_account, appcontent_len, from_resource);
    launched_response_t launched_response;
    launched_response.accepted = false;
    launched_response.errmsg = "illegal signature";
    if (appcontent_len != appcontent_2_len) {
        return launched_response;
    }

    int i;
    for (i = 0; i < appcontent_len; i++) {
        if (appcontent[i] != appcontent_2[i]) {
            return launched_response;
        }
    }

    launched_response.accepted = true;
    launched_response.errmsg = "ok";
    return launched_response;
}

void on_answered(long chatid, bool accepted, const char* errmsg) {
    printf("In on_answered, chatid is %ld, accepted is %d, errmsg is %s\n", chatid, accepted, errmsg);
}

void on_closed(long chatid, const char* errmsg) {
    printf("In on_closed, chatid is %ld, errmsg is %s\n", chatid, errmsg);
}

void handle_data(long chatid, const char* data, const int data_len, data_type_t data_type, channel_type_t channel_type) {
    printf("In handle_data, chatid is %ld, data_len is %d, data_type is %d, channel_type is %d\n", chatid, data_len, data_type, channel_type);
}

void handle_send_data_succ(long chatid, int groupid, const char* ctx, const int ctx_len) {
    printf("In handle_send_data_succ, chatid is %ld, groupid is %d, ctx_len is %d\n", chatid, groupid, ctx_len);
}

void handle_send_data_fail(long chatid, int groupid, const char* ctx, const int ctx_len) {
    printf("In handle_send_data_fail, chatid is %ld, groupid is %d, ctx_len is %d\n", chatid, groupid, ctx_len);
}

static token_fetcher_t token_fetcher_1 = {
    .fetch_token = fetch_token_1
};

static token_fetcher_t token_fetcher_2 = {
    .fetch_token = fetch_token_2
};

static online_status_handler_t online_status_handler = {
    .status_change = status_change
};

static rtscall_event_handler_t rtscall_event_handler_1 = {
    .on_launched = on_launched_1,
    .on_answered = on_answered,
    .on_closed = on_closed,
    .handle_data = handle_data,
    .handle_send_data_succ = handle_send_data_succ,
    .handle_send_data_fail = handle_send_data_fail
};

static rtscall_event_handler_t rtscall_event_handler_2 = {
    .on_launched = on_launched_2,
    .on_answered = on_answered,
    .on_closed = on_closed,
    .handle_data = handle_data,
    .handle_send_data_succ = handle_send_data_succ,
    .handle_send_data_fail = handle_send_data_fail
};

int main() {
    user_t user1_obj;
	user_t* user1 = &user1_obj;
	mimc_rtc_init(user1, username1, "camera01", NULL);
    mimc_rtc_register_token_fetcher(user1, &token_fetcher_1);
    mimc_rtc_register_online_status_handler(user1, &online_status_handler);
    mimc_rtc_register_rtscall_event_handler(user1, &rtscall_event_handler_1);
    mimc_rtc_login(user1);

    user_t user2_obj;
    user_t* user2 = &user2_obj;
    mimc_rtc_init(user2, username2, "camera02", NULL);
    mimc_rtc_register_token_fetcher(user2, &token_fetcher_2);
    mimc_rtc_register_online_status_handler(user2, &online_status_handler);
    mimc_rtc_register_rtscall_event_handler(user2, &rtscall_event_handler_2);
    mimc_rtc_login(user2);

    sleep(1);
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

    long chatid = mimc_rtc_dial_call(user1, username2, appcontent_2, appcontent_2_len, "camera02");
    printf("user1 sendbuffer size is %d\n", mimc_rtc_get_sendbuffer_size(user1));

    sleep(1);
    printf("user2 sendbuffer size is %d\n", mimc_rtc_get_sendbuffer_size(user2));
    const char* data = "12323131323";
    const char* ctx = "dasda";
    mimc_rtc_send_data(user1, chatid, data, strlen(data), AUD, RELAY_T, ctx, strlen(ctx), false, P1_T, 2);
    printf("user1 sendbuffer usagerate is %f\n", mimc_rtc_get_sendbuffer_usagerate(user1));
    sleep(1);
    mimc_rtc_close_call(user1, chatid, "i don't wanna talk");
    sleep(1);
    mimc_rtc_logout(user1);
    mimc_rtc_logout(user2);
    sleep(1);
    mimc_rtc_fini(user1);
    mimc_rtc_fini(user2);
}