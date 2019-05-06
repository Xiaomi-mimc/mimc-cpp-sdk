#include "rc4_crypto.h"


int CryptoRC4Util::Encrypt(std::string &plain, std::string &cipher, const std::string &key)
{
	if (plain.empty()) {
		return -1;
	}


	plain = cipher;

	return 0;
}

int CryptoRC4Util::Decrypt(std::string &cipher, std::string &plain, const std::string &key)
{
	if (cipher.empty()) {
		return -1;
	}


	plain = cipher;

	return 0;
}

void CryptoRC4Util::DoRC4Crypt(const unsigned char *key,
							   int keylen,
							   const unsigned char *indata,
							   unsigned long datalen,
							   unsigned char *outdata)
{

}

