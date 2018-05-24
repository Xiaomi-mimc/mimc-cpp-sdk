#ifndef MIMC_CPP_SDK_UTILS_H
#define MIMC_CPP_SDK_UTILS_H

#include <string>
#include <stdlib.h>
#include <time.h>
#include <sstream>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <string.h>
#include <iomanip>
#include <crypto/base64.h>

class Utils {
public:
    static std::string generateRandomString(int length);
    static std::string int2str(const int64_t &int_temp);
    static std::string hash4SHA1AndBase64(const std::string &plain);
};
#endif //MIMC_CPP_SDK_UTILS_H