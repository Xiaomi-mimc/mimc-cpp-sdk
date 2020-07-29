/*
 * Copyright (c) 2019, Xiaomi Inc.
 * All rights reserved.
 *
 * @file p2p_packet.h
 * @brief
 *
 * @author huanghua3@xiaomi.com
 * @date 2019-04-03
 */

#ifndef MIMC_SDK_P2P_PACKET_H
#define MIMC_SDK_P2P_PACKET_H

#include <string.h>
#ifdef _WIN32
#include <stdint.h>
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#else
#include <netinet/in.h>
#endif // _WIN32

#define MP_MSG_NET_INFO "MP_MSG_NET_INFO"

#define MP_VERSION 0xA1
#define MP_MAX_SIZE 512

typedef enum {
    MP_NET_DETECT = 0x01,
    MP_NET_DETECT_RESP = 0x02,
    MP_P2P_PUNCH = 0x03,
    MP_P2P_PUNCH_RESP = 0x04,
}MP_TYPE;

typedef enum {
    MP_NAT_UNKNOWN = 0,
    MP_NAT_SYMMETRIC = 1,
    MP_NAT_PORT_RESTRICTED_CONE = 2,
    MP_NAT_OTHER_CONE = 3,
}MP_NAT_TYPE;

#ifdef _WIN32
PACK(
	struct MPHeader {
	unsigned char version;
	unsigned char type;
	unsigned short dataSize;

	inline void build(unsigned char type, unsigned short size) {
		this->version = MP_VERSION;
		this->type = type;
		this->dataSize = htons(size);
	}

	inline void parse(unsigned char *version, unsigned char *type, unsigned short *dataSize) {
		*version = this->version;
		*type = this->type;
		*dataSize = ntohs(this->dataSize);
	}
});
#else
struct __attribute__((__packed__)) MPHeader {
	unsigned char version;
	unsigned char type;
	unsigned short dataSize;

	inline void build(unsigned char type, unsigned short size) {
		this->version = MP_VERSION;
		this->type = type;
		this->dataSize = htons(size);
	}

	inline void parse(unsigned char *version, unsigned char *type, unsigned short *dataSize) {
		*version = this->version;
		*type = this->type;
		*dataSize = ntohs(this->dataSize);
	}
};
#endif // _WIN32

#ifdef _WIN32
PACK(
	struct MPPacket {
		MPHeader header;
		unsigned char data[0];
	});
#else
struct __attribute__((__packed__)) MPPacket {
	MPHeader header;
	unsigned char data[0];
};
#endif // _WIN32

#ifdef _WIN32
PACK(
	struct MPNetDetect {
		struct sockaddr_in lanAddr;
		uint64_t uuid;
	});
#else
struct __attribute__((__packed__)) MPNetDetect {
	struct sockaddr_in lanAddr;
	uint64_t uuid;
};
#endif

#ifdef _WIN32
PACK(
	struct MPNetDetectResp {
		struct sockaddr_in wanAddr;
		uint64_t uuid;
	});
#else
struct __attribute__((__packed__)) MPNetDetectResp {
	struct sockaddr_in wanAddr;
	uint64_t uuid;
};
#endif // _WIN32

#ifdef _WIN32
PACK(
	struct MPNetInfo {
		struct sockaddr_in lanAddr;
		struct sockaddr_in wanAddr;
		unsigned char natType;//refer to MP_NAT_TYPE

		MPNetInfo() {
			memset(&lanAddr, 0, sizeof(struct sockaddr_in));
			memset(&wanAddr, 0, sizeof(struct sockaddr_in));
			natType = MP_NAT_UNKNOWN;
		}
});
#else
struct __attribute__((__packed__)) MPNetInfo {
	struct sockaddr_in lanAddr;
	struct sockaddr_in wanAddr;
	unsigned char natType;//refer to MP_NAT_TYPE

	MPNetInfo() {
		memset(&lanAddr, 0, sizeof(struct sockaddr_in));
		memset(&wanAddr, 0, sizeof(struct sockaddr_in));
		natType = MP_NAT_UNKNOWN;
	}
};
#endif

#endif //MIMC_SDK_P2P_PACKET_H
