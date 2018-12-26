#ifndef MIMC_CPP_SDK_UTILS_H
#define MIMC_CPP_SDK_UTILS_H

#include <string>
#include <cstdlib>
#include <cstdio>
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
#include <sys/time.h>
#include <net/if.h>

#ifdef __linux
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#endif

#ifdef __linux
#define ACCESS access
#define MKDIR(a) mkdir((a), 0755)
#endif
#ifdef _WIN32
#define ACCESS _access
#define MKDIR(a) _mkdir((a))
#endif

const int MAXPATHLEN = 80;

class Utils {
public:
    static std::string generateRandomString(int length);
    static int64_t generateRandomLong();
    static std::string int2str(const int64_t& int_temp);
    static std::string hexToAscii(const char* src, int srcLen);
    static std::string hash4SHA1AndBase64(const std::string& plain);
    static std::string getLocalIp();
    static int64_t currentTimeMillis();
    static int64_t currentTimeMicros();
    static void getCwd(char* currentPath, int maxLen);
    static bool createDirIfNotExist(const std::string& pDir);
    static char* ltoa(int64_t value, char* str);
};
#endif //MIMC_CPP_SDK_UTILS_H