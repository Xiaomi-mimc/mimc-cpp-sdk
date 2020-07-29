/*
 * Copyright (c) 2019, Xiaomi Inc.
 * All rights reserved.
 *
 * @file p2p_processor.cpp
 * @brief
 *
 * @author huanghua3@xiaomi.com
 * @date 2019-03-26
 */
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif // _WIN32

#include "mimc/p2p_processor.h"
#include "mimc/p2p_callsession.h"
#include "XMDTransceiver.h"
#include <zconf.h>
#include <cstdlib>
#include <string>
#ifndef _IOS_MIMC_USE_
#include <malloc.h>
#endif

using namespace std;
using namespace mimc;

#define PROCESS_SLEEP_MS 10
#define MAX_NET_DETECT_COUNT (1000 / PROCESS_SLEEP_MS)
#define MAX_EXCHANGE_NET_INFO_COUNT (2000 / PROCESS_SLEEP_MS)
#define MAX_P2P_PUNCH_COUNT (5000 / PROCESS_SLEEP_MS)

#define MAX_TRY_PORT_COUNT 20

#define P2P_SERVER_PORT 32037

#define MAX_P2P_SERVER_COUNT 3

pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;

map<uint64_t, P2PProcessor*> gP2PProcessors;

vector<string> gP2PServers = {"120.92.76.186","120.92.88.59"};
pthread_rwlock_t gP2PServersMutex = PTHREAD_RWLOCK_INITIALIZER;

bool addrCompare(struct sockaddr_in *addr1, struct sockaddr_in *addr2) {
    if((addr1->sin_addr.s_addr == addr2->sin_addr.s_addr) && (addr1->sin_port == addr2->sin_port) )
        return true;
    else
        return false;
}

int strToInaddr(const char* straddr, struct in_addr *inaddr) {
    return inet_pton(AF_INET, straddr, inaddr);
}

int inaddrToStr(struct in_addr *inaddr, char *straddr) {
    const char *p = inet_ntop(AF_INET, inaddr, straddr, INET_ADDRSTRLEN);
    if (!p) {
        return -1;
    }
    return 0;
}

