#include <gtest/gtest.h>
#include <mimc/user.h>
#include <json-c/json.h>
#include <test/mimc_message_handler.h>
#include <test/mimc_onlinestatus_handler.h>
#include <test/mimc_tokenfetcher.h>
#include <thread>
#include <chrono>
#include <crypto/base64.h>

using namespace std;

#ifndef STAGING
string appId = "2882303761517613988";
string appKey = "5361761377988";
string appSecret = "2SZbrJOAL1xHRKb7L9AiRQ==";
#else
string appId = "2882303761517479657";
string appKey = "5221747911657";
string appSecret = "PtfBeZyC+H8SIM/UXhZx1w==";
#endif

string urlPrefixForTopic = "https://mimc.chat.xiaomi.net/api/topic/" + appId;

string appAccount1 = "mi153";
string appAccount2 = "mi108";
string appAccount3 = "mi110";

class MIMCTest : public testing::Test
{
protected:
	void SetUp() {

		user1 = new User(atoll(appId.c_str()), appAccount1);
		user2 = new User(atoll(appId.c_str()), appAccount2);
		user3 = new User(atoll(appId.c_str()), appAccount3);

		tokenFetcher1 = new TestTokenFetcher(appId, appKey, appSecret, appAccount1);
		tokenFetcher2 = new TestTokenFetcher(appId, appKey, appSecret, appAccount2);
		tokenFetcher3 = new TestTokenFetcher(appId, appKey, appSecret, appAccount3);

		onlineStatusHandler1 = new TestOnlineStatusHandler();
		onlineStatusHandler2 = new TestOnlineStatusHandler();
		onlineStatusHandler3 = new TestOnlineStatusHandler();

		user1MessageHandler = new TestMessageHandler();
	    user2MessageHandler = new TestMessageHandler();
		user3MessageHandler = new TestMessageHandler();

		user1->registerTokenFetcher(tokenFetcher1);
		user1->registerOnlineStatusHandler(onlineStatusHandler1);
		user1->registerMessageHandler(user1MessageHandler);

		user2->registerTokenFetcher(tokenFetcher2);
		user2->registerOnlineStatusHandler(onlineStatusHandler2);
		user2->registerMessageHandler(user2MessageHandler);

		user3->registerTokenFetcher(tokenFetcher3);
		user3->registerOnlineStatusHandler(onlineStatusHandler3);
		user3->registerMessageHandler(user3MessageHandler);
	}

	void TearDown() {
		delete user1;
		user1 = NULL;
		delete user2;
		user2 = NULL;
		delete user3;
		user3 = NULL;

		delete tokenFetcher1;
		tokenFetcher1 = NULL;

		delete tokenFetcher2;
		tokenFetcher2 = NULL;

		delete tokenFetcher3;
		tokenFetcher3 = NULL;

		delete onlineStatusHandler1;
		onlineStatusHandler1 = NULL;

		delete onlineStatusHandler2;
		onlineStatusHandler2 = NULL;

		delete onlineStatusHandler3;
		onlineStatusHandler3 = NULL;

		delete user1MessageHandler;
		user1MessageHandler = NULL;

		delete user2MessageHandler;
		user2MessageHandler = NULL;

		delete user3MessageHandler;
		user3MessageHandler = NULL;

		while (topicIds.size() > 0){
			int64_t topicId = topicIds.back();
			User* owern = owerns.back();
			dismissTopic(urlPrefixForTopic, appKey, appSecret, topicId, owern);
			topicIds.pop_back();
			owerns.pop_back();
		}
	}

	void testP2PSendOneMessage() {

		user1->login();
		//sleep(2);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		ASSERT_EQ(Online, user1->getOnlineStatus());

		user2->login();
		//sleep(2);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		ASSERT_EQ(Online, user2->getOnlineStatus());

		string payload1 = "你猜我是谁哈哈哈";
		string packetId = user1->sendMessage(user2->getAppAccount(), payload1);
		//usleep(200000);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		string recvPacketId;
		ASSERT_TRUE(user1MessageHandler->pollServerAck(recvPacketId));
		ASSERT_EQ(packetId, recvPacketId);
		//usleep(400000);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));

		MIMCMessage message;
		ASSERT_TRUE(user2MessageHandler->pollMessage(message));

		ASSERT_TRUE(message.getSequence() > 0);
		ASSERT_EQ(user1->getAppAccount(), message.getFromAccount());
		ASSERT_EQ(payload1, message.getPayload());

		ASSERT_FALSE(user2MessageHandler->pollMessage(message));

		//sleep(10);
		std::this_thread::sleep_for(std::chrono::seconds(10));

		ASSERT_TRUE(user1->logout());
		//usleep(400000);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		ASSERT_EQ(Offline, user1->getOnlineStatus());

		ASSERT_TRUE(user2->logout());
		//usleep(400000);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		ASSERT_EQ(Offline, user2->getOnlineStatus());
	}

