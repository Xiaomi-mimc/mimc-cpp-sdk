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

#ifdef WIN_USE_DLL
#ifdef MIMCAPI_EXPORTS
#define MIMCAPI __declspec(dllexport)
#else
#define MIMCAPI __declspec(dllimport)
#endif // MIMCAPI_EXPORTS
#else
#define MIMCAPI
#endif

#ifdef _WIN32
class MIMCAPI Base64Util {
#else
class Base64Util {
#endif // _WIN32

	
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
