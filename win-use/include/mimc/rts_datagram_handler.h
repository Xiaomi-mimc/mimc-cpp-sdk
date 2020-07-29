/*
 * Copyright (c) 2019, Xiaomi Inc.
 * All rights reserved.
 *
 * @file rts_datagram_handler.h
 * @brief
 *
 * @author huanghua3@xiaomi.com
 * @date 2019-04-01
 */

#ifndef MIMC_CPP_SDK_RTS_DATAGRAM_HANDLER_H
#define MIMC_CPP_SDK_RTS_DATAGRAM_HANDLER_H

#include <DatagramHandler.h>
#include <mimc/user.h>
#include <mimc/p2p_processor.h>

class RtsDatagramHandler : public DatagramRecvHandler {
public:
    RtsDatagramHandler(User* user) {
        this->mUser = user;
    }

    virtual void handle(char* ip, int port, char* data, uint32_t len) {
    }

    virtual void handle(char* ip, int port, char* data, uint32_t len, unsigned char packetType) {
        switch(packetType) {
            case P2P_PACKET:
                P2PProcessor::handleP2PPacket(mUser, ip, port, data, len);
                break;
            default:
                break;
        }
    }

private:
    User* mUser;

};

#endif //MIMC_CPP_SDK_RTS_DATAGRAM_HANDLER_H
