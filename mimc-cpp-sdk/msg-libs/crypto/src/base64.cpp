//
// Created by zhengsiyao on 6/18/15.
//

#include <openssl/bio.h>
#include <openssl/evp.h>

#include "base64.h"

MSG_CRYPTO_LIB_NS_BEGIN

	int Base64Util::Encode(const std::string &message, std::string &output)
	{
		BIO *bio, *b64;
		int res = 0;

		char *pp;
		long sz;

		b64 = BIO_new(BIO_f_base64());
		bio = BIO_new(BIO_s_mem());
		bio = BIO_push(b64, bio);
		BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
		BIO_write(bio, message.c_str(), (int)message.size());
		if (BIO_flush(bio) != 1) {
			res = -1;
			goto final_label;
		}
		sz = BIO_get_mem_data(bio, &pp);
		output.assign((const char*)pp, (unsigned long)sz);

		final_label:
		BIO_free_all(bio);

		return res;
	}

	int Base64Util::Decode(const std::string &b64message, std::string &output)
	{
#define B64_DECODE_BUF_LEN 512
		BIO *bio, *b64;
		int res = 0;

		unsigned long msg_size = b64message.size();
		if (msg_size < 2) {
			return -1;
		}

		// int decodeLen = CalcDecodeLength(b64message);
		// char *buffer = new char[decodeLen];
		char *buffer;
		char pre_alloc_buffer[B64_DECODE_BUF_LEN];
		if (msg_size > B64_DECODE_BUF_LEN) {
			buffer = new char[msg_size];
		} else {
			buffer = &pre_alloc_buffer[0];
		}

		bio = BIO_new_mem_buf((void *)b64message.c_str(), (int)msg_size);

		b64 = BIO_new(BIO_f_base64());
		bio = BIO_push(b64, bio);
		BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
		int len = BIO_read(bio, buffer, (int)msg_size);
		if (len <= 0) {
			res = -1;
			goto final_label;
		}

		output.assign((const char*)buffer, (unsigned long)len);

		final_label:
		if (msg_size > B64_DECODE_BUF_LEN) {
			delete[] buffer;
		}
		BIO_free_all(bio);

		return res;
	}

	int Base64Util::CalcDecodeLength(const std::string& b64input)
	{
		int len = b64input.size();
		int padding = 0;

		if (b64input[len-1] == '=' && b64input[len-2] == '=') {
			padding = 2;
		} else if (b64input[len-1] == '=') {
			padding = 1;
		}

		return len*3/4 - padding;
	}

MSG_CRYPTO_LIB_NS_END
