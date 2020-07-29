#include <mimc/user_c.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
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

char* username1 = "mimc_im_user555";
char* username2 = "mimc_im_user666";

typedef enum {
	SINGLE_MESSAGE = 0,
	GROUP_MESSAGE
} message_type_t;

typedef struct {
	char* packetid;
	int64_t sequence;
	char* from_account;
	char* from_resource;
	char* to_account;
	char* to_resource;
	char* payload;
	int payload_len;
	char* biz_type;
	time_t timestamp;
} message_t;

typedef struct {
	char* packetid;
	int64_t sequence;
	time_t timestamp;
	char* from_account;
	char* from_resource;
	uint64_t topicid;
	char* payload;
	int payload_len;
	char* biz_type;
} group_message_t;

typedef struct {
	int size;
	void* messages;
	int head;
	int tail;
	bool (*push)(void* message_queue, const void* message, message_type_t message_type);
	bool (*pop)(void* message_queue, void* message, message_type_t message_type);
} message_queue_t;

bool message_push(void* queue, const void* msg, message_type_t msg_type) {
	message_queue_t* message_queue = (message_queue_t*)queue;
	if (message_queue == NULL || message_queue->messages == NULL) {
		return false;
	}

	if ((message_queue->tail + 1) % message_queue->size == message_queue->head) {
		printf("message_queue is full, push failed\n");
		return false;
	}

	if (msg_type == SINGLE_MESSAGE) {
		message_t* message_queue_sigle_messages = (message_t*)message_queue->messages;
		const mimc_message_t* message = (const mimc_message_t*)msg;

		message_queue_sigle_messages[message_queue->tail].packetid = (char*)malloc(sizeof(char) * (strlen(message->packetid) + 1));
		strcpy(message_queue_sigle_messages[message_queue->tail].packetid, message->packetid);

		message_queue_sigle_messages[message_queue->tail].sequence = message->sequence;

		message_queue_sigle_messages[message_queue->tail].from_account = (char*)malloc(sizeof(char) * (strlen(message->from_account) + 1));
		strcpy(message_queue_sigle_messages[message_queue->tail].from_account, message->from_account);

		message_queue_sigle_messages[message_queue->tail].from_resource = (char*)malloc(sizeof(char) * (strlen(message->from_resource) + 1));
		strcpy(message_queue_sigle_messages[message_queue->tail].from_resource, message->from_resource);

		message_queue_sigle_messages[message_queue->tail].to_account = (char*)malloc(sizeof(char) * (strlen(message->to_account) + 1));
		strcpy(message_queue_sigle_messages[message_queue->tail].to_account, message->to_account);

		message_queue_sigle_messages[message_queue->tail].to_resource = (char*)malloc(sizeof(char) * (strlen(message->to_resource) + 1));
		strcpy(message_queue_sigle_messages[message_queue->tail].to_resource, message->to_resource);

		message_queue_sigle_messages[message_queue->tail].payload = (char*)malloc(sizeof(char) * message->payload_len);
		strncpy(message_queue_sigle_messages[message_queue->tail].payload, message->payload, message->payload_len);

		message_queue_sigle_messages[message_queue->tail].payload_len = message->payload_len;

		message_queue_sigle_messages[message_queue->tail].biz_type = (char*)malloc(sizeof(char) * (strlen(message->biz_type) + 1));
		strcpy(message_queue_sigle_messages[message_queue->tail].biz_type, message->biz_type);

		message_queue_sigle_messages[message_queue->tail].timestamp = message->timestamp;

		printf("message_queue push single message succeed\n");

	} else if (msg_type == GROUP_MESSAGE) {
		group_message_t* message_queue_group_messages = (group_message_t*)message_queue->messages;
		const mimc_group_message_t* group_message = (const mimc_group_message_t*)msg;

		message_queue_group_messages[message_queue->tail].packetid = (char*)malloc(sizeof(char) * (strlen(group_message->packetid) + 1));
		strcpy(message_queue_group_messages[message_queue->tail].packetid, group_message->packetid);

		message_queue_group_messages[message_queue->tail].sequence = group_message->sequence;

		message_queue_group_messages[message_queue->tail].from_account = (char*)malloc(sizeof(char) * (strlen(group_message->from_account) + 1));
		strcpy(message_queue_group_messages[message_queue->tail].from_account, group_message->from_account);

		message_queue_group_messages[message_queue->tail].from_resource = (char*)malloc(sizeof(char) * (strlen(group_message->from_resource) + 1));
		strcpy(message_queue_group_messages[message_queue->tail].from_resource, group_message->from_resource);

		message_queue_group_messages[message_queue->tail].topicid = group_message->topicid;

		message_queue_group_messages[message_queue->tail].payload = (char*)malloc(sizeof(char) * group_message->payload_len);
		strncpy(message_queue_group_messages[message_queue->tail].payload, group_message->payload, group_message->payload_len);

		message_queue_group_messages[message_queue->tail].payload_len = group_message->payload_len;

		message_queue_group_messages[message_queue->tail].biz_type = (char*)malloc(sizeof(char) * (strlen(group_message->biz_type) + 1));
		strcpy(message_queue_group_messages[message_queue->tail].biz_type, group_message->biz_type);

		message_queue_group_messages[message_queue->tail].timestamp = group_message->timestamp;

		printf("message_queue push group message succeed\n");
	}

	message_queue->tail = (message_queue->tail + 1) % message_queue->size;

	return true;	
}

