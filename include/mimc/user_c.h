#ifndef MIMC_CPP_SDK_USER_C_H
#define MIMC_CPP_SDK_USER_C_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	FEC_T,
	ACK_T
} stream_type_t;

typedef enum {
	AUD,
	VID
} data_type_t;

typedef enum {
	RELAY_T,
	P2P_INTRANET_T,
	P2P_INTERNET_T
} channel_type_t;

typedef enum {
	P0_T,
	P1_T,
	P2_T
} data_priority_t;

typedef enum {
	OFFLINE,
	ONLINE
} online_status_t;

typedef struct {
	void* value;
} user_t;

typedef struct {
	stream_type_t type;
	unsigned int ackstream_waittime_ms;
	bool is_encrypt;
} stream_config_t;

typedef struct {
	bool accepted;
	const char* errmsg;
} launched_response_t;

typedef struct {
	const char* (*fetch_token)();
} token_fetcher_t;

typedef struct {
	void (*status_change)(online_status_t online_status, const char* err_type, const char* err_reason, const char* err_description);
} online_status_handler_t;

typedef struct {
	launched_response_t (*on_launched)(long chatid, const char* from_account, const char* appcontent, const int appcontent_len, const char* from_resource);
	void (*on_answered)(long chatid, bool accepted, const char* errmsg); 
	void (*on_closed)(long chatid, const char* errmsg);
	void (*handle_data)(long chatid, const char* data, const int data_len, data_type_t data_type, channel_type_t channel_type);
	void (*handle_send_data_succ)(long chatid, int groupid, const char* ctx, const int ctx_len);
	void (*handle_send_data_fail)(long chatid, int groupid, const char* ctx, const int ctx_len);
} rtscall_event_handler_t;

void mimc_rtc_init(user_t* user, const char* appaccount, const char* resource, const char* cachepath);
void mimc_rtc_fini(user_t* user);
bool mimc_rtc_login(user_t* user);
bool mimc_rtc_logout(user_t* user);
void mimc_rtc_set_max_callnum(user_t* user, unsigned int num);
void mimc_rtc_init_audiostream_config(user_t* user, const stream_config_t* stream_config);
void mimc_rtc_init_videostream_config(user_t* user, const stream_config_t* stream_config);
void mimc_rtc_set_sendbuffer_size(user_t* user, int size);
void mimc_rtc_set_recvbuffer_size(user_t* user, int size);
int mimc_rtc_get_sendbuffer_size(user_t* user);
int mimc_rtc_get_recvbuffer_size(user_t* user);
float mimc_rtc_get_sendbuffer_usagerate(user_t* user);
float mimc_rtc_get_recvbuffer_usagerate(user_t* user);
void mimc_rtc_clear_sendbuffer(user_t* user);
void mimc_rtc_clear_recvbuffer(user_t* user);
long mimc_rtc_dial_call(user_t* user, const char* to_appaccount, const char* appcontent, const int appcontent_len, const char* to_resource);
void mimc_rtc_close_call(user_t* user, long chatid, const char* bye_reason);
bool mimc_rtc_send_data(user_t* user, long chatid, const char* data, const int data_len, const data_type_t data_type, const channel_type_t channel_type, const char* ctx, const int ctx_len, const bool can_be_dropped, const data_priority_t data_priority, const unsigned int resend_count);

void mimc_rtc_register_token_fetcher(user_t* user, const token_fetcher_t* token_fetcher);
void mimc_rtc_register_online_status_handler(user_t* user, const online_status_handler_t* online_status_handler);
void mimc_rtc_register_rtscall_event_handler(user_t* user, const rtscall_event_handler_t* rtscall_event_handler);

#ifdef __cplusplus
}
#endif

#endif
