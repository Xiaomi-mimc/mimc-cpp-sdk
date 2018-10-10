#ifndef MIMC_CPP_SDK_UTILS_H
#define MIMC_CPP_SDK_UTILS_H

#include <string>
#include <cstdlib>
#include <time.h>
#include <sstream>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <string.h>
#include <crypto/base64.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

class Utils {
public:
    static std::string generateRandomString(int length);
    static long generateRandomLong();
    static std::string int2str(const int64_t &int_temp);
    static std::string hexToAscii(const char* src, int srcLen);
    static std::string hash4SHA1AndBase64(const std::string &plain);
    static std::string getLocalIp();
};
#endif //MIMC_CPP_SDK_UTILS_H