bool message_pop(void* queue, void* msg, message_type_t msg_type) {
	message_queue_t* message_queue = (message_queue_t*)queue;
	if (message_queue == NULL || message_queue->messages == NULL || msg == NULL) {
		return false;
	}

	if (message_queue->tail == message_queue->head) {
		return false;
	}

	if (msg_type == SINGLE_MESSAGE) {
		message_t* message = (message_t*)msg;
		message_t* message_queue_sigle_messages = (message_t*)message_queue->messages;

		message->packetid = (char*)malloc(sizeof(char) * (strlen(message_queue_sigle_messages[message_queue->head].packetid) + 1));
		strcpy(message->packetid, message_queue_sigle_messages[message_queue->head].packetid);
		free(message_queue_sigle_messages[message_queue->head].packetid);

		message->sequence = message_queue_sigle_messages[message_queue->head].sequence;

		message->from_account = (char*)malloc(sizeof(char) * (strlen(message_queue_sigle_messages[message_queue->head].from_account) + 1));
		strcpy(message->from_account, message_queue_sigle_messages[message_queue->head].from_account);
		free(message_queue_sigle_messages[message_queue->head].from_account);

		message->from_resource = (char*)malloc(sizeof(char) * (strlen(message_queue_sigle_messages[message_queue->head].from_resource) + 1));
		strcpy(message->from_resource, message_queue_sigle_messages[message_queue->head].from_resource);
		free(message_queue_sigle_messages[message_queue->head].from_resource);

		message->to_account = (char*)malloc(sizeof(char) * (strlen(message_queue_sigle_messages[message_queue->head].to_account) + 1));
		strcpy(message->to_account, message_queue_sigle_messages[message_queue->head].to_account);
		free(message_queue_sigle_messages[message_queue->head].to_account);

		message->to_resource = (char*)malloc(sizeof(char) * (strlen(message_queue_sigle_messages[message_queue->head].to_resource) + 1));
		strcpy(message->to_resource, message_queue_sigle_messages[message_queue->head].to_resource);
		free(message_queue_sigle_messages[message_queue->head].to_resource);

		message->payload = (char*)malloc(sizeof(char) * message_queue_sigle_messages[message_queue->head].payload_len);
		strncpy(message->payload, message_queue_sigle_messages[message_queue->head].payload, message_queue_sigle_messages[message_queue->head].payload_len);
		free(message_queue_sigle_messages[message_queue->head].payload);

		message->payload_len = message_queue_sigle_messages[message_queue->head].payload_len;

		message->biz_type = (char*)malloc(sizeof(char) * (strlen(message_queue_sigle_messages[message_queue->head].biz_type) + 1));
		strcpy(message->biz_type, message_queue_sigle_messages[message_queue->head].biz_type);
		free(message_queue_sigle_messages[message_queue->head].biz_type);

		message->timestamp = message_queue_sigle_messages[message_queue->head].timestamp;

		printf("message_queue pop single message succeed\n");
	} else if (msg_type == GROUP_MESSAGE) {
		group_message_t* group_message = (group_message_t*)msg;
		group_message_t* message_queue_group_messages = (group_message_t*)message_queue->messages;

		group_message->packetid = (char*)malloc(sizeof(char) * (strlen(message_queue_group_messages[message_queue->head].packetid) + 1));
		strcpy(group_message->packetid, message_queue_group_messages[message_queue->head].packetid);
		free(message_queue_group_messages[message_queue->head].packetid);

		group_message->sequence = message_queue_group_messages[message_queue->head].sequence;

		group_message->from_account = (char*)malloc(sizeof(char) * (strlen(message_queue_group_messages[message_queue->head].from_account) + 1));
		strcpy(group_message->from_account, message_queue_group_messages[message_queue->head].from_account);
		free(message_queue_group_messages[message_queue->head].from_account);

		group_message->from_resource = (char*)malloc(sizeof(char) * (strlen(message_queue_group_messages[message_queue->head].from_resource) + 1));
		strcpy(group_message->from_resource, message_queue_group_messages[message_queue->head].from_resource);
		free(message_queue_group_messages[message_queue->head].from_resource);

		group_message->topicid = message_queue_group_messages[message_queue->head].topicid;

		group_message->payload = (char*)malloc(sizeof(char) * message_queue_group_messages[message_queue->head].payload_len);
		strncpy(group_message->payload, message_queue_group_messages[message_queue->head].payload, message_queue_group_messages[message_queue->head].payload_len);
		free(message_queue_group_messages[message_queue->head].payload);

		group_message->payload_len = message_queue_group_messages[message_queue->head].payload_len;

		group_message->biz_type = (char*)malloc(sizeof(char) * (strlen(message_queue_group_messages[message_queue->head].biz_type) + 1));
		strcpy(group_message->biz_type, message_queue_group_messages[message_queue->head].biz_type);
		free(message_queue_group_messages[message_queue->head].biz_type);

		group_message->timestamp = message_queue_group_messages[message_queue->head].timestamp;

		printf("message_queue pop group message succeed\n");
	}

	message_queue->head = (message_queue->head + 1) % message_queue->size;
	return true;
}

