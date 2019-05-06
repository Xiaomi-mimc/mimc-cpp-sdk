#ifndef COMMON_H
#define COMMON_H

#include <time.h>
#include <cstdint>
#include <cstdlib>

uint64_t current_ms();
uint64_t rand64();
uint32_t rand32();
uint64_t xmd_ntohll(uint64_t val);
uint64_t xmd_htonll(uint64_t val);
int get_eth0_ipv4(uint32_t *ip);
void trans_uint32_t(uint32_t& out, char* in);
void trans_uint64_t(uint64_t& out, char* in);
void trans_uint16_t(uint16_t& out, char* in);
bool IsBigEndian();


#endif //COMMON_H
