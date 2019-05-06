//
// Created by zhengsiyao on 6/18/15.
//

#ifndef CRYPTO_BASE64_H
#define CRYPTO_BASE64_H

#include <iostream>
#include <string>
#include <openssl/rc4.h>

#include "common.h"

MSG_CRYPTO_LIB_NS_BEGIN

	class Base64Util {
	public:
		static int Encode(const std::string& message, std::string& output);
		static int Decode(const std::string& b64message, std::string& output);

	private:
		Base64Util()
		{
		}

		static int CalcDecodeLength(const std::string& b64input);
	};

MSG_CRYPTO_LIB_NS_END

#endif //CRYPTO_BASE64_H
