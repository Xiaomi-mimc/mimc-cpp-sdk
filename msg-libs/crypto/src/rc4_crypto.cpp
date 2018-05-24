//
// Created by zhengsiyao on 6/16/15.
//

#include "rc4_crypto.h"

MSG_CRYPTO_LIB_NS_BEGIN

	int CryptoRC4Util::Encrypt(const std::string &plain, std::string &cipher, const std::string &key)
	{
		if (plain.empty()) {
			return -1;
		}

		if (key.empty()) {
			return -1;
		}

		unsigned long content_size = plain.size();
		unsigned char* output = new unsigned char[content_size];
		DoRC4Crypt((const unsigned char*)key.c_str(), (int)key.size(),
				   (const unsigned char*)plain.c_str(), content_size,
				   output);
		cipher.assign((const char*)output, content_size);
		delete[] output;

		return 0;
	}

	int CryptoRC4Util::Decrypt(const std::string &cipher, std::string &plain, const std::string &key)
	{
		if (cipher.empty()) {
			return -1;
		}

		if (key.empty()) {
			return -1;
		}

		unsigned long content_size = cipher.size();
		unsigned char* output = new unsigned char[content_size];
		DoRC4Crypt((const unsigned char*)key.c_str(), (int)key.size(),
				   (const unsigned char*)cipher.c_str(), content_size,
				   output);
		plain.assign((const char*)output, content_size);
		delete[] output;

		return 0;
	}

	void CryptoRC4Util::DoRC4Crypt(const unsigned char *key,
								   int keylen,
								   const unsigned char *indata,
								   unsigned long datalen,
								   unsigned char *outdata)
	{
		RC4_KEY rc4_key;
		RC4_set_key(&rc4_key, keylen, key);
		RC4(&rc4_key, datalen, indata, outdata);
	}

MSG_CRYPTO_LIB_NS_END