static message_queue_t message_recieve_queue = {100,0,0,0,message_push,message_pop};
static message_queue_t group_message_recieve_queue = {100,0,0,0,message_push,message_pop};

void status_change(online_status_t online_status, const char* type, const char* reason, const char* desc) {
    printf("In statusChange, status is %d, type is %s, reason is %s, desc is %s\n", online_status, type, reason, desc);
}

static online_status_handler_t online_status_handler = {
    .status_change = status_change
};

void handle_message(const mimc_message_t* packets, uint64_t pkts_len) {
	if(message_recieve_queue.messages == 0 && message_recieve_queue.size > 0) {
		message_recieve_queue.messages = (message_t*)malloc(sizeof(message_t) * message_recieve_queue.size);
		memset(message_recieve_queue.messages, 0, sizeof(message_t) * message_recieve_queue.size);
	}

	int i;
	for (i = 0; i < pkts_len; i++) {
		message_recieve_queue.push(&message_recieve_queue, &packets[i], SINGLE_MESSAGE);
	}
}

void handle_online_message(mimc_message_t* packets){
	if(message_recieve_queue.messages == 0 && message_recieve_queue.size > 0) {
		message_recieve_queue.messages = (message_t*)malloc(sizeof(message_t) * message_recieve_queue.size);
		memset(message_recieve_queue.messages, 0, sizeof(message_t) * message_recieve_queue.size);
	}

	message_recieve_queue.push(&message_recieve_queue, packets, SINGLE_MESSAGE);	
}

void handle_group_message(const mimc_group_message_t* packets, uint64_t pkts_len) {
	if(group_message_recieve_queue.messages == 0 && group_message_recieve_queue.size > 0) {
		group_message_recieve_queue.messages = (group_message_t*)malloc(sizeof(group_message_t) * group_message_recieve_queue.size);
		memset(group_message_recieve_queue.messages, 0, sizeof(group_message_t) * group_message_recieve_queue.size);
	}

	int i;
	for (i = 0; i < pkts_len; i++) {
		group_message_recieve_queue.push(&group_message_recieve_queue, &packets[i], GROUP_MESSAGE);
	}
}

void handle_server_ack(const char* packetid, int64_t sequence, time_t timestamp, const char* desc) {
	printf("receive server ack, packetid is %s, sequence is %lld, timestamp is %ld, desc is %s\n", packetid, sequence, timestamp, desc);
}

void handle_send_msg_timeout(const mimc_message_t* message) {
	printf("send message timeout, packetid is %s, sequence is %lld, to_account is %s, to_resource is %s, payload is %s, payload_len is %d\n", message->packetid, message->sequence, message->to_account, message->to_resource, message->payload, message->payload_len);
}

void handle_send_group_msg_timeout(const mimc_group_message_t* group_message) {
	printf("send group message timeout, packetid is %s, sequence is %lld, topicid is %llu, payload is %s, payload_len is %d\n", group_message->packetid, group_message->sequence, group_message->topicid, group_message->payload, group_message->payload_len);
}

static message_handler_t message_handler = {
    .handle_message = handle_message,
    .handle_group_message = handle_group_message,
    .handle_server_ack = handle_server_ack,
    .handle_send_msg_timeout = handle_send_msg_timeout,
    .handle_send_group_msg_timeout = handle_send_group_msg_timeout,
	.handle_online_message = handle_online_message,
};

