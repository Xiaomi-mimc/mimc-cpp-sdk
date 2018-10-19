#include "common.h"
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>


uint64_t current_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000  +  tv.tv_usec / 1000;
}

uint64_t rand64() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srandom(tv.tv_sec);
    uint64_t r1 = random();
    srandom(tv.tv_usec);
    uint64_t r2 = random();

    return (r1 | (r2 << 32));
}


uint32_t rand32() {
    unsigned int seed = 0x00000000FFFFFFFF & current_ms();
    srandom(seed);
    return random();
}


uint64_t xmd_ntohll(uint64_t val) {
   if (BYTE_ORDER == LITTLE_ENDIAN)
   {
       return (((uint64_t)ntohl((int)((val << 32) >> 32))) << 32) | (unsigned int)ntohl((int)(val >> 32));
   }
   else if (BYTE_ORDER == BIG_ENDIAN)
   {
       return val;
   }
}

uint64_t xmd_htonll(uint64_t val) {
    if (BYTE_ORDER == LITTLE_ENDIAN)
    {
        return (((uint64_t)htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
    }
    else if (BYTE_ORDER == BIG_ENDIAN)
    {
        return val;
    }
}

int get_eth0_ipv4(uint32_t *ip) {
    int fd, err;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return fd;
    }

    ifr.ifr_addr.sa_family = AF_INET;

    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

    err = ioctl(fd, SIOCGIFADDR, &ifr);
    if (err) {
        return err;
    }

    close(fd);

    *ip =  ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
    return 0;
}




