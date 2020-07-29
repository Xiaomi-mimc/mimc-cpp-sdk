#ifndef MIMC_CPP_SDK_USER_C_H
#define MIMC_CPP_SDK_USER_C_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	FEC_T,
	ACK_T
} stream_type_t;

typedef enum {
	AUD,
	VID,
	FILED
} data_type_t;

typedef enum {
	RELAY_T,
	P2P_INTRANET_T,
	P2P_INTERNET_T,
	//added by huanghua on 2019-04-13 for p2p
	CHANNEL_AUTO_T
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

typedef enum
{
	ERROR_T,
	WARN_T,
	INFO_T,
	DEBUG_T
} log_level_t;

typedef enum
{
	MILLISECONDS_T,
	SECONDS_T,
	MINUTES_T
} time_unit_t;

typedef struct {
	void* value;
	void* token_fetcher;
	void* online_status_handler;
	void* rtscall_event_handler;
	void* message_handler;
} user_t;

typedef struct {
	stream_type_t type;
	unsigned int ackstream_waittime_ms;
	bool is_encrypt;
} stream_config_t;

typedef struct {
	bool accepted;
	const char* desc;
} launched_response_t;

typedef struct {
	const char* packetid;
	int64_t sequence;
	const char* from_account;
	const char* from_resource;
	const char* to_account;
	const char* to_resource;
	const char* payload;
	int payload_len;
	const char* biz_type;
	time_t timestamp;
} mimc_message_t;

typedef struct {
	const char* packetid;
	int64_t sequence;
	time_t timestamp;
	const char* from_account;
	const char* from_resource;
	uint64_t topicid;
	const char* payload;
	int payload_len;
	const char* biz_type;
} mimc_group_message_t;

typedef struct {
	void (*status_change)(online_status_t online_status, const char* type, const char* reason, const char* desc);
} online_status_handler_t;

typedef struct {
	launched_response_t (*on_launched)(uint64_t callid, const char* from_account, const char* appcontent, const int appcontent_len, const char* from_resource);
	void (*on_answered)(uint64_t callid, bool accepted, const char* desc); 
	void (*on_closed)(uint64_t callid, const char* desc);
	void (*on_data)(uint64_t callid, const char* from_account, const char* resource, const char* data, const int data_len, data_type_t data_type, channel_type_t channel_type);
	void (*on_send_data_success)(uint64_t callid, int dataid, const char* ctx, const int ctx_len);
	void (*on_send_data_failure)(uint64_t callid, int dataid, const char* ctx, const int ctx_len);
	//added by huanghua for p2p
	void (*on_p2p_result)(uint64_t callId, int result, int selfNatType, int peerNatType);
} rtscall_event_handler_t;

typedef struct {
	void (*handle_message)(const mimc_message_t* packets, uint64_t pkts_len);
	void (*handle_group_message)(const mimc_group_message_t* packets, uint64_t pkts_len);
	void (*handle_server_ack)(const char* packetid, int64_t sequence, time_t timestamp, const char* desc);
	void (*handle_send_msg_timeout)(const mimc_message_t* message);
	void (*handle_send_group_msg_timeout)(const mimc_group_message_t* group_message);
	void (*handle_online_message)(const mimc_message_t* packets);
} message_handler_t;

typedef struct
{
	void (*debug)(const char *msg);
	void (*info)(const char *msg);
	void (*warn)(const char *msg);
	void (*error)(const char *msg);
} log_handler_t;

typedef void (*fetch_token_t)(void *args, char *out_buffer, int out_buffer_len);
typedef void (*free_args_t)(void *args);


// deprecated
int mimc_rtc_get_login_timeout();
// deprecated
void mimc_rtc_init(user_t *user, int64_t appid, const char *appaccount, const char *resource, bool is_save_cache, const char *cachepath);
// deprecated
void mimc_rtc_fini(user_t* user);
// deprecated
bool mimc_rtc_get_token_fetch_succeed(user_t* user);
// deprecated
short mimc_rtc_get_token_request_status(user_t* user);
// deprecated
bool mimc_rtc_login(user_t* user);
// deprecated
bool mimc_rtc_logout(user_t* user);
// deprecated
bool mimc_rtc_isonline(user_t* user);
bool mimc_rtc_channel_connected(user_t* user);
void mimc_rtc_set_max_callnum(user_t* user, unsigned int num);
unsigned int mimc_rtc_get_max_callnum(user_t* user);
void mimc_rtc_init_audiostream_config(user_t* user, const stream_config_t* stream_config);
void mimc_rtc_init_videostream_config(user_t* user, const stream_config_t* stream_config);
void mimc_rtc_set_sendbuffer_max_size(user_t* user, int size);
void mimc_rtc_set_sendbuffer_size(user_t* user, int size);
void mimc_rtc_set_recvbuffer_max_size(user_t* user, int size);
void mimc_rtc_set_recvbuffer_size(user_t* user, int size);
int mimc_rtc_get_sendbuffer_used_size(user_t* user);
int mimc_rtc_get_sendbuffer_max_size(user_t* user);
int mimc_rtc_get_sendbuffer_size(user_t* user);
int mimc_rtc_get_recvbuffer_used_size(user_t* user);
int mimc_rtc_get_recvbuffer_max_size(user_t* user);
int mimc_rtc_get_recvbuffer_size(user_t* user);
float mimc_rtc_get_sendbuffer_usagerate(user_t* user);
float mimc_rtc_get_recvbuffer_usagerate(user_t* user);
void mimc_rtc_clear_sendbuffer(user_t* user);
void mimc_rtc_clear_recvbuffer(user_t* user);
uint64_t mimc_rtc_dial_call(user_t* user, const char* to_appaccount, const char* appcontent, const int appcontent_len, const char* to_resource);
void mimc_rtc_close_call(user_t* user, uint64_t callid, const char* bye_reason);
int mimc_rtc_send_data(user_t* user, uint64_t callid, const char* data, const int data_len, const data_type_t data_type, const channel_type_t channel_type, const char* ctx, const int ctx_len, const bool can_be_dropped, const data_priority_t data_priority, const unsigned int resend_count);
// deprecated
void mimc_rtc_register_token_fetcher(user_t *user, const char *app_account);
// deprecated
void mimc_rtc_register_online_status_handler(user_t *user, const online_status_handler_t *online_status_handler);
void mimc_rtc_register_rtscall_event_handler(user_t *user, const rtscall_event_handler_t *rtscall_event_handler);
// deprecated
const char* mimc_im_send_message(user_t* user, const char* to_appaccount, const char* payload, const int payload_len, const char* biz_type, const bool is_store);
void mimc_send_online_message(user_t* user,const char* to_appaccount,const char *payload, const int payload_len, const char *biz_type, const bool is_store, char *result, int result_len);
// deprecated
const char* mimc_im_send_group_message(user_t* user, int64_t topicid, const char* payload, const int payload_len, const char* biz_type, const bool is_store);
// deprecated
void mimc_im_register_message_handler(user_t* user, const message_handler_t* message_handler);


void mimc_init(user_t *user, int64_t appid, const char *appaccount, const char *resource, bool is_save_cache,const char *cachepath);
void mimc_fini(user_t *user);
void mimc_register_token_fetcher(user_t *user, void *args, fetch_token_t fetch_token, free_args_t free_args);
void mimc_register_online_status_handler(user_t *user, const online_status_handler_t *online_status_handler);
void mimc_register_message_handler(user_t *user, const message_handler_t *message_handler);
bool mimc_login(user_t *user);
bool mimc_logout(user_t *user);
bool mimc_isonline(user_t *user);
bool mimc_get_token_fetch_succeed(user_t *user);
short mimc_get_token_request_status(user_t *user);
int mimc_get_login_timeout();
/**
 * result：用于回传结果信息，由外部指定存储空间，不关心可不指定
 * result_len：存储空间大小，建议长度64字节
*/
void mimc_send_message(user_t *user, const char *to_appaccount, const char *payload, const int payload_len, const char *biz_type, const bool is_store, char *result, int result_len);
void mimc_send_group_message(user_t *user, int64_t topicid, const char *payload, const int payload_len, const char *biz_type, const bool is_store, char *result, int result_len);

void mimc_enable_p2p(user_t *user, const bool enable);

void mimc_set_app_package(user_t *user, const char* app_package);

void mimc_set_test_packet_loss(user_t *user, int value);

void mimc_set_log_level(log_level_t log_level);
void mimc_register_log_handler(log_handler_t *log_handler);

void mimc_set_base_of_backoff_When_fetch_token(user_t *user, time_unit_t time_unit, long base);
void mimc_set_cap_of_backoff_When_fetch_token(user_t *user, time_unit_t time_unit, long cap);
void mimc_set_base_of_backoff_When_connect_fe(user_t *user, time_unit_t time_unit, long base);
void mimc_set_cap_of_backoff_When_connect_fe(user_t *user, time_unit_t time_unit, long cap);

#ifdef __cplusplus
}
#endif

#endif
