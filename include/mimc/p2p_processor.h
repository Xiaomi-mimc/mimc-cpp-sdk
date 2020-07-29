/*
 * Copyright (c) 2019, Xiaomi Inc.
 * All rights reserved.
 *
 * @file p2p_processor.h
 * @brief
 *
 * @author huanghua3@xiaomi.com
 * @date 2019-03-26
 */

#ifndef MIMC_SDK_P2P_PROCESSOR_H
#define MIMC_SDK_P2P_PROCESSOR_H

#include <thread>
#include <mimc/p2p_packet.h>
#include <mimc/user.h>
#include <mimc/rts_data.pb.h>
#include <mimc/rts_signal.pb.h>
#include <mimc/rts_connection_info.h>

#define PPS_DOMAIN "pps.io.mi.com"

#define NETSTATUS_MAX_LOSSRATE 0.1

enum P2PState {
    MPS_P2P_START = 0,
    MPS_NET_DETECT,
    MPS_NET_DETECT_TIMEOUT,
    MPS_NET_DETECT_OK,
    MPS_EXCHANGE_NET_INFO,
    MPS_EXCHANGE_NET_INFO_TIMEOUT,
    MPS_EXCHANGE_NET_INFO_OK,
    MPS_P2P_PUNCH,
    MPS_P2P_PUNCH_TIMEOUT,
    MPS_P2P_INTERNET_OK,
    MPS_P2P_INTRANET_OK,
};


class P2PProcessor {

public:
    static P2PProcessor* getP2PProcessor(uint64_t callId);

    static void startP2P(User* user, uint64_t callId);

    static void stopP2P(User* user, uint64_t callId);

    static void updateP2PServerList(const std::vector<std::string>& p2pServers);

    static void handleNetInfo(User* user, mimc::UserInfo* userInfo);

    static void handleP2PPacket(User* user, char* ip, uint16_t port, char *data, uint32_t size);

    static void handleNetDetectResp(User* user, char* ip, uint16_t port, char *data, uint32_t size);

    static void handleBurrowPacket(User* user, char* ip, uint16_t port, char *data, uint32_t size);

    static void closeP2PConn(User* user, uint64_t callId);

private:
    P2PProcessor(User *user, uint64_t callId);

    ~P2PProcessor();

    void start();

    void stop();

    void run();

    int sendNetDetectRequest();

    int sendNetInfo();

    int sendBurrowRequest(int num);

    void updateP2PState(P2PState p2pState);

    void updateLocalAddr();
    MPNetInfo& getNetInfo(bool peer);
    void setAddrInfo(std::string ip, uint16_t port, bool peer, bool lan);
    bool getAddrInfo(std::string &ip, uint16_t& port, bool peer, bool lan);

    uint64_t createP2PConn(std::string ip, uint16_t port, mimc::PKT_TYPE pkt_type, RtsConnType connType);

    void handleNetDetectResp(MPNetDetectResp *netDetectResp, char* ip, uint16_t port);

    void handleNetInfo(mimc::UserInfo* userInfo);

    void handleBurrowPacket(mimc::BurrowPacket &burrowPacket , char* ip, uint16_t port);

private:
    User *mUser;
    uint64_t mCallId;
    bool mStop;

    std::thread* mThread;

    MPNetInfo mNetInfo;
    MPNetInfo mPeerNetInfo;

    P2PState mP2PState;

    std::map<std::pair<std::string,uint16_t>,struct sockaddr_in> mNetDetectMap;

    XMDLoggerWrapper *mLogger;

};

#endif //MIMC_SDK_P2P_PROCESSOR_H