	void testSendGroupMessage() {
		user1->login();
		//sleep(2);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		ASSERT_EQ(Online, user1->getOnlineStatus());

		user2->login();
		//sleep(2);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		ASSERT_EQ(Online, user2->getOnlineStatus());

		user3->login();
		std::this_thread::sleep_for(std::chrono::seconds(2));
		ASSERT_EQ(Online, user3->getOnlineStatus());

		vector<User*> members;
		members.push_back(user1); 
		members.push_back(user3);
		std::string topicName = "p2t message test";
		int64_t topicId = createTopic(urlPrefixForTopic, topicName, appKey, appSecret, members, user2);

		ASSERT_TRUE(topicId != -1);
		XMDLoggerWrapper::instance()->info("Create topic success, topicId = %lld", topicId);
		topicIds.push_back(topicId);
		owerns.push_back(user2);

		std::string payload = "this is group message, 哈哈哈！";
		string packetId = user1->sendGroupMessage(topicId, payload, "111");

		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		string recvPacketId;
		ASSERT_TRUE(user1MessageHandler->pollServerAck(recvPacketId));
		ASSERT_EQ(packetId, recvPacketId);

		std::this_thread::sleep_for(std::chrono::milliseconds(400));

		MIMCGroupMessage user2Message;
		ASSERT_TRUE(user2MessageHandler->pollGroupMessage(user2Message));
		ASSERT_EQ(payload, user2Message.getPayload());

		MIMCGroupMessage user3Message;
		ASSERT_TRUE(user3MessageHandler->pollGroupMessage(user3Message));
		ASSERT_EQ(payload, user3Message.getPayload());

		MIMCGroupMessage user1Message;
		ASSERT_FALSE(user1MessageHandler->pollGroupMessage(user1Message));

		dismissTopic(urlPrefixForTopic, appKey, appSecret, topicId, user2);

		topicIds.pop_back();
		owerns.pop_back();

		ASSERT_TRUE(user1->logout());
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		ASSERT_EQ(Offline, user1->getOnlineStatus());

		ASSERT_TRUE(user2->logout());
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		ASSERT_EQ(Offline, user2->getOnlineStatus());

		ASSERT_TRUE(user3->logout());
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		ASSERT_EQ(Offline, user3->getOnlineStatus());
	}

	int64_t createTopic(string& url, string& topicName, string& appKey, string& appSecret, vector<User*> &members, User* owner) {
		int64_t topicId = -1;
		string extra = "1111";
		vector<string> memberAccounts;
		string accounts;
		if (members.size() > 0) {
			for (size_t i = 0; i < members.size(); i++) {
				memberAccounts.push_back(members.at(i)->getAppAccount());
			}
			accounts = listToString(memberAccounts);
		}

		vector<std::string> header;
		json_object* dataJson = json_object_new_object();

		json_object_object_add(dataJson, "topicName", json_object_new_string(topicName.c_str()));
		json_object_object_add(dataJson, "accounts", json_object_new_string(accounts.c_str()));
		json_object_object_add(dataJson, "extra", json_object_new_string(extra.c_str()));

		owner->login();
		//sleep(2);
		std::this_thread::sleep_for(std::chrono::seconds(2));

		time_t tv = time(NULL);

		if ((tv & 1) == 0) {
			header.push_back("appKey: " + appKey);
			header.push_back("appSecret: " + appSecret);
			header.push_back("Content-Type: application/json");
			if (!includeChinese(owner->getAppAccount().c_str())) {
				header.push_back("appAccount: " + owner->getAppAccount());
			}else {
				string output;
				ccb::Base64Util::Encode(owner->getAppAccount(), output);   //any problem? support chinese?
				header.push_back("appAccount: " + output);
				header.push_back("appAccountEncode: base64");
			}
			XMDLoggerWrapper::instance()->info("createTopic, on path 0.");
		}else {
			header.push_back("token: " + owner->getToken());
			header.push_back("Content-Type: application/json");
			XMDLoggerWrapper::instance()->info("createTopic, on path 1.");
		}
        std::string body = json_object_get_string(dataJson);
		string fetchString = performCurl(url, header, body, 1);
		json_object* pobj = json_tokener_parse(fetchString.c_str());

		if (pobj == NULL) {
			XMDLoggerWrapper::instance()->error("json_tokener_parse failed, pobj is NULL");  //error?
			return -1;
		}

		json_object* retobj = NULL;
		json_object_object_get_ex(pobj, "code", &retobj);
		int retCode = json_object_get_int(retobj);

		if (retCode != 200) {
			XMDLoggerWrapper::instance()->error("json_tokener_parse failed, retobj is NULL");
			XMDLoggerWrapper::instance()->error(json_object_get_string(pobj));
			XMDLoggerWrapper::instance()->error("retCode=%d", retCode);
			return -1;
		}

		json_object* dataObject = NULL;
		json_object_object_get_ex(pobj, "data", &dataObject);

		json_object* topicInfoObject = NULL;
		json_object_object_get_ex(dataObject, "topicInfo", &topicInfoObject);

		json_object* topicIdObject = NULL;
		json_object_object_get_ex(topicInfoObject, "topicId", &topicIdObject);

		topicId = json_object_get_int64(topicIdObject);
		return topicId;
	}

