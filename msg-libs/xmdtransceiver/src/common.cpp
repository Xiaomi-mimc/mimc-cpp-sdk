#include "common.h"
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <time.h>
#ifdef _LITEOS_USE_
#include "yd_socket.h"
#else
#include <sys/socket.h>
#endif
#ifndef _IOS_MIMC_USE_
#include <endian.h>
#else
#include <machine/endian.h>
#endif
#endif

#include <chrono>
#include <sys/types.h>


uint64_t current_ms() {
#ifdef _WIN32
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch());
	return ms.count();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

uint64_t xmd_current_us() {
#ifdef _WIN32
	std::chrono::microseconds us = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::system_clock::now().time_since_epoch());
	return us.count();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
#endif // _WIN32
}

int64_t currentTimeMillis() {
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch());
	return ms.count();
}



uint64_t rand64() {
#ifdef _WIN32
	std::random_device rd;
	std::mt19937_64 mt(rd());
	std::uniform_int_distribution<uint64_t> dist1(UINT32_MAX, UINT64_MAX);
	std::uniform_int_distribution<uint64_t> dist2(0, UINT32_MAX);
	return (uint64_t)((dist1(mt) << 32) | dist2(mt));
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srandom(tv.tv_sec);
    uint64_t r1 = random();
    srandom(tv.tv_usec);
    uint64_t r2 = random();

    return (r1 | (r2 << 32));
#endif
}

uint64_t rand64(uint32_t ip, uint16_t port) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srandom(tv.tv_sec);
    uint64_t r1 = random();
    srandom((tv.tv_usec ^ ip ^ port));
    uint64_t r2 = random();

    return (r1 | (r2 << 32));
}


uint32_t rand32() {
#ifdef _WIN32
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
	return (uint32_t)dist(mt);
#else
	unsigned int seed = 0x00000000FFFFFFFF & current_ms();
	srandom(seed);
	return random();
#endif
}


uint64_t xmd_ntohll(uint64_t val) {
#ifdef _WIN32
	if (IsBigEndian())
	{
		return val;
	}
	return (((uint64_t)ntohl((int)((val << 32) >> 32))) << 32) | (unsigned int)ntohl((int)(val >> 32));
#else
	if (BYTE_ORDER == LITTLE_ENDIAN)
	{
		return (((uint64_t)ntohl((int)((val << 32) >> 32))) << 32) | (unsigned int)ntohl((int)(val >> 32));
	}
	else if (BYTE_ORDER == BIG_ENDIAN)
	{
		return val;
	}
#endif
}

uint64_t xmd_htonll(uint64_t val) {
#ifdef _WIN32
	if (IsBigEndian())
	{
		return val;
	}
	return (((uint64_t)htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
#else
	if (BYTE_ORDER == LITTLE_ENDIAN)
	{
		return (((uint64_t)htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
	}
	else if (BYTE_ORDER == BIG_ENDIAN)
	{
		return val;
	}
#endif
}

int get_eth0_ipv4(uint32_t *ip) {
#ifdef _WIN32
	char host_name[1024];
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 0);
	if (::WSAStartup(wVersionRequested, &wsaData) != 0) {
		WSACleanup();
		return -1;
	}
	if (gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR)
	{
		WSACleanup();
		return -1;
	}
#ifdef _LITEOS_USE_
	struct hostent *host = yd_gethostbyname(host_name);
#else
	struct hostent *host = gethostbyname(host_name);
#endif
	if (host == NULL)
	{
		WSACleanup();
		return -1;
	}
	//Obtain the computer's IP
	unsigned char b1 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b1;
	unsigned char b2 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b2;
	unsigned char b3 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b3;
	unsigned char b4 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b4;
	WSACleanup();
	uint32_t uint32_ip = 0;
	uint32_ip |= (int)b4 & 0xff;
	uint32_ip = (uint32_ip << 8) | ((int)b3 & 0xff);
	uint32_ip = (uint32_ip << 8) | ((int)b2 & 0xff);
	uint32_ip = (uint32_ip << 8) | ((int)b1 & 0xff);
	*ip = uint32_ip;
#else
	int fd, err;
	struct ifreq ifr;

#ifdef _LITEOS_USE_
	fd = yd_socket(AF_INET, SOCK_DGRAM, 0);
#else
	fd = socket(AF_INET, SOCK_DGRAM, 0);
#endif
	if (fd < 0) {
		return fd;
	}

    ifr.ifr_addr.sa_family = AF_INET;

    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

    err = ioctl(fd, SIOCGIFADDR, &ifr);
    if (err) {
        close(fd);
        return err;
    }

    close(fd);

    *ip =  ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
#endif
    return 0;
}

uint16_t getLocalPort(int fd) {
    struct sockaddr_in  loc_addr;  
    socklen_t len = sizeof(loc_addr);  
    memset(&loc_addr, 0, len); 
    if (-1 == getsockname(fd, (struct sockaddr *)&loc_addr, &len)) {
        return -1;
    }

    return ntohs(loc_addr.sin_port);
}


std::string getLocalStringIp() {
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


uint32_t getLocalIntIp() {
    uint32_t localIp = 0;
#ifdef _WIN32
	WSADATA wsa_Data;
	int wsa_ReturnCode = WSAStartup(0x101, &wsa_Data);

	// Get the local hostname
	char szHostName[255];
	gethostname(szHostName, 255);

	struct hostent *host_entry;
	host_entry = gethostbyname(szHostName);
	localIp = *(struct in_addr *)*host_entry->h_addr_list;
	WSACleanup();
	 
#else
	int number;
	int fd = -1;
	struct ifconf ifc = { 0 }; ///if.h
	struct ifreq buf[16];

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		return 0;
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
				localIp = ((struct sockaddr_in*)(&buf[tmp].ifr_addr))->sin_addr.s_addr;
			}
		}
	}
	close(fd);
#endif // _WIN32

    return localIp;
}



void trans_uint32_t(uint32_t& out, char* in) {
    char* p = (char*)&out;
    for (int i = 0; i < 4; i++) {
        p[i] = in[i];
    }
}

void trans_uint64_t(uint64_t& out, char* in) {
    char* p = (char*)&out;
    for (int i = 0; i < 8; i++) {
        p[i] = in[i];
    }
}

void trans_uint16_t(uint16_t& out, char* in) {
    char* p = (char*)&out;
    for (int i = 0; i < 2; i++) {
        p[i] = in[i];
    }
}




