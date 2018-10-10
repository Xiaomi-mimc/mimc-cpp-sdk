#ifndef COMMON_H
#define COMMON_H

#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>


uint64_t current_ms();
uint64_t rand64();
uint32_t rand32();
uint64_t ntohll(uint64_t val);
uint64_t htonll(uint64_t val);
int get_eth0_ipv4(uint32_t *ip);



#endif //COMMON_H
