#ifndef COMMON_H
#define COMMON_H

#ifdef _WIN32
#include <ws2tcpip.h>
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h> //?
#endif // _WIN32

#include <time.h>
#include <cstdint>
#include <cstdlib>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>


uint64_t current_ms();
uint64_t xmd_current_us();
int64_t currentTimeMillis();
uint64_t rand64();
uint32_t rand32();
uint64_t xmd_ntohll(uint64_t val);
uint64_t xmd_htonll(uint64_t val);
int get_eth0_ipv4(uint32_t *ip);
std::string getLocalStringIp();
uint32_t getLocalIntIp();
uint16_t getLocalPort(int fd);
void trans_uint32_t(uint32_t& out, char* in);
void trans_uint64_t(uint64_t& out, char* in);
void trans_uint16_t(uint16_t& out, char* in);
bool IsBigEndian();
uint32_t covert_ip(char* ip);


#endif //COMMON_H
