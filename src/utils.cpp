#include <mimc/utils.h>

std::string Utils::generateRandomString(int length) {
    char s[length + 1];
    srand(currentTimeMicros());
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

int64_t Utils::generateRandomLong() {
    srand(currentTimeMicros());
    int64_t l = rand() % 128;
    for (int i = 0; i < 7; i++) {
        srand(currentTimeMicros());
        l = l << 8 | rand() % 256;
    }
    return l;
}

std::string Utils::int2str(const int64_t& int_temp) {
    std::stringstream stream;
    stream << int_temp;
    return stream.str();
}

std::string Utils::hexToAscii(const char* src, int srcLen) {
    char dest[srcLen * 2];
    for (int i = 0, j = 0; i < srcLen; i++) {
        unsigned char highHalf = ((unsigned char)src[i]) >> 4;
        unsigned char lowHalf = ((unsigned char)src[i]) & 0x0F;
        dest[j++] = (highHalf <= 9) ? (highHalf + '0') : (highHalf - 10 + 'A');
        dest[j++] = (lowHalf <= 9) ? (lowHalf + '0') : (lowHalf - 10 + 'A');
    }
    return std::string(dest, srcLen * 2);
}

std::string Utils::hash4SHA1AndBase64(const std::string& plain) {
    SHA_CTX c;
    unsigned char md[SHA_DIGEST_LENGTH + 1];
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

std::string Utils::getLocalIp() {
    const char* const loop_ip = "127.0.0.1";
    std::string localIp = loop_ip;
    int number;
    int fd = -1;
    const char* ip = NULL;
    struct ifconf ifc = {0}; ///if.h
    struct ifreq buf[16];

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return "";
    }
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (ioctl(fd, SIOCGIFCONF, (char*)&ifc) == 0) //ioctl.h
    {
        number = ifc.ifc_len / sizeof(struct ifreq);
        int tmp = number;
        while (tmp-- > 0)
        {
            if ((ioctl(fd, SIOCGIFADDR, (char*)&buf[tmp])) == 0)
            {
                ip = (inet_ntoa(((struct sockaddr_in*)(&buf[tmp].ifr_addr))->sin_addr));
                if (strcmp(ip, "127.0.0.1") != 0) {
                    printf("IP Address:%s\n", ip);
                    localIp = ip;
                    break;
                }
            }
        }
    }
    close(fd);

    return localIp;
}

int64_t Utils::currentTimeMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int64_t Utils::currentTimeMicros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void Utils::getCwd(char* currentPath, int maxLen) {
    if (currentPath != NULL) {
        getcwd(currentPath, maxLen);
    }
}

bool Utils::createDirIfNotExist(const std::string& pDir) {
    int iRet = 0;
    int iLen = 0;
    char* pszDir = NULL;

    if (pDir.empty()) {
        return true;
    }

    pszDir = strdup(pDir.c_str());
    iLen = strlen(pszDir);

    for (int i = 1; i < iLen; i++) {
        if (pszDir[i] == '/' || i == iLen - 1) {
            if (i < iLen - 1) {
                pszDir[i] = '\0';
            }

            iRet = ACCESS(pszDir, 0);
            if (iRet != 0) {
                iRet = MKDIR(pszDir);
                if (iRet != 0) {
                    return false;
                }
            }

            pszDir[i] = '/';
        }
    }

    free(pszDir);

    return true;
}

char* Utils::ltoa(int64_t value, char* str) {
    sprintf(str, "%llu", value);
    return str;
}