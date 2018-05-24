//
// Created by zhengsiyao on 6/17/15.
//

#include <iostream>
#include <string>
#include <ctime>

#include <gtest/gtest.h>
#include <coder_base64_util.h>

#include "rc4_crypto.h"

namespace xm_pass = com::xiaomi::passport;

TEST(RC4Test, Test1)
{
	std::string cipher_base64 {"OcT/xVi/BBrGF8cblPE0lUptUpWbufe8KQAQB5UEVMJBgZXx5p1hW3wgzFQ9AcKAWEZIXKwLNfkUqQo0+swKYteqRNuKaOTDhh0jQ/P9eb/IsHFZCk38BGSK9PNwgc5MupPjHjp3enPuHF09BkDwq8vAfRUquhJIMcHJVCivCFXMbhSFEp+dPkhRSL9cuKIqV1ZSzxqztUxnsv2SYWCG4WgJJFkVll6W0Rh7a5Z+KejfARhFYlpEDei8/sWYDl1jioxhHuX2qmucvWS0FRoQBf2R6K8/kGruORVz5cBU+P9VOKnoxiqY5DKICuEslgsjGvL6WL7eDCLdPcbQH10mUHUqPlMw3CrX214KBB+8EpNuc6ANi7R8QEUf/iYmFF4JBsjKMNPtVso4wxeonBpPzRvUtMSoOBUo4OmDSLF5abzhQ4p0mQ61SOrR8ZXNpA=="};
	std::string cipher;
	xm_pass::client::CoderBase64Util::decode(cipher_base64, cipher);
	// std::cout << cipher_base64 << std::endl;
	// std::cout << cipher << std::endl;
	std::string security_key_base64 {"1ZvvxCYNURrqssknSNP5DQ=="};
	std::string security_key;
	xm_pass::client::CoderBase64Util::decode(security_key_base64, security_key);
	std::string rc4_key = security_key + "_94003969993563781";
	std::string plain_msg;
	ccb::CryptoRC4Util::Decrypt(cipher, plain_msg, rc4_key);
	// std::cout << plain_msg << std::endl;
	std::string deciphered_msg {"<message id=\"94003969993563781\" to=\"52073989@xiaomi.com\" from=\"496977920@xiaomi.com/TlcROVV8\" chid=\"3\"><body encode=\"base64\">eyJtc2dJZCI6OTQwMDM5Njk5OTM1NjM3ODEsInR5cGUiOiJzbXMiLCJwZHUiOiIwMTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5Iiwic2VudFRpbWUiOjE0MzQ0Mzk3NzM2NjZ9</body></message>"};
	EXPECT_EQ(deciphered_msg, plain_msg);
}