	string performCurl(string& url, vector<string>& headerList, string& body, int requestType) {
		curl_global_init(CURL_GLOBAL_ALL);
		CURL *curl = curl_easy_init();
		CURLcode res;

		string result;
		if (curl) {
			if (requestType == 1) {
				curl_easy_setopt(curl, CURLOPT_POST, 1);
			}else {
				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			}
			
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

			struct curl_slist *headers = NULL;
			for (size_t i = 0; i < headerList.size(); i++) {
				headers = curl_slist_append(headers, headerList.at(i).c_str()); // weather need anymore?
			}
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

			if (requestType == 1){
				if (body.length() > 0){
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
					curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
				}
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&result));
			}

			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

			res = curl_easy_perform(curl);
			if (res != CURLE_OK) {
				XMDLoggerWrapper::instance()->error("curl perform error, error code is %d", res);
			}
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
		}
		curl_global_cleanup();
		return result;
	}

	string listToString(vector<string>& vecString) {
		if (vecString.size() == 0) {
			return "";
		}

		string ret = "";
		for (size_t i = 0; i < vecString.size(); i++) {
			if (ret == "") {
				ret += vecString.at(i);
				continue;
			}
			ret += "," + vecString.at(i);
		}
		return ret;
	}

	//返回0：无中文，返回1：有中文
	bool includeChinese(const char *str) {
		bool ret = false;
		char c;
		while (c = *str++) {
			//如果字符高位为1且下一字符高位也是1则有中文字符
			if ((c & 0x80) && (*str & 0x80)) {
				ret = true;
				break;
			}
		}
		return ret;
	}

	static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
		string *bodyp = (string *)userp;
		bodyp->append((const char *)contents, size * nmemb);

		return size * nmemb;
	}

	void dismissTopic(string& url, string& appKey, string& appSecurity, int64_t topicId, User* owner) {
		url = url + "/" + to_string(topicId);
		owner->login();
		this_thread::sleep_for(std::chrono::seconds(2));
		vector<std::string> header;

		time_t tv = time(NULL);
		if ((tv & 1) == 0) {
			header.push_back("appKey: " + appKey);
			header.push_back("appSecret: " + appSecret);
			header.push_back("Content-Type: application/json");
			if (!includeChinese(owner->getAppAccount().c_str())) {
				header.push_back("appAccount: " + owner->getAppAccount());
			}
			else {
				string output;
				ccb::Base64Util::Encode(owner->getAppAccount(), output);
				header.push_back("appAccount: " + output);
				header.push_back("appAccountEncode: base64");
			}
			XMDLoggerWrapper::instance()->info("dismissTopic, on path 0");
		}else {
			header.push_back("token: " + owner->getToken());
			header.push_back("Content-Type: application/json");
			XMDLoggerWrapper::instance()->error("dismissTopic, on path 1");
		}
		std::string body = "";
		performCurl(url, header, body, 0);

	}

protected:
    User *user1;
    User *user2;
	User *user3;
	
	TestTokenFetcher* tokenFetcher1;
	TestTokenFetcher* tokenFetcher2;
	TestTokenFetcher* tokenFetcher3;

	TestOnlineStatusHandler* onlineStatusHandler1;
	TestOnlineStatusHandler* onlineStatusHandler2;
	TestOnlineStatusHandler* onlineStatusHandler3;

    TestMessageHandler * user1MessageHandler;
    TestMessageHandler * user2MessageHandler;
	TestMessageHandler * user3MessageHandler;
	
	std::vector<int64_t> topicIds;
	std::vector<User*> owerns;
};

TEST_F(MIMCTest, testP2PSendOneMsg) {
    testP2PSendOneMessage();
}

TEST_F(MIMCTest, testSendGroupMessage) {
	testSendGroupMessage();
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
