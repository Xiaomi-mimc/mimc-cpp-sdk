#include <mimc/utils.h>
#include <chrono>
#include <random>

#ifdef _WIN32
#include <winsock.h>
#pragma comment(lib,"ws2_32.lib")
#endif // _WIN32

std::string Utils::generateRandomString(int length) {
    char* s= new char[length + 1];
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
	delete[] s;
    return randomString;
}

int64_t Utils::generateRandomLong() {
#ifdef _WIN32
	std::random_device rd;
	std::mt19937_64 mt(rd());
	std::uniform_int_distribution<uint64_t> dist1(0, UINT64_MAX);
	return dist1(mt) & 0x7fffffffffffffff;
#else
	srand(currentTimeMicros());
	int64_t l = rand() % 128;
	for (int i = 0; i < 7; i++) {
		srand(currentTimeMicros());
		l = l << 8 | rand() % 256;
	}

	return l;
#endif // _WIN32
}

std::string Utils::int2str(const int64_t& int_temp) {
    std::stringstream stream;
    stream << int_temp;
    return stream.str();
}

std::string Utils::hexToAscii(const char* src, int srcLen) {
    char* dest = new char[srcLen * 2];
    for (int i = 0, j = 0; i < srcLen; i++) {
        unsigned char highHalf = ((unsigned char)src[i]) >> 4;
        unsigned char lowHalf = ((unsigned char)src[i]) & 0x0F;
        dest[j++] = (highHalf <= 9) ? (highHalf + '0') : (highHalf - 10 + 'A');
        dest[j++] = (lowHalf <= 9) ? (lowHalf + '0') : (lowHalf - 10 + 'A');
    }
	std::string res(dest, srcLen * 2);
	delete[] dest;
    return res;
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
#ifdef _WIN32
	WSADATA wsa_Data;
	int wsa_ReturnCode = WSAStartup(0x101, &wsa_Data);

	// Get the local hostname
	char szHostName[255];
	gethostname(szHostName, 255);

	struct hostent *host_entry;
	host_entry = gethostbyname(szHostName);
	localIp = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);
	WSACleanup();
	 
#else
	int number;
	int fd = -1;
	const char* ip = NULL;
	struct ifconf ifc = { 0 }; ///if.h
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
#endif // _WIN32

    return localIp;
}


int64_t Utils::currentTimeMillis() {
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch());
	return ms.count();
}

int64_t Utils::currentTimeMicros() {
	std::chrono::microseconds microS = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::system_clock::now().time_since_epoch());
	return microS.count();
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
        return false;
    }

    pszDir = strdup(pDir.c_str());
    if (!pszDir) {
        return false;    
    }

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
                    free(pszDir);
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