uint64_t getTimeInMS(void)
{
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

P2PProcessor::P2PProcessor(User *user, uint64_t callId) : mUser(user), mCallId(callId) {
    mStop = false;
    mP2PState = MPS_P2P_START;
    mThread = NULL;
    mLogger = XMDLoggerWrapper::instance();
}

P2PProcessor::~P2PProcessor() {
    mLogger->debug("P2PProcessor::~P2PProcessor, uuid:%llu, callId:%llu", mUser->getUuid(), mCallId);
}

P2PProcessor* P2PProcessor::getP2PProcessor(uint64_t callId) {
    P2PProcessor *p2pProcessor = NULL;
    pthread_mutex_lock(&gMutex);
    if (gP2PProcessors.count(callId) > 0) {
        p2pProcessor = gP2PProcessors.at(callId);
    }
    pthread_mutex_unlock(&gMutex);
    return p2pProcessor;
}

void P2PProcessor::startP2P(User* user, uint64_t callId) {
    P2PProcessor *p2pProcessor = NULL;
    pthread_mutex_lock(&gMutex);
    if (gP2PProcessors.count(callId) > 0) {
        p2pProcessor = gP2PProcessors.at(callId);
    } else {
        p2pProcessor = new P2PProcessor(user, callId);
        gP2PProcessors[callId] = p2pProcessor;
    }
    pthread_mutex_unlock(&gMutex);
    p2pProcessor->start();
}

void P2PProcessor::stopP2P(User* user, uint64_t callId) {
    //XMDLoggerWrapper::instance()->debug("P2PProcessor::stopP2P start,callId:%llu",callId);
    P2PProcessor *p2pProcessor = NULL;
    pthread_mutex_lock(&gMutex);
    if (gP2PProcessors.count(callId) > 0) {
        p2pProcessor = gP2PProcessors.at(callId);
        p2pProcessor->stop();

        gP2PProcessors.erase(callId);
        delete p2pProcessor;
        //malloc_trim(0);
    }
    pthread_mutex_unlock(&gMutex);
    //XMDLoggerWrapper::instance()->debug("P2PProcessor::stopP2P end,callId:%llu",callId);
}

void P2PProcessor::start() {
    mStop = false;
    mThread = new thread(&P2PProcessor::run, this);
}

void P2PProcessor::stop() {
    mLogger->info("P2PProcessor::stop, uuid:%llu, callId:%llu", mUser->getUuid(), mCallId);
    mStop = true;
    if (mThread != NULL) {
        mThread->join();
        delete mThread;
        mThread = NULL;
    }
    mLogger->info("P2PProcessor::stop end, uuid:%llu, callId:%llu", mUser->getUuid(), mCallId);
}

void P2PProcessor::run() {
    //mLogger->info("P2PProcessor::run start, uuid:%llu, callId:%llu", mUser->getUuid(), mCallId);

    int netDetectCount = 0;
    int exchangeNetInfoCount = 0;
    int p2pPunchCount = 0;

    int sendBurrowNum = 0;

    int p2pErrorState = 0;

    while (!mStop) {
        switch(mP2PState) {
            case MPS_P2P_START:
            case MPS_NET_DETECT:
            {
                if(netDetectCount < MAX_NET_DETECT_COUNT) {
                    if(netDetectCount++ % 10 == 0)
                        sendNetDetectRequest();
                }
                else {
                    if (mNetInfo.wanAddr.sin_port == 0) {
                        p2pErrorState |= (1 << 0);
                        BindRelayResponse* bindRelayResponse = mUser->getBindRelayResponse();
                        setAddrInfo(const_cast<string &>(bindRelayResponse->internet_ip()), bindRelayResponse->internet_port(), false, false);
                        if (mPeerNetInfo.wanAddr.sin_port != 0)
                            updateP2PState(MPS_EXCHANGE_NET_INFO_OK);
                        else
                            updateP2PState(MPS_NET_DETECT_TIMEOUT);
                    }
                    else {
                        updateP2PState(MPS_NET_DETECT_OK);
                    }
                }
                break;
            }
            case MPS_NET_DETECT_TIMEOUT:
            case MPS_NET_DETECT_OK:
            case MPS_EXCHANGE_NET_INFO:
            {
                if (exchangeNetInfoCount < MAX_EXCHANGE_NET_INFO_COUNT) {
                    if (exchangeNetInfoCount++ % 40 == 0)
                        sendNetInfo();
                }
                else {
                    p2pErrorState |= (1 << 1);
                    updateP2PState(MPS_EXCHANGE_NET_INFO_TIMEOUT);
                }
                break;
            }
            case MPS_EXCHANGE_NET_INFO_TIMEOUT:
            case MPS_EXCHANGE_NET_INFO_OK:
            case MPS_P2P_PUNCH:
            {
                if (p2pPunchCount < MAX_P2P_PUNCH_COUNT) {
                    if (p2pPunchCount <= 100) {
                        if(p2pPunchCount % 20 == 0)
                            sendBurrowRequest(sendBurrowNum++);
                    }
                    else {
                        if(p2pPunchCount % 50 == 0)
                            sendBurrowRequest(sendBurrowNum++);
                    }
                    p2pPunchCount++;
                }
                else {
                    p2pErrorState = (1 << 2);
                    updateP2PState(MPS_P2P_PUNCH_TIMEOUT);
                }
                break;
            }
            case MPS_P2P_PUNCH_TIMEOUT:
            case MPS_P2P_INTRANET_OK:
            case MPS_P2P_INTERNET_OK:
                mStop = true;
                break;
            default:
                break;
        }
        //usleep(PROCESS_SLEEP_MS * 1000);
		std::this_thread::sleep_for(std::chrono::milliseconds(PROCESS_SLEEP_MS));
    }

    int p2pResult = -p2pErrorState;//~p2pErrorState+1;
    if(mP2PState == MPS_P2P_INTRANET_OK) {
        p2pResult = P2P_INTRANET;
    }
    else if (mP2PState == MPS_P2P_INTERNET_OK) {
        p2pResult = P2P_INTERNET;
    }
    mUser->getRTSCallEventHandler()->onP2PResult(mCallId, p2pResult, mNetInfo.natType, mPeerNetInfo.natType);
    mLogger->info("P2PProcessor::run end, uuid:%llu, callId:%llu, p2pState:%d, p2pResult:%d", mUser->getUuid(), mCallId, mP2PState, p2pResult);
}

MPNetInfo& P2PProcessor::getNetInfo(bool peer) {
    return peer ? mPeerNetInfo : mNetInfo;
}

void P2PProcessor::updateP2PServerList(const std::vector<std::string>& p2pServers) {

    int serverCount = p2pServers.size();
    if (serverCount == 0)
        return;

    int selectedServerCount = serverCount;
    int randomIndex = 0;
    if(serverCount > MAX_P2P_SERVER_COUNT) {
        selectedServerCount = MAX_P2P_SERVER_COUNT;
        srand(time(NULL));
        randomIndex = rand();
    }

    vector<string> newP2PServers;
    int curServerCount = 0;
    for (curServerCount = 0; curServerCount < selectedServerCount; curServerCount++) {
        newP2PServers.push_back(p2pServers[randomIndex % serverCount]);
        randomIndex++;
    }

    pthread_rwlock_wrlock(&gP2PServersMutex);
    gP2PServers.swap(newP2PServers);
    pthread_rwlock_unlock(&gP2PServersMutex);
}

void P2PProcessor::setAddrInfo(std::string ip, uint16_t port, bool peer, bool lan) {

    struct sockaddr_in *sockAddr;
    if (peer && lan) {
        sockAddr = &(mPeerNetInfo.lanAddr);
    } else if (peer && !lan) {
        sockAddr = &(mPeerNetInfo.wanAddr);
    } else if (!peer && lan) {
        sockAddr = &(mNetInfo.lanAddr);
    } else {
        sockAddr = &(mNetInfo.wanAddr);
    }

    sockAddr->sin_family = AF_INET;
    sockAddr->sin_addr.s_addr = inet_addr(ip.c_str());
    sockAddr->sin_port = htons(port);
}

bool P2PProcessor::getAddrInfo(std::string &ip, uint16_t& port, bool peer, bool lan) {

    struct sockaddr_in *sockAddr;
    if (peer && lan) {
        sockAddr = &(mPeerNetInfo.lanAddr);
    } else if (peer && !lan) {
        sockAddr = &(mPeerNetInfo.wanAddr);
    } else if (!peer && lan) {
        sockAddr = &(mNetInfo.lanAddr);
    } else {
        sockAddr = &(mNetInfo.wanAddr);
    }

    char ipBuf[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &(sockAddr->sin_addr), ipBuf, INET_ADDRSTRLEN);

    int ipLen = 0;
    for (ipLen = 0; ipLen < INET_ADDRSTRLEN; ipLen++) {
        if (ipBuf[ipLen] == '\0') {
            break;
        }
    }

    std::string ipStr(ipBuf, ipLen);
    if(ipStr.empty() || (ipStr == "0.0.0.0") || (ipStr == "127.0.0.1") || (sockAddr->sin_port == 0)) {
        mLogger->debug("get addr info(%d,%d) failed, addr %s:%d", peer, lan, ipStr.c_str(), ntohs(sockAddr->sin_port));
        return false;
    }
    ip = ipStr;
    port = ntohs(sockAddr->sin_port);
    return true;
}

void P2PProcessor::updateLocalAddr() {
    if(mNetInfo.lanAddr.sin_port == 0) {
        std::string localIp = "127.0.0.1";
        uint16_t localPort = 0;
        if ((mUser->getXmdTransceiver()->getLocalInfo(localIp, localPort) < 0) || localIp.empty()
            || localIp == "0.0.0.0" || localIp == "127.0.0.1" ) {
            mLogger->debug("XMDTransceiver::getLocalInfo result(%s:%d)", localIp.c_str(),localPort);

            pthread_rwlock_rdlock(&gP2PServersMutex);
            if(gP2PServers.size() > 0)
            {
                string serverIp = gP2PServers[0];
                pthread_rwlock_unlock(&gP2PServersMutex);
                uint16_t serverPort = P2P_SERVER_PORT;

                int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                if (sockfd >= 0) {
                    socklen_t addrLen = sizeof(struct sockaddr_in);
                    struct sockaddr_in localAddr;
                    memset(&localAddr,0,addrLen);
                    struct sockaddr_in serverAddr;
                    serverAddr.sin_family = AF_INET;
                    serverAddr.sin_addr.s_addr = inet_addr(serverIp.c_str());
                    serverAddr.sin_port = htons(serverPort);
                    if((connect(sockfd, (const struct sockaddr *)&serverAddr, addrLen) == 0)
                       && (getsockname(sockfd, (struct sockaddr *) &localAddr, &addrLen) >= 0)) {
                        mNetInfo.lanAddr.sin_addr = localAddr.sin_addr;
                        mNetInfo.lanAddr.sin_port = htons(localPort);
#ifdef _WIN32
						closesocket(sockfd);
#else
						close(sockfd);
#endif // _WIN32
                        return;
                    }
#ifdef _WIN32
					closesocket(sockfd);
#else
					close(sockfd);
#endif // _WIN32
                }
            } else {
                pthread_rwlock_unlock(&gP2PServersMutex);
            }
        }

        setAddrInfo(localIp, localPort, false, true);
    }
}

void P2PProcessor::updateP2PState(P2PState p2pState) {
    if (p2pState > mP2PState)
        mP2PState = p2pState;
}

int P2PProcessor::sendNetDetectRequest() {

    unsigned char mpBuf[MP_MAX_SIZE] = {0};
    MPPacket *mpPacket = (MPPacket *)mpBuf;
    MPNetDetect *mpNetDetect = (MPNetDetect *)mpPacket->data;

    int netDetectSize = sizeof(MPNetDetect);
    int mpPacketSize = sizeof(mpPacket->header) + netDetectSize;

    mpPacket->header.build(MP_NET_DETECT, netDetectSize);

    updateLocalAddr();

    memcpy(&(mpNetDetect->lanAddr), &(mNetInfo.lanAddr), sizeof(struct sockaddr_in));
    mpNetDetect->uuid = mCallId;

    pthread_rwlock_rdlock(&gP2PServersMutex);
    for (string ps : gP2PServers) {
        if(mNetDetectMap.count(make_pair(ps, P2P_SERVER_PORT)) == 0) {
            if (mUser->getXmdTransceiver()->buildAndSendDatagram((char *)ps.c_str(), P2P_SERVER_PORT, (char *)mpPacket, mpPacketSize, 0, P2P_PACKET) < 0) {
                mLogger->warn("send MPNetDetect to %s:%d failed, uuid:%llu, callId:%llu\n",
                              ps.c_str(), P2P_SERVER_PORT, mUser->getUuid(), mCallId);
            } else {
                mLogger->debug("send MPNetDetect to %s:%d ok, uuid:%llu, callId:%llu\n",
                              ps.c_str(), P2P_SERVER_PORT, mUser->getUuid(), mCallId);
            }
        }
    }
    pthread_rwlock_unlock(&gP2PServersMutex);

    updateP2PState(MPS_NET_DETECT);

    return 0;

}

int P2PProcessor::sendNetInfo()
{
    if(mP2PState == MPS_NET_DETECT_TIMEOUT) {
        return -1;
    }

    pthread_rwlock_rdlock(&mUser->getCallsRwlock());
    if (mUser->getCurrentCalls()->count(mCallId) == 0) {
        pthread_rwlock_unlock(&mUser->getCallsRwlock());
        return -1;
    }
    P2PCallSession& callSession = mUser->getCurrentCalls()->at(mCallId);
    pthread_rwlock_unlock(&mUser->getCallsRwlock());

    if (callSession.getCallType() != SINGLE_CALL) {
        mLogger->warn("the current call is not single, uuid:%llu, callId:%llu", mUser->getUuid(), mCallId);
        return -1;
    }

    if (callSession.getIntranetBurrowState() || callSession.getInternetBurrowState()) {
        mLogger->warn("p2p already ok, uuid:%llu, callId:%llu", mUser->getUuid(), mCallId);
        return -1;
    }

    UserInfo toUser = callSession.getPeerUser();
    const std::string& toUserAppAccount = toUser.appaccount();

    std::string lanIp;
    uint16_t lanPort;

    std::string wanIp;
    uint16_t wanPort;

    bool netInfoReady = false;

    if (getAddrInfo(lanIp, lanPort, false, true)) {
        netInfoReady = true;
    }

    if (getAddrInfo(wanIp, wanPort, false, false)) {
        netInfoReady = true;
    }

    if (!netInfoReady)
        return -1;

    string hasPeerNetInfo = "0";
    if (mPeerNetInfo.wanAddr.sin_port != 0)
        hasPeerNetInfo = "1";

    // natType|hasPeerNetinfo
    char resourceBuf[16];
    snprintf(resourceBuf, sizeof(resourceBuf), "%u|%s", mNetInfo.natType, hasPeerNetInfo.c_str());
    string resource = (char *)resourceBuf;
    //string resource = to_string(mNetInfo.natType) + "|" + hasPeerNetInfo;
    mimc::UserInfo userInfo;
    mimc::UserInfo* theUser = &userInfo;
    theUser->set_uuid(mUser->getUuid());
    theUser->set_resource(resource);
    theUser->set_appid(mUser->getAppId());
    theUser->set_appaccount(mUser->getAppAccount());
    theUser->set_intranetip(lanIp);
    theUser->set_intranetport(lanPort);
    theUser->set_internetip(wanIp);
    theUser->set_internetport(wanPort);
    theUser->set_connid(mCallId);

    mLogger->debug("send net info: uuid(%lld), natType|hasPeerNetinfo(%s), appId(%lld), "
                  "appAccount(%s), intranetIp(%s), intranetPort(%d), "
                  "internetIp(%s), internetPort(%d), "
                  "callId(%llu)",
                  theUser->uuid(), theUser->resource().c_str(), theUser->appid(),
                  theUser->appaccount().c_str(), lanIp.c_str(), lanPort,
                  wanIp.c_str(), wanPort,
                  theUser->connid());

    int userInfoBufSize = userInfo.ByteSize();
    char* userInfoBuf= new char[userInfoBufSize];
    memset(userInfoBuf, 0, userInfoBufSize);
    userInfo.SerializeToArray(userInfoBuf, userInfoBufSize);
    std::string userInfoBufStr(userInfoBuf, userInfoBufSize);

    std::string bizType = MP_MSG_NET_INFO;
    mUser->sendMessage(toUserAppAccount, userInfoBufStr, bizType);

    delete[] userInfoBuf;

    updateP2PState(MPS_EXCHANGE_NET_INFO);

    return 0;
}

int P2PProcessor::sendBurrowRequest(int num) {

    pthread_rwlock_rdlock(&mUser->getCallsRwlock());
    if (mUser->getCurrentCalls()->count(mCallId) == 0) {
        pthread_rwlock_unlock(&mUser->getCallsRwlock());
        return -1;
    }
    P2PCallSession& callSession = mUser->getCurrentCalls()->at(mCallId);
    pthread_rwlock_unlock(&mUser->getCallsRwlock());

    if (callSession.getCallType() != SINGLE_CALL) {
        mLogger->warn("the current call is not single, uuid:%llu, callId:%llu", mUser->getUuid(), mCallId);
        return -1;
    }

    if (callSession.getIntranetBurrowState()) {
        mLogger->warn("p2p(intranet) already ok, uuid:%llu, callId:%llu", mUser->getUuid(), mCallId);
        return -1;
    }

    UserInfo peerUserInfo = callSession.getPeerUser();

    unsigned char mpBuf[MP_MAX_SIZE] = {0};
    MPPacket *mpPacket = (MPPacket *)mpBuf;

    BurrowPacket burrowPacket;
    burrowPacket.set_uuid(mUser->getUuid());
    burrowPacket.set_resource(mUser->getResource());
    burrowPacket.set_call_id(mCallId);
    burrowPacket.set_burrow_id(num);
    burrowPacket.set_burrow_type(INTRANET_BURROW_REQUEST);

    int burrowPacketSize = burrowPacket.ByteSize();
    int mpPacketSize = sizeof(mpPacket->header) + burrowPacketSize;
    mpPacket->header.build(MP_P2P_PUNCH, burrowPacketSize);
    burrowPacket.SerializeToArray((char *)mpPacket->data, burrowPacketSize);
//    char *burrowPacketBytes = new char[burrowPacketSize];
//    memset(burrowPacketBytes, 0, burrowPacketSize);
//    burrowPacket.SerializeToArray(burrowPacketBytes, burrowPacketSize);

    string peerLanIp;
    uint16_t peerLanPort;
    if(getAddrInfo(peerLanIp, peerLanPort, true, true) == false) {
        peerLanIp = peerUserInfo.intranetip();
        peerLanPort = peerUserInfo.intranetport();
    }

    int ret1 = -1;
    if ((ret1 = mUser->getXmdTransceiver()->buildAndSendDatagram((char *)peerLanIp.c_str(), peerLanPort,
                                                                 (char *)mpPacket, mpPacketSize, 0, P2P_PACKET)) < 0) {
        mLogger->warn("send INTRANET_BURROW_REQUEST to %s:%d failed, from uuid:%llu, to uuid:%llu",
                      peerLanIp.c_str(), peerLanPort, mUser->getUuid(), peerUserInfo.uuid());
    } else {
        mLogger->debug("send INTRANET_BURROW_REQUEST to %s:%d ok, from uuid:%llu, to uuid:%llu",
                      peerLanIp.c_str(), peerLanPort, mUser->getUuid(), peerUserInfo.uuid());
    }

    if (callSession.getInternetBurrowState()) {
        mLogger->warn("p2p(internet) already ok, uuid:%llu, callId:%llu", mUser->getUuid(), mCallId);
        return 0;
    }

    burrowPacket.set_burrow_type(INTERNET_BURROW_REQUEST);
    memset((char *)mpPacket->data, 0, burrowPacketSize);
    burrowPacket.SerializeToArray((char *)mpPacket->data, burrowPacketSize);
//    memset(burrowPacketBytes, 0, burrowPacketSize);
//    burrowPacket.SerializeToArray(burrowPacketBytes, burrowPacketSize);

    string peerWanIp;
    uint16_t peerWanPort;
    if (getAddrInfo(peerWanIp, peerWanPort, true, false) == false) {
        peerWanIp = peerUserInfo.internetip();
        peerWanPort = peerUserInfo.internetport();
    }

    int delayMS = 0;
    int tryPortCount = 1;
    if((mPeerNetInfo.natType == MP_NAT_SYMMETRIC) && (mNetInfo.natType != MP_NAT_OTHER_CONE)) {
        tryPortCount = MAX_TRY_PORT_COUNT;
        delayMS = 10;
    }
    int ret2 = -1;
    for (int i = 0; i < tryPortCount; i++) {
        if (mUser->getXmdTransceiver()->buildAndSendDatagram((char *) peerWanIp.c_str(), peerWanPort,
                                                             (char *)mpPacket, mpPacketSize, delayMS, P2P_PACKET) < 0) {
            if(i == 0)
                mLogger->warn("send INTERNET_BURROW_REQUEST to %s:%d failed, from uuid:%llu, to uuid:%llu",
                          peerWanIp.c_str(), peerWanPort, mUser->getUuid(), peerUserInfo.uuid());
        } else {
            ret2 = 0;
            if(i == 0)
                mLogger->debug("send INTERNET_BURROW_REQUEST to %s:%d ok, from uuid:%llu, to uuid:%llu",
                          peerWanIp.c_str(), peerWanPort, mUser->getUuid(), peerUserInfo.uuid());
        }
        peerWanPort++;
    }

    if (ret1 < 0 && ret2 < 0)
        return ret1;

    updateP2PState(MPS_P2P_PUNCH);

    return 0;
}

void P2PProcessor::handleP2PPacket(User* user, char* ip, uint16_t port, char *data, uint32_t len) {

    MPPacket *mpPacket = (MPPacket *)data;

    uint8_t mpVersion;
    uint8_t mpType;
    uint16_t mpDataSize;

    mpPacket->header.parse(&mpVersion, &mpType, &mpDataSize);

    XMDLoggerWrapper::instance()->debug("receive MPPacket(%d) from %s:%d", mpType, ip, port);

    switch(mpType) {
        case MP_NET_DETECT_RESP:
        {
            handleNetDetectResp(user, ip, port, (char *)mpPacket->data, mpDataSize);
            break;
        }
        case MP_P2P_PUNCH:
        case MP_P2P_PUNCH_RESP:
        {
            handleBurrowPacket(user, ip, port, (char *)mpPacket->data, mpDataSize);
            break;
        }
        default:
            break;
    }
}

void P2PProcessor::handleNetDetectResp(User* user, char* ip, uint16_t port, char *data, uint32_t size) {
    MPNetDetectResp* netDetectResp = (MPNetDetectResp *)data;
    uint64_t callId = netDetectResp->uuid;

    P2PProcessor*p2pProcessor = P2PProcessor::getP2PProcessor(callId);
    if (p2pProcessor == NULL)
        return;

    p2pProcessor->handleNetDetectResp(netDetectResp, ip, port);
}

void P2PProcessor::handleNetDetectResp(MPNetDetectResp *netDetectResp, char *ip, uint16_t port) {

    char strWanAddr[20] = {0};
    inet_ntop(AF_INET, &(netDetectResp->wanAddr.sin_addr), strWanAddr, INET_ADDRSTRLEN);
    uint16_t wanPort = ntohs(netDetectResp->wanAddr.sin_port);

    mLogger->debug("receive MPNetDetectResp[%s:%d] from %s:%d",
                  strWanAddr, wanPort, ip, port);

    pair<string,uint16_t> p = make_pair(ip,port);
    if (mNetDetectMap.count(p) == 1)
        return;
    mNetDetectMap[p] = netDetectResp->wanAddr;

    if (port != P2P_SERVER_PORT) {
        memcpy(&(mNetInfo.wanAddr), &(netDetectResp->wanAddr), sizeof(struct sockaddr_in));
        mNetInfo.natType = MP_NAT_OTHER_CONE;
        updateP2PState(MPS_NET_DETECT_OK);
    }
    else if (mNetInfo.wanAddr.sin_port == 0) {
        memcpy(&(mNetInfo.wanAddr), &(netDetectResp->wanAddr), sizeof(struct sockaddr_in));
        BindRelayResponse* bindRelayResponse = mUser->getBindRelayResponse();
        mLogger->debug("get wanAddr[%s:%d] from %s:%d, wanAddr[%s:%d] from relay server",
                      strWanAddr, wanPort, ip, port, bindRelayResponse->internet_ip().c_str(), bindRelayResponse->internet_port());
        if (bindRelayResponse->has_internet_port()) {
            uint16_t oldPort = bindRelayResponse->internet_port();
            if(oldPort != 0 && oldPort != wanPort) {
                mNetInfo.natType = MP_NAT_SYMMETRIC;
                updateP2PState(MPS_NET_DETECT_OK);
            }
        }
    }
    else if (mNetInfo.wanAddr.sin_port != netDetectResp->wanAddr.sin_port) {
        if(wanPort > ntohs(mNetInfo.wanAddr.sin_port))
            memcpy(&(mNetInfo.wanAddr), &(netDetectResp->wanAddr), sizeof(struct sockaddr_in));
        mNetInfo.natType = MP_NAT_SYMMETRIC;
        updateP2PState(MPS_NET_DETECT_OK);
    }
    else {
        if (mNetInfo.natType == MP_NAT_UNKNOWN)
            mNetInfo.natType = MP_NAT_PORT_RESTRICTED_CONE;
        updateP2PState(MPS_NET_DETECT_OK);
    }

    string selfWanIp;
    uint16_t selfWanPort;
    getAddrInfo(selfWanIp, selfWanPort, false, false);
    mLogger->debug("handleNetDetectResp end, wanAddr[%s:%d], natType:%d",selfWanIp.c_str(), selfWanPort, mNetInfo.natType);

    return;
}

void P2PProcessor::handleBurrowPacket(User* user, char* ip, uint16_t port, char *data, uint32_t size) {

    BurrowPacket burrowPacket;
    burrowPacket.ParseFromArray(data, size);
    uint64_t callId = burrowPacket.call_id();
    P2PProcessor*p2pProcessor = P2PProcessor::getP2PProcessor(callId);
    if (p2pProcessor == NULL)
        return;

    p2pProcessor->handleBurrowPacket(burrowPacket, ip, port);
}

void P2PProcessor::handleBurrowPacket(BurrowPacket& burrowPacket, char* ip, uint16_t port) {

    if (!burrowPacket.has_burrow_id() || !burrowPacket.has_burrow_type() || !burrowPacket.has_call_id()
        || !burrowPacket.has_uuid() || !burrowPacket.has_resource()) {
        mLogger->warn("receive burrow packet from %s:%d, don't contain enough info", ip, port);
        return;
    }
    uint64_t burrowId = burrowPacket.burrow_id();
    BURROW_TYPE burrowType = burrowPacket.burrow_type();
    uint64_t uuid = burrowPacket.uuid();
    uint64_t callId = burrowPacket.call_id();
    string &resource = const_cast<string &>(burrowPacket.resource());

    mLogger->debug("receive burrow packet(%d) from %s:%d, myUuid:%llu, peerUuid:%llu, peerResource:%s, callId:%llu",
                  burrowType, ip, port, mUser->getUuid(), uuid, resource.c_str(), burrowPacket.call_id());

    pthread_rwlock_rdlock(&mUser->getCallsRwlock());
    if (mUser->getCurrentCalls()->count(callId) == 0) {
        pthread_rwlock_unlock(&mUser->getCallsRwlock());
        return;
    }
    P2PCallSession& callSession = mUser->getCurrentCalls()->at(callId);
    pthread_rwlock_unlock(&mUser->getCallsRwlock());

    CallState callState = callSession.getCallState();
    if (callState != RUNNING || callSession.getCallType() != SINGLE_CALL) {
        mLogger->warn("the call session is wrong, myUuid:%llu, callId:%llu, callState:%d, callType:%d",
                      mUser->getUuid(), mCallId, callState, callSession.getCallType());
        return;
    }

    if (callSession.getPeerUser().uuid() != (int64_t)uuid || callSession.getPeerUser().resource() != resource) {
        mLogger->warn("the uuid or resource mismatch, recvUuid:%llu, recvResource:%s, peerUuid:%llu, peerResource:%s",
                      uuid, resource.c_str(), callSession.getPeerUser().uuid(),
                      callSession.getPeerUser().resource().c_str());
        return;
    }

    updateP2PState(MPS_P2P_PUNCH);

    if ((burrowType == INTERNET_BURROW_REQUEST) || (burrowType == INTERNET_BURROW_RESPONSE)) {
        //收到打洞请求 验证是不是peerIp peerPort
        setAddrInfo(ip, port, true, false);
        //updatePeerIpOrPort(callSession, ip, port);
    } else {
        setAddrInfo(ip, port, true, true);
    }

    if (burrowType == INTRANET_BURROW_REQUEST || burrowType == INTERNET_BURROW_REQUEST) {
        //回复打洞响应
        unsigned char mpBuf[MP_MAX_SIZE] = {0};
        MPPacket *mpPacket = (MPPacket *)mpBuf;

        BurrowPacket burrowResponsePacket;
        burrowResponsePacket.set_uuid(mUser->getUuid());
        burrowResponsePacket.set_resource(mUser->getResource());
        burrowResponsePacket.set_call_id(mCallId);
        burrowResponsePacket.set_burrow_id(burrowId);
        burrowResponsePacket.set_burrow_type(
                burrowType == INTRANET_BURROW_REQUEST ? INTRANET_BURROW_RESPONSE : INTERNET_BURROW_RESPONSE);

        int burrowResponsePacketSize = burrowResponsePacket.ByteSize();
        int mpPacketSize = sizeof(mpPacket->header) + burrowResponsePacketSize;
        mpPacket->header.build(MP_P2P_PUNCH_RESP, burrowResponsePacketSize);
        burrowResponsePacket.SerializeToArray((char *)mpPacket->data, burrowResponsePacketSize);
//        char *burrowPacketBytes = new char[burrowPacketSize];
//        memset(burrowPacketBytes, 0, burrowPacketSize);
//        burrowResponsePacket.SerializeToArray(burrowPacketBytes, burrowPacketSize);

        if (mUser->getXmdTransceiver()->buildAndSendDatagram(ip, port, (char *)mpPacket, mpPacketSize, 0, P2P_PACKET) < 0) {
            mLogger->warn("send burrow response(%d) to %s:%d failed, myUuid:%llu, callId:%llu",
                          burrowResponsePacket.burrow_type(), ip, port, mUser->getUuid(), mCallId);
        } else {
            mLogger->debug("send burrow response(%d) to %s:%d ok, myUuid:%llu, callId:%llu",
                          burrowResponsePacket.burrow_type(), ip, port, mUser->getUuid(), mCallId);
        }

        return;
    }

    if (burrowType == INTRANET_BURROW_RESPONSE) {
        if (callSession.getIntranetBurrowState()) {
            return;
        }
        //发起方创建connection
        if (callSession.isCreator()) {
            uint64_t connId = createP2PConn(ip, port, INTRANET_CONN_REQUEST, INTRANET_CONN);
            if (connId > 0) {
                //确定两端都成功后再设置
                //callSession.setP2PIntranetConnId(connId);
                updateP2PState(MPS_P2P_INTRANET_OK);
                callSession.setIntranetBurrowState(true);
            }
        }
        else {
            updateP2PState(MPS_P2P_INTRANET_OK);
            callSession.setIntranetBurrowState(true);
        }
        return;
    }

    if (burrowType == INTERNET_BURROW_RESPONSE) {

        if (callSession.getInternetBurrowState()) {
            return;
        }

        //发起方创建connection
        if (callSession.isCreator()) {
            uint64_t connId = createP2PConn(ip, port, INTERNET_CONN_REQUEST, INTERNET_CONN);
            if (connId > 0) {
                //确定两端都成功后再设置
                //callSession.setP2PInternetConnId(connId);
                updateP2PState(MPS_P2P_INTERNET_OK);
                callSession.setInternetBurrowState(true);
            }
        }
        else {
            updateP2PState(MPS_P2P_INTERNET_OK);
            callSession.setInternetBurrowState(true);
        }
        return;
    }
}

void P2PProcessor::handleNetInfo(User* user, UserInfo* userInfo) {
    uint64_t callId = userInfo->connid();
    P2PProcessor*p2pProcessor = P2PProcessor::getP2PProcessor(callId);
    if (p2pProcessor == NULL)
        return;

    p2pProcessor->handleNetInfo(userInfo);
}

void P2PProcessor::handleNetInfo(UserInfo* userInfo) {

    mLogger->debug("receive net info: uuid(%llu), natType|hasPeerNetInfo(%s), appId(%llu), "
                  "appAccount(%s), intranetIp(%s), intranetPort(%d), "
                  "internetIp(%s), internetPort(%d), "
                  "callId(%llu)",
                  userInfo->uuid(), userInfo->resource().c_str(), userInfo->appid(),
                  userInfo->appaccount().c_str(), userInfo->intranetip().c_str(), userInfo->intranetport(),
                  userInfo->internetip().c_str(), userInfo->internetport(), userInfo->connid());

    string natTypeStr = "0";
    string hasPeerNetInfo = "0";
    // natType|hasPeerNetInfo
    string resource = userInfo->resource();
    string::size_type delimPos = resource.find("|");
    if((delimPos > 0) && (delimPos < resource.size() - 1)) {
        natTypeStr = resource.substr(0, delimPos);
        hasPeerNetInfo = resource.substr(delimPos + 1);
    }
    else
        natTypeStr = resource;
    // int natType = stoi(natTypeStr);
    int natType = strtol(natTypeStr.c_str(), NULL, 10);
    mPeerNetInfo.natType = natType;
    setAddrInfo(const_cast<string &>(userInfo->internetip()), userInfo->internetport(), true, false);
    setAddrInfo(const_cast<string &>(userInfo->intranetip()), userInfo->intranetport(), true, true);

    if (hasPeerNetInfo == "1")
        updateP2PState(MPS_EXCHANGE_NET_INFO_OK);
}

uint64_t P2PProcessor::createP2PConn(string ip, uint16_t port, PKT_TYPE pkt_type, RtsConnType connType) {

    mimc::UserPacket userPacket;
    userPacket.set_uuid(mUser->getUuid());
    userPacket.set_resource(mUser->getResource());
    userPacket.set_pkt_type(pkt_type);
    userPacket.set_call_id(mCallId);
    userPacket.set_region_bucket(mUser->getRegionBucket());

    int message_size = userPacket.ByteSize();
    char *messageBytes = new char[message_size];
    memset(messageBytes, 0, message_size);
    userPacket.SerializeToArray(messageBytes, message_size);

    char portStr[8] = { '0' };
    snprintf(portStr, sizeof(portStr), "%u", port);
    std::string peerAddress = ip + ":" + (char *)(portStr);
    // std::string peerAddress = ip + ":" + to_string(port);

    uint64_t p2pConnId = mUser->getXmdTransceiver()->createConnection((char *)ip.c_str(), port, messageBytes,
                                                                      message_size, XMD_TRAN_TIMEOUT,
                                                                      new RtsConnectionInfo(peerAddress, connType, mCallId));
    if (p2pConnId == 0) {
        mLogger->warn("create p2p connection failed, uuid:%llu, callId:%llu, peer addr:%s",
                      mUser->getUuid(), mCallId, peerAddress.c_str());
    }else {
        mLogger->debug("create p2p connection ok, uuid:%llu, callId:%llu, peer addr:%s, connId:%llu",
                      mUser->getUuid(), mCallId, peerAddress.c_str(), p2pConnId);
    }

    return p2pConnId;
}

void P2PProcessor::closeP2PConn(User* user, uint64_t callId) {

    //XMDLoggerWrapper::instance()->debug("P2PProcessor::closeP2PConn start, callId:%llu",callId);
    //pthread_rwlock_rdlock(&user->getCallsRwlock());
    if (user->getCurrentCalls()->count(callId) == 0) {
        //pthread_rwlock_unlock(&user->getCallsRwlock());
        return;
    }
    P2PCallSession &callSession = user->getCurrentCalls()->at(callId);
    //pthread_rwlock_unlock(&user->getCallsRwlock());

    if (callSession.getP2PIntranetConnId() != 0) {
        user->getXmdTransceiver()->closeConnection(callSession.getP2PIntranetConnId());
        callSession.resetP2PIntranetConn();
    }
    if (callSession.getP2PInternetConnId() != 0) {
        user->getXmdTransceiver()->closeConnection(callSession.getP2PInternetConnId());
        callSession.resetP2PInternetConn();
    }

    stopP2P(user,callId);

    //XMDLoggerWrapper::instance()->debug("P2PProcessor::closeP2PConn end, callId:%llu",callId);

}