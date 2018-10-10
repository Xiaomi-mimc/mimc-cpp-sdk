#ifndef CRYPTO_RC4_CRYPTO_H
#define CRYPTO_RC4_CRYPTO_H

#include <string>
#include <openssl/rc4.h>
#include "common.h"

class CryptoRC4Util {
public:
	static int Decrypt(const std::string &cipher, /* out */std::string &plain,
					   const std::string &key);

	static int Encrypt(const std::string &plain, /* out */std::string &cipher,
					   const std::string &key);

private:
	CryptoRC4Util()
	{
	}

	static void DoRC4Crypt(const unsigned char *key,
						   int keylen,
						   const unsigned char *indata,
						   unsigned long datalen,
						   unsigned char *outdata);
};



#endif //CRYPTO_RC4_CRYPTO_H
