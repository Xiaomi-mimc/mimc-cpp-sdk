#include "common.h"
#include <string.h>
#include <chrono>
#include <random>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#endif

uint64_t current_ms() {
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch());
	return ms.count();
}

uint64_t rand64() {
	std::random_device rd;
	std::mt19937_64 mt(rd());
	std::uniform_int_distribution<uint64_t> dist1(UINT32_MAX, UINT64_MAX);
	std::uniform_int_distribution<uint64_t> dist2(0, UINT32_MAX);
	return (uint64_t)((dist1(mt) << 32) | dist2(mt));
}


uint32_t rand32() {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
	return (uint32_t)dist(mt);
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
	struct hostent *host = gethostbyname(host_name);
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

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return fd;
	}

	ifr.ifr_addr.sa_family = AF_INET;

	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

	err = ioctl(fd, SIOCGIFADDR, &ifr);
	if (err) {
		return err;
	}

	close(fd);
	*ip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
#endif
	return 0;
}

bool IsBigEndian()
{
	short word = 0x4321;
	if ((*(char *)& word) != 0x21)
		return true;
	else
		return false;
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