#include <mimc/utils.h>

std::string Utils::generateRandomString(int length) {
    char s[length + 1];
    srand(time(NULL));
    for (int i = 0; i < length; i++) {
    	switch (rand() % 3) {
    		case 1:
    			s[i] = 'A' + rand() % 26;
    			break;
    		case 2:
    			s[i] = 'a' + rand() % 26;
    			break;
    		default:
    			s[i] = '0' + rand() % 10;
    			break;
    	}
    }
    s[length] = '\0';
    std::string randomString = s;
    return randomString;
}

std::string Utils::int2str(const int64_t &int_temp) {
    std::stringstream stream;
    stream << int_temp;
    return stream.str();
}

std::string Utils::hexToAscii(const char* src, int srcLen) {
    char dest[srcLen*2];
    for (int i = 0,j = 0; i < srcLen; i++) {
        unsigned char highHalf = ((unsigned char)src[i]) >> 4;
        unsigned char lowHalf = ((unsigned char)src[i]) & 0x0F;
        dest[j++] = (highHalf <= 9) ? (highHalf + '0') : (highHalf - 10 + 'A');
        dest[j++] = (lowHalf <= 9) ? (lowHalf + '0') : (lowHalf - 10 + 'A');
    }
    return std::string(dest, srcLen*2);
}

std::string Utils::hash4SHA1AndBase64(const std::string &plain) {
    SHA_CTX c;
    unsigned char md[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)(plain.c_str()), plain.length(), md);
    SHA1_Init(&c);
    SHA1_Update(&c, plain.c_str(), plain.length());
    SHA1_Final(md, &c);
    md[SHA_DIGEST_LENGTH] = '\0';
    OPENSSL_cleanse(&c, sizeof(c));

    std::string sha;
    std::string result;
    sha.assign((char *)md, SHA_DIGEST_LENGTH);
    ccb::Base64Util::Encode(sha, result);

    return result;
}