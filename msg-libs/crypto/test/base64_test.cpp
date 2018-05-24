//
// Created by zhengsiyao on 6/18/15.
//

#include <iostream>
#include <string>
#include <ctime>

#include <gtest/gtest.h>
#include "base64.h"

const std::string msg1 {"a"};
const std::string msg1_b64_1 {"YQ"};
const std::string msg1_b64_2 {"YQ="};
const std::string msg1_b64_3 {"YQ=="}; //
const std::string msg1_b64_4 {"YQ==="};

const std::string msg2 {"hello world"};
const std::string msg2_b64_1 {"aGVsbG8gd29ybGQ"};
const std::string msg2_b64_2 {"aGVsbG8gd29ybGQ="}; //
const std::string msg2_b64_3 {"aGVsbG8gd29ybGQ=="};

const std::string msg3 {"aaa"};
const std::string msg3_b64_1 {"YWFh"}; //
const std::string msg3_b64_2 {"YWFh="};
const std::string msg3_b64_3 {"YWFh=="};

const std::string inv_b64_1 {"-&^"};

TEST(BASE64_TEST, EncodeTest)
{
	std::string b64;

	ccb::Base64Util::Encode(msg1, b64);
	ASSERT_EQ(msg1_b64_3, b64);

	ccb::Base64Util::Encode(msg2, b64);
	ASSERT_EQ(msg2_b64_2, b64);

	ccb::Base64Util::Encode(msg3, b64);
	ASSERT_EQ(msg3_b64_1, b64);
}

TEST(BASE64_TEST, DecodeTest)
{
	std::string msg;
	int res;

	res = ccb::Base64Util::Decode(msg1_b64_3, msg);
	std::cout << "msg1_b64_3 decode res = " << res << " msg = " << msg << std::endl;
	ASSERT_EQ(msg1, msg);

	res = ccb::Base64Util::Decode(msg2_b64_2, msg);
	std::cout << "msg2_b64_2 decode res = " << res << " msg = " << msg << std::endl;
	ASSERT_EQ(msg2, msg);

	res = ccb::Base64Util::Decode(msg3_b64_1, msg);
	std::cout << "msg3_b64_1 decode res = " << res << " msg = " << msg << std::endl;
	ASSERT_EQ(msg3, msg);

	res = ccb::Base64Util::Decode(inv_b64_1, msg);
	ASSERT_NE(0, res);

}