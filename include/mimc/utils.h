#ifndef MIMC_CPP_SDK_UTILS_H
#define MIMC_CPP_SDK_UTILS_H

#include <string>
#include <cstdlib>
#include <cstdio>
#include <time.h>
#include <sstream>
#include <string.h>
#include <crypto/base64.h>

#ifdef _WIN32
#include "pthread.h"
#include "openssl/sha.h"
#include "openssl/crypto.h"
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#endif

#ifdef __linux
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef _IOS_MIMC_USE_
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

#ifdef _IOS_MIMC_USE_
#define ACCESS access
#define MKDIR(a) mkdir((a), 0755)
#endif

const int MAXPATHLEN = 80;

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
class MIMCAPI Utils {
#else
class Utils {
#endif // _WIN32

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