static void* check_and_get_message(void* arg) {
	while(true) {
		message_t message;
		memset(&message, 0, sizeof(message));
		if(message_recieve_queue.pop(&message_recieve_queue, &message, SINGLE_MESSAGE)) {
			if (strcmp(message.biz_type, "text") == 0) {
				printf("%s send single message to %s, message is %s\n", message.from_account, message.to_account, message.payload);
			}
		}
		free(message.packetid);
		free(message.from_account);
		free(message.from_resource);
		free(message.to_account);
		free(message.to_resource);
		free(message.payload);
		free(message.biz_type);

		group_message_t group_message;
		memset(&group_message, 0, sizeof(group_message));
		if(group_message_recieve_queue.pop(&group_message_recieve_queue, &group_message, GROUP_MESSAGE)) {
			if (strcmp(group_message.biz_type, "text") == 0) {
				printf("%s send group message to topic %llu, message is %s\n", group_message.from_account, group_message.topicid, group_message.payload);
			}
		}
		free(group_message.packetid);
		free(group_message.from_account);
		free(group_message.from_resource);
		free(group_message.payload);
		free(group_message.biz_type);

		usleep(100000);
	}
}

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
	const char *url = "http://10.38.162.149/api/account/token";
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

void free_args(void *args) {

}

char* getLocalTime() {
	time_t nowtime;
	nowtime = time(NULL);
	static char tmp[64] = {0};
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&nowtime));
	
	return tmp;
}

void debug(const char *msg)
{

	printf("%s D: %s.\n", getLocalTime(), msg);
}

void info(const char *msg)
{
	printf("%s I: %s.\n", getLocalTime(), msg);
}

void warn(const char *msg)
{
	printf("%s W: %s.\n", getLocalTime(), msg);
}

void error(const char *msg)
{
	printf("%s E: %s.\n", getLocalTime(), msg);
}


static log_handler_t log_handler = {
	.debug = debug,
	.info = info,
	.warn = warn,
	.error = error
};


int main() {
	mimc_set_log_level(INFO_T);
	mimc_register_log_handler(&log_handler);


	user_t user1_obj;
	user_t* user1 = &user1_obj;
	mimc_init(user1, atoll(app_id), username1, "01", false, NULL);
	mimc_register_token_fetcher(user1, username1, fetch_token, free_args);
	mimc_register_online_status_handler(user1, &online_status_handler);
    mimc_register_message_handler(user1, &message_handler);
	mimc_set_base_of_backoff_When_fetch_token(user1, SECONDS_T, 1);
	mimc_set_cap_of_backoff_When_fetch_token(user1, MINUTES_T, 1);
	mimc_set_base_of_backoff_When_connect_fe(user1, SECONDS_T, 10);
	mimc_set_cap_of_backoff_When_connect_fe(user1, SECONDS_T, 10);
	time_t user1_login_time = time(NULL);
	mimc_login(user1);

    user_t user2_obj;
	user_t* user2 = &user2_obj;
	mimc_init(user2, atoll(app_id), username2, "03", false, NULL);
	mimc_register_token_fetcher(user2, username2, fetch_token, free_args);
	mimc_register_online_status_handler(user2, &online_status_handler);
    mimc_register_message_handler(user2, &message_handler);
    time_t user2_login_time = time(NULL);
    mimc_login(user2);

    while ((time(NULL) - user1_login_time < 5 && !mimc_isonline(user1)) || (time(NULL) - user1_login_time < 5 && !mimc_isonline(user2))) {
        usleep(100000);
    }

    pthread_t get_message_tid;
    pthread_create(&get_message_tid, NULL, check_and_get_message, NULL);

    int i;
	char send_msg_result[128];
    for (i = 0; i < 5; i++) {
    	char message[20];
    	strcpy(message, "hello world ");
    	char index[4];
    	sprintf(index, "%d", i);
    	strcat(message, index);

		mimc_send_message(user1, username2, message, strlen(message) + 1, "text", true, send_msg_result, 128);
		printf("mimc_send_message result:%s\n", send_msg_result);
		sleep(1);
	}

	sleep(5);
    mimc_send_online_message(user1, username2, "", 0, "", true, send_msg_result, 128);
    printf("mimc_send_online_message end,send_msg_result:%s\n",send_msg_result);

    sleep(15);

	

    pthread_cancel(get_message_tid);
    pthread_join(get_message_tid, NULL);
	mimc_logout(user1);
	mimc_logout(user2);
	mimc_fini(user1);
	mimc_fini(user2);

	return 0;
}