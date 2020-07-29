#include "mimc/user.h"
#include "mimc/connection.h"
#include "mimc/packet_manager.h"
#include "mimc/serverfetcher.h"
#include "mimc/p2p_callsession.h"
#include "mimc/utils.h"
#include "mimc/threadsafe_queue.h"
#include "mimc/rts_connection_handler.h"
#include "mimc/rts_stream_handler.h"
#include "mimc/rts_datagram_handler.h"
#include "mimc/rts_send_data.h"
#include "mimc/rts_send_signal.h"
#include "mimc/ims_push_service.pb.h"
#include "mimc/rts_signal.pb.h"
#include "mimc/rts_context.h"
#include "XMDTransceiver.h"
#include "json-c/json.h"
#include <fstream>
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include "mimc/mutex_lock.h"
#include "mimc/error.h"


#ifdef _WIN32
#else
#include <unistd.h>
#endif // _WIN32

#include "mimc/p2p_processor.h"


User::User(int64_t appId, std::string appAccount, std::string resource, std::string cachePath,bool is_save_cache)
        :audioStreamConfig(ACK_TYPE, ACK_STREAM_WAIT_TIME_MS, false),
         videoStreamConfig(FEC_TYPE, ACK_STREAM_WAIT_TIME_MS, false) ,
         appId(appId),
         appAccount(appAccount),
         chid(MIMC_CHID),
         testPacketLoss(0),
         permitLogin(false),
         onlineStatus(Offline),
         tokenInvalid(false),
         addressInvalid(false),
         tokenRequestStatus(-1),
         tokenFetchSucceed(false),
         serverFetchSucceed(false),
         bindRelayResponse(NULL),
         isSaveCache(is_save_cache),
         lastLoginTimestamp(0),lastPingTimestamp(0),tokenFetcher(NULL),statusHandler(NULL),messageHandler(NULL),
         rtsCallEventHandler(NULL),rtsConnectionHandler(NULL),rtsStreamHandler(NULL),rtsDatagramHandler(NULL),
         packetManager(new PacketManager()),conn(new Connection()),
         xmdSendBufferSize(DEFAULT_SEND_BUFFER_SIZE),xmdRecvBufferSize(DEFAULT_RECV_BUFFER_SIZE),
         currentCalls(new std::map<uint64_t, P2PCallSession>()),onlaunchCalls(new std::map<uint64_t, pthread_t>()),
         xmdTranseiver(NULL),maxCallNum(1),
         mutex_0(PTHREAD_RWLOCK_INITIALIZER),mutex_onlaunch(PTHREAD_RWLOCK_INITIALIZER),mutex_1(PTHREAD_MUTEX_INITIALIZER),
         p2pEnabled(false),sendThreadStopFlag(false),receiveThreadStopFlag(false),checkThreadStopFlag(false)
         {
    
    this->resetRelayLinkState();
    if (resource == "") {
        resource = "cpp_default";
    }
    this->cacheExist = false;
    if (cachePath == "") {
        char buffer[MAXPATHLEN];
        Utils::getCwd(buffer, MAXPATHLEN);
        cachePath = buffer;
    }
    if (cachePath.back() == '/') {
        cachePath.pop_back();
    }
    this->cachePath = cachePath + '/' + Utils::int2str(appId) + '/' + appAccount + '/' + resource;
    XMDLoggerWrapper::instance()->info("cachePath is %s", this->cachePath.c_str());
    this->cacheFile = this->cachePath + '/' + "mimc.info";
    if(is_save_cache){
        createCacheFileIfNotExist(this);
    }
    
    if (resource == "cpp_default") {
        this->resource = Utils::generateRandomString(8);
    } else {
        this->resource = resource;
    }
    this->conn->setUser(this);
    mimcBackoff::getInstance()->init();

    pthread_create(&sendThread, NULL, sendPacket, (void *)(this->conn));
    pthread_create(&receiveThread, NULL, receivePacket, (void *)(this->conn));
    pthread_create(&checkThread, NULL, checkTimeout, (void *)(this->conn));
}

User::~User() {
    XMDLoggerWrapper::instance()->info("~User,appcount is %s",appAccount.c_str());
    sendThreadStopFlag = true;
    receiveThreadStopFlag = true;
    checkThreadStopFlag = true;
    
    pthread_join(sendThread, NULL);
    this->conn->resetSock();
    pthread_join(receiveThread, NULL);
    pthread_join(checkThread, NULL);

    if (this->xmdTranseiver) {
        this->xmdTranseiver->stop();
        this->xmdTranseiver->join();
    }

    clearCurrentCallsMap();

    delete this->packetManager;
    delete this->currentCalls;
    delete this->onlaunchCalls;
    delete this->xmdTranseiver;
    
    delete this->conn;
    delete this->rtsConnectionHandler;
    delete this->rtsStreamHandler;
    delete this->rtsDatagramHandler;
}

#ifdef __ANDROID__
void User::handleQuit(int signo)
{
    //pthread_exit(NULL);
}
#endif

void* User::sendPacket(void *arg) {

#ifndef _WIN32 
#ifdef _IOS_MIMC_USE_ 
    pthread_setname_np("mimc-send");
#else    
    prctl(PR_SET_NAME, "mimc-send");
#endif //end _IOS_MIMC_USE_    
#endif // !_WIN32

    XMDLoggerWrapper::instance()->info("mimc sendThread start to run");
    Connection *conn = (Connection *)arg;
    User * user = conn->getUser();
    unsigned char * packetBuffer = NULL;
    int packet_size = 0;
    MessageDirection msgType;

    while (!user->sendThreadStopFlag || user->getPacketManager()->packetsWaitToSend.Size() > 0)
    {
        msgType = C2S_DOUBLE_DIRECTION;
        if (conn->getState() == NOT_CONNECTED) {
            if (user->getTestPacketLoss() >= 100) {
                //usleep(10000);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            if (!fetchToken(user)) {
                mimcBackoff::getInstance()->getbackoffWhenFetchToken().sleep();
                continue;
            } 

            mimcBackoff::getInstance()->getbackoffWhenFetchToken().reset();
            fetchServerAddr(user);
            // user->lastCreateConnTimestamp = time(NULL);
            int64_t t1 = Utils::currentTimeMillis();
            XMDLoggerWrapper::instance()->debug("Prepare to connect");
            if (!conn->connect()) {
                XMDLoggerWrapper::instance()->debug("In sendPacket, socket connect failed, user is %s", user->getAppAccount().c_str());
                //user->getBackoffWhenConnectFe().sleep();
                mimcBackoff::getInstance()->getbackoffWhenConnectFe().sleep();

                continue;
            } 

            //user->getBackoffWhenConnectFe().reset();
            mimcBackoff::getInstance()->getbackoffWhenConnectFe().reset();
            int64_t t2 = Utils::currentTimeMillis();
            XMDLoggerWrapper::instance()->info("Socket connected");
            XMDLoggerWrapper::instance()->info("Socket connect timecost=%llu", t2 - t1);
            conn->setState(SOCK_CONNECTED);

            XMDLoggerWrapper::instance()->info("fe conn timecost begin time=%llu", t2);
            packet_size = user->getPacketManager()->encodeConnectionPacket(packetBuffer, conn);
        } else if (conn->getState() == SOCK_CONNECTED) {
            //usleep(5000);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else if (conn->getState() == HANDSHAKE_CONNECTED) {
            time_t now = time(NULL);
            if (user->getOnlineStatus() == Offline && (now - user->lastLoginTimestamp > LOGIN_TIMEOUT || user->tokenInvalid)) {
                if (user->permitLogin) {
                    if (!fetchToken(user)) {
                        mimcBackoff::getInstance()->getbackoffWhenFetchToken().sleep();
                        continue;
                    } 

                    mimcBackoff::getInstance()->getbackoffWhenFetchToken().reset();
                    int64_t ts = Utils::currentTimeMillis();
                    XMDLoggerWrapper::instance()->info("fe bind timecost begin time=%llu", ts);
                    packet_size = user->getPacketManager()->encodeBindPacket(packetBuffer, conn);
                    user->lastLoginTimestamp = time(NULL);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            } else {
                struct waitToSendContent obj;
                if (user->getPacketManager()->packetsWaitToSend.Pop(obj)) {
                    XMDLoggerWrapper::instance()->info("mimc send queue size=%d, timeout queue size=%d",
                    user->getPacketManager()->packetsWaitToSend.Size(), user->getPacketManager()->packetsWaitToTimeout.size());
                    msgType = obj.type;
                    if (obj.cmd == BODY_CLIENTHEADER_CMD_SECMSG) {
                        packet_size = user->getPacketManager()->encodeSecMsgPacket(packetBuffer, conn, obj.message);
                    }
                    else if (obj.cmd == BODY_CLIENTHEADER_CMD_UNBIND) {
                        packet_size = user->getPacketManager()->encodeUnBindPacket(packetBuffer, conn);
                    }
                } else if (now - user->lastPingTimestamp > PING_TIMEINTERVAL) {
                    packet_size = user->getPacketManager()->encodePingPacket(packetBuffer, conn);
                }
            }
        }

        if (packet_size < 0 || packetBuffer == NULL) {
            //usleep(10000);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (msgType == C2S_DOUBLE_DIRECTION) {
            conn->trySetNextResetTs();
        }

        user->lastPingTimestamp = time(NULL);

        if (user->getTestPacketLoss() >= 100) {
            delete[] packetBuffer;
            packetBuffer = NULL;
            continue;
        }

        int ret = conn->writen((void *)packetBuffer, packet_size);
        if (ret != packet_size) {
            XMDLoggerWrapper::instance()->error("mimc sendPacket failed, ret != packet_size, ret=%d, packet_size=%d", ret, packet_size);
            conn->resetSock();
        }
        else {
            XMDLoggerWrapper::instance()->debug("mimc sendPacket success,ret=%d", ret);
        }

        delete[] packetBuffer;
        packetBuffer = NULL;
    }

    return NULL;
}

void* User::receivePacket(void *arg) {

#ifndef _WIN32 
#ifdef _IOS_MIMC_USE_ 
    pthread_setname_np("mimc-recv");
#else    
    prctl(PR_SET_NAME, "mimc-recv");
#endif //end _IOS_MIMC_USE_    
#endif // !_WIN32

    XMDLoggerWrapper::instance()->info("mimc receiveThread start to run");
    Connection *conn = (Connection *)arg;
    User * user = conn->getUser();
    unsigned char * packetHeaderBuffer = new unsigned char[HEADER_LENGTH];
    pthread_cleanup_push(receiveCleanup, packetHeaderBuffer);

    while (!user->receiveThreadStopFlag)
    {
        if (conn->getState() == NOT_CONNECTED) {
            //usleep(20000);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        memset(packetHeaderBuffer, 0, HEADER_LENGTH);
        int ret = conn->readn((void *)packetHeaderBuffer, HEADER_LENGTH);
        XMDLoggerWrapper::instance()->debug("mimc receiveThread receive packet header");
        if (ret != HEADER_LENGTH) {
            XMDLoggerWrapper::instance()->error("mimc receiveThread receive packet header error, HEADER_LENGTH is %d, ret is %d", HEADER_LENGTH, ret);
            conn->resetSock();
            continue;
        }
        int body_len = user->getPacketManager()->char2int(packetHeaderBuffer, HEADER_BODYLEN_OFFSET) + BODY_CRC_LEN;
        unsigned char * packetBodyBuffer = new unsigned char[body_len];
        memset(packetBodyBuffer, 0, body_len);
        ret = conn->readn((void *)packetBodyBuffer, body_len);
        XMDLoggerWrapper::instance()->debug("mimc receiveThread receive packet body");
        if (ret != body_len) {
            XMDLoggerWrapper::instance()->error("mimc receiveThread receive packet body error, body_len is %d, ret is %d", body_len, ret);
            conn->resetSock();
            delete[] packetBodyBuffer;
            packetBodyBuffer = NULL;
            continue;
        }
        
        if (user->getTestPacketLoss() >= 100) {
            delete[] packetBodyBuffer;
            packetBodyBuffer = NULL;
            continue;
        }
        conn->clearNextResetSockTs();
        int packet_size = HEADER_LENGTH + body_len;
        unsigned char *packetBuffer = new unsigned char[packet_size];
        memset(packetBuffer, 0, packet_size);
        memmove(packetBuffer, packetHeaderBuffer, HEADER_LENGTH);
        memmove(packetBuffer + HEADER_LENGTH, packetBodyBuffer, body_len);

        delete[] packetBodyBuffer;
        packetBodyBuffer = NULL;
        XMDLoggerWrapper::instance()->debug("mimc receiveThread receive packet succeed, start decode packet");
        ret = user->getPacketManager()->decodePacketAndHandle(packetBuffer, conn);
        delete[] packetBuffer;
        packetBuffer = NULL;
        if (ret < 0) {
            XMDLoggerWrapper::instance()->error("decodePacketAndHandle failed,ret = %d",ret);
            conn->resetSock();
        }
    }

    delete[] packetHeaderBuffer;
    packetHeaderBuffer = NULL;

    pthread_cleanup_pop(0);

    return NULL;
}

void User::receiveCleanup(void *arg) {
    unsigned char* packetHeaderBuffer = (unsigned char*)arg;
    delete[] packetHeaderBuffer;
}

void* User::checkTimeout(void *arg) {
#ifndef _WIN32 
#ifdef _IOS_MIMC_USE_ 
    pthread_setname_np("mimc-timeout");
#else    
    prctl(PR_SET_NAME, "mimc-timeout");
#endif //end _IOS_MIMC_USE_    
#endif // !_WIN32

    XMDLoggerWrapper::instance()->info("mimc checkThread start to run");
    Connection *conn = (Connection *)arg;
    User * user = conn->getUser();

    while (!user->checkThreadStopFlag)
    {
        if ((conn->getNextResetSockTs() > 0) && (time(NULL) - conn->getNextResetSockTs() > 0)) {
            XMDLoggerWrapper::instance()->info("In checkTimeout, packet recv timeout");
            conn->resetSock();
        }
        user->getPacketManager()->checkMessageSendTimeout(user);
        user->getPacketManager()->checkReceiveMessageSequence();
        user->rtsScanAndCallBack();
        user->relayConnScanAndCallBack();
        RtsSendData::sendPingRelayRequest(user);
        //sleep(1);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return NULL;
}

void User::relayConnScanAndCallBack() {
    if (this->relayLinkState != BEING_CREATED) {
        return;
    }
    if (time(NULL) - this->latestLegalRelayLinkStateTs < RELAY_CONN_TIMEOUT) {
        return;
    }

    if (this->relayConnId != 0) {
        this->xmdTranseiver->closeConnection(relayConnId);
    }

    pthread_rwlock_rdlock(&mutex_0);
    std::vector<uint64_t> &tmpCallVec = getCallVec();
    for (std::vector<uint64_t>::const_iterator iter = tmpCallVec.begin(); iter != tmpCallVec.end(); iter++) {
        uint64_t callId = *iter;
        XMDLoggerWrapper::instance()->error("In relayConnScanAndCallBack >= RELAY_CONN_TIMEOUT, callId=%llu", callId);
        P2PCallSession callSession;
        if (! getCallSessionNoSafe(callId, callSession)) {
            continue;
        }
        if (callSession.getCallState() == WAIT_SEND_UPDATE_REQUEST) {
            RtsSendSignal::sendByeRequest(this, callId, UPDATE_TIMEOUT,callSession);
        }
    }
    pthread_rwlock_unlock(&mutex_0);

    clearCurrentCallsMap();
    this->resetRelayLinkState();
}

void User::rtsScanAndCallBack() {
    if (currentCallsMapSize() == 0) {
        return;
    }

    /* RWMutexLock rwlock(&mutex_0);
    rwlock.wlock(); */
    pthread_rwlock_wrlock(&mutex_0);
    int flags = 0;
    std::vector<uint64_t> &tmpCallVec = getCallVec();
    for (std::vector<uint64_t>::iterator iter = tmpCallVec.begin(); iter != tmpCallVec.end();) {
        const uint64_t callId = *iter;
        P2PCallSession callSession;
        if (! getCallSessionNoSafe(callId, callSession)) {
            iter++;
            continue;
        }
        const CallState& callState = callSession.getCallState();
        if (callState == RUNNING) {
            iter++;
            continue;
        }

        if (time(NULL) - callSession.getLatestLegalCallStateTs() < RTS_CALL_TIMEOUT) {
            iter++;
            continue;
        }

        if (callState == WAIT_CREATE_RESPONSE || callState == WAIT_SEND_CREATE_REQUEST) {
            XMDLoggerWrapper::instance()->error("In rtsScanAndCallBack, callId %llu state %d is timeout, user is %s", callId, callState, appAccount.c_str());
            if(this->currentCalls->count(callId) != 0) {
                 currentCalls->erase(callId);
            }
            iter = currentCallVec.erase(iter);

            rtsCallEventHandler->onAnswered(callId, false, DIAL_CALL_TIMEOUT);
        } else if (callState == WAIT_INVITEE_RESPONSE || callState == WAIT_CALL_ONLAUNCHED) {
            XMDLoggerWrapper::instance()->error("In rtsScanAndCallBack, callId %llu state %d is timeout, user is %s", callId, callState, appAccount.c_str());
            pthread_t onlaunchCallThread;
            if (getLaunchCallThread(callId, onlaunchCallThread)) {
#ifndef __ANDROID__
                pthread_cancel(onlaunchCallThread);
#else
                if(pthread_kill(onlaunchCallThread, 0) != ESRCH)
				{
    				pthread_kill(onlaunchCallThread, SIGTERM);
				}
#endif
                deleteLaunchCall(callId);
            }

            if(this->currentCalls->count(callId) > 0) {
                currentCalls->erase(callId);
            }
            iter = currentCallVec.erase(iter);

            rtsCallEventHandler->onClosed(callId, INVITE_RESPONSE_TIMEOUT);
        } else if (callState == WAIT_UPDATE_RESPONSE || callState == WAIT_SEND_UPDATE_REQUEST) {
            XMDLoggerWrapper::instance()->error("In rtsScanAndCallBack, callId %llu state %d is timeout, user is %s", callId, callState, appAccount.c_str());
            RtsSendSignal::sendByeRequest(this, callId, UPDATE_TIMEOUT,callSession);

            if(this->currentCalls->count(callId) > 0) {
                    currentCalls->erase(callId);
            }
            iter = currentCallVec.erase(iter);

            rtsCallEventHandler->onClosed(callId, UPDATE_TIMEOUT);
        } else {
            XMDLoggerWrapper::instance()->error("In rtsScanAndCallBack, callId %llu state: %d, user is %s", callId,callState, appAccount.c_str());
            iter++;
        }

        //added by huanghua on 2019-04-13 for p2p
        XMDLoggerWrapper::instance()->info("In rtsScanAndCallBack, callId:%llu, user:%s, state:%d timeout", callId, appAccount.c_str(), callState);

        P2PProcessor::closeP2PConn(this, callId);
        flags = 1;
    }
    pthread_rwlock_unlock(&mutex_0);    

    if(flags){
        RtsSendData::closeRelayConnWhenNoCall(this);
    }    

}

void* User::onLaunched(void *arg) {
#ifndef _WIN32 
#ifdef _IOS_MIMC_USE_ 
    pthread_setname_np("mimc-launched");
#else    
    prctl(PR_SET_NAME, "mimc-launched");
#endif //end _IOS_MIMC_USE_    
#endif // !_WIN32

#ifndef __ANDROID__
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
#else
    signal(SIGTERM, handleQuit);
#endif
    onLaunchedParam* param = (onLaunchedParam*)arg;
    User* user = param->user;
    uint64_t callId = param->callId;
    mimc::UserInfo fromUser;
    std::string appContent;


    P2PCallSession callSession;
    if (! user->getCallSession(callId, callSession)) {
        return 0;
    }
    fromUser = callSession.getPeerUser();
    appContent = callSession.getAppContent();

    LaunchedResponse userResponse = user->getRTSCallEventHandler()->onLaunched(callId, fromUser.appaccount(), appContent, fromUser.resource());

    {
        /* RWMutexLock rwlock((pthread_rwlock_t*)&user->getCallsRwlock());
        rwlock.wlock(); */
        pthread_rwlock_wrlock(&user->getCallsRwlock());
        P2PCallSession p2pCallSession;
        if (! user->getCallSessionNoSafe(callId, p2pCallSession)) {
            pthread_rwlock_unlock(&user->getCallsRwlock());
            return 0;
        }

        if (!userResponse.isAccepted()) {
            RtsSendSignal::sendInviteResponse(user, callId, p2pCallSession.getCallType(), mimc::PEER_REFUSE, userResponse.getDesc());
            user->getCurrentCalls()->erase(callId);
            pthread_rwlock_unlock(&user->getCallsRwlock());
            
            RtsSendData::closeRelayConnWhenNoCall(user);
            user->deleteLaunchCall(callId);
            return 0;
        }
        RtsSendSignal::sendInviteResponse(user, callId, p2pCallSession.getCallType(), mimc::SUCC, userResponse.getDesc());
        p2pCallSession.setCallState(RUNNING);
        p2pCallSession.setLatestLegalCallStateTs(time(NULL));
        user->updateCurrentCallsMapNoSafe(callId, p2pCallSession);
        pthread_rwlock_unlock(&user->getCallsRwlock());
    }

    //会话接通，开始P2P相关流程
    // added by huanghua for p2p on 2019-04-01
    if(user->isP2PEnabled())
        P2PProcessor::startP2P(user, callId);

    delete param;
    param = NULL;

    user->deleteLaunchCall(callId);
    return 0;
}



void User::onLaunchCleanup(void *arg) {
    //pthread_rwlock_unlock((pthread_rwlock_t *)arg);
}

void User::createCacheFileIfNotExist(User * user) {
    if (!Utils::createDirIfNotExist(user->getCachePath())) {
        return;
    }
    std::fstream file;
    file.open(user->getCacheFile().c_str());
    if (file) {
        user->cacheExist = true;
        if (file.is_open()) {
            file.close();
        }
        return;
    } 

    XMDLoggerWrapper::instance()->info("cacheFile is not exist, create now");
    file.open(user->getCacheFile().c_str(), std::ios::out);
    if (file) {
        XMDLoggerWrapper::instance()->info("cacheFile create succeed");
        user->cacheExist = true;
        if (file.is_open()) {
            file.close();
        }
    }
}

bool User::fetchToken(User * user) {
    if (user->tokenFetchSucceed && !user->tokenInvalid) {
        return true;
    }
    const char* str = NULL;
    int str_size = 0;
    if (user->cacheExist) {
        std::ifstream file(user->getCacheFile(), std::ios::in | std::ios::ate);
        long size = file.tellg();
        file.seekg(0, std::ios::beg);
        user->tokenRequestStatus = -1;
        char* localStr = new char[size + 1];
        memset(localStr, 0, size + 1);
        file.read(localStr, size);
        file.close();
        json_object* pobj_local = NULL;
        if (!user->tokenInvalid && size>0 && user->parseToken(localStr, pobj_local)) {
            json_object_put(pobj_local);
            delete[] localStr;
            return true;
        } 
        
        if (user->getTokenFetcher() == NULL) {
            json_object_put(pobj_local);
            delete[] localStr;
            return false;
        }
        user->tokenRequestStatus = 0;
        std::string remoteStr = user->getTokenFetcher()->fetchToken();
        json_object* pobj_remote;
        if (!user->parseToken(remoteStr.c_str(), pobj_remote)) {
            json_object_put(pobj_local);
            json_object_put(pobj_remote);
            delete[] localStr;
            return false;
        }
        
        user->tokenRequestStatus = 1;
        user->tokenInvalid = false;
        std::ofstream fileOut(user->getCacheFile(), std::ios::out | std::ios::ate);
        if (fileOut.is_open()) {
            if (pobj_local == NULL) {
                str = remoteStr.c_str();
                str_size = remoteStr.size();
            } else {
                json_object* dataobj = json_object_new_object();
                char s[21] = {0};
                json_object_object_add(dataobj, "appId", json_object_new_string(Utils::ltoa(user->getAppId(), s)));
                json_object_object_add(dataobj, "appPackage", json_object_new_string(user->getAppPackage().c_str()));
                json_object_object_add(dataobj, "appAccount", json_object_new_string(user->getAppAccount().c_str()));
                json_object_object_add(dataobj, "miChid", json_object_new_int(user->getChid()));
                json_object_object_add(dataobj, "miUserId", json_object_new_string(Utils::ltoa(user->getUuid(), s)));
                json_object_object_add(dataobj, "miUserSecurityKey", json_object_new_string(user->getSecurityKey().c_str()));
                json_object_object_add(dataobj, "token", json_object_new_string(user->getToken().c_str()));
                json_object_object_add(dataobj, "regionBucket", json_object_new_int(user->getRegionBucket()));
                json_object_object_add(dataobj, "feDomainName", json_object_new_string(user->getFeDomain().c_str()));
                json_object_object_add(dataobj, "relayDomainName", json_object_new_string(user->getRelayDomain().c_str()));
                json_object_object_add(dataobj, "ppsMiotDomainName", json_object_new_string(user->getP2PDomain().c_str()));
                json_object_object_add(pobj_local, "code", json_object_new_int(200));
                json_object_object_add(pobj_local, "data", dataobj);

                str = json_object_get_string(pobj_local);
                str_size = strlen(str);
            }
            fileOut.write(str, str_size);
            fileOut.close();
        }
        json_object_put(pobj_local);
        json_object_put(pobj_remote);
        delete[] localStr;
        return true;
    } 
    
    if (user->getTokenFetcher() == NULL) {
        return false;
    }
    user->tokenRequestStatus = 0;
    std::string remoteStr = user->getTokenFetcher()->fetchToken();
    json_object* pobj_remote;
    if (!user->parseToken(remoteStr.c_str(), pobj_remote)) {
        json_object_put(pobj_remote);
        return false;
    }
    
    user->tokenRequestStatus = 1;
    user->tokenInvalid = false;
    if(!user->isSaveCache) {
        json_object_put(pobj_remote);
        return true;
    }
    createCacheFileIfNotExist(user);
    if (user->cacheExist) {
        std::ofstream file(user->getCacheFile(), std::ios::out | std::ios::ate);
        if (file.is_open()) {
            str = remoteStr.c_str();
            str_size = remoteStr.size();
            file.write(str, str_size);
            file.close();
        }
    }
    json_object_put(pobj_remote);
    return true;
}

bool User::fetchServerAddr(User* user) {
    MIMCMutexLock mutexLock(&user->mutex_1);
    if (user->serverFetchSucceed && !user->addressInvalid) {
        return true;
    }
    if (user->feDomain == "" || user->relayDomain == "") {
        XMDLoggerWrapper::instance()->error("User::fetchServerAddr, feDomain or relayDomain is empty");
        return false;
    }

    if (user->p2pDomain == "") {
        XMDLoggerWrapper::instance()->error("User::fetchServerAddr: found empty p2pDomain");
    }

    const char* str = NULL;
    int str_size = 0;
    if (user->cacheExist) {
        std::ifstream file(user->getCacheFile(), std::ios::in | std::ios::ate);
        long size = file.tellg();
        file.seekg(0, std::ios::beg);
        char* localStr = new char[size + 1];
        memset(localStr, 0, size + 1);
        file.read(localStr, size);
        file.close();
        json_object* pobj_local;

        if (user->parseServerAddr(localStr, pobj_local) && !user->addressInvalid) {
            json_object_put(pobj_local);
            delete[] localStr;
            return true;
        } 
        
        std::string domainList = user->feDomain + "," + user->relayDomain;
        if (user->p2pDomain != "") {
            domainList += ",";
            domainList += user->p2pDomain;
        }
        std::string remoteStr = ServerFetcher::fetchServerAddr(RESOLVER_URL, domainList);

        json_object* pobj_remote;
        if (!user->parseServerAddr(remoteStr.c_str(), pobj_remote)) {
            json_object_put(pobj_local);
            json_object_put(pobj_remote);
            delete[] localStr;
            return false;
        }
        
        user->addressInvalid = false;
        std::ofstream fileOut(user->getCacheFile(), std::ios::out | std::ios::ate);
        if (fileOut.is_open()) {
            if (pobj_local == NULL) {
                str = remoteStr.c_str();
                str_size = remoteStr.size();
            } else {
                json_object* feAddrArray = json_object_new_array();
                for (std::vector<std::string>::const_iterator iter = user->feAddresses.begin(); iter != user->feAddresses.end(); iter++) {
                    std::string feAddr = *iter;
                    json_object_array_add(feAddrArray, json_object_new_string(feAddr.c_str()));
                }
                json_object* relayAddrArray = json_object_new_array();
                for (std::vector<std::string>::const_iterator iter = user->relayAddresses.begin(); iter != user->relayAddresses.end(); iter++) {
                    std::string relayAddr = *iter;
                    json_object_array_add(relayAddrArray, json_object_new_string(relayAddr.c_str()));
                }
                json_object* p2pAddrArray = json_object_new_array();
                if (user->p2pDomain != "" && !user->p2pAddresses.empty()) {
                    for (std::vector<std::string>::const_iterator iter = user->p2pAddresses.begin(); iter != user->p2pAddresses.end(); ++iter) {
                        json_object_array_add(p2pAddrArray, json_object_new_string((*iter).c_str()));
                    }
                    XMDLoggerWrapper::instance()->info("User::fetchServerAddr: p2pDomain(%s) has %d mapped addresses",
                                                        user->p2pDomain.c_str(), user->p2pAddresses.size());
                }

                json_object_object_add(pobj_local, user->feDomain.c_str(), feAddrArray);
                json_object_object_add(pobj_local, user->relayDomain.c_str(), relayAddrArray);
                json_object_object_add(pobj_local, user->p2pDomain.c_str(), p2pAddrArray);

                str = json_object_get_string(pobj_local);
                str_size = strlen(str);
            }
            fileOut.write(str, str_size);
            fileOut.close();
        }
        json_object_put(pobj_local);
        json_object_put(pobj_remote);

        delete[] localStr;
        return true;
    } 
    
    std::string domainList = user->feDomain + "," + user->relayDomain;
    if (user->p2pDomain != "") {
        domainList += ",";
        domainList += user->p2pDomain;
    }

    std::string remoteStr = ServerFetcher::fetchServerAddr(RESOLVER_URL, domainList);

    json_object* pobj_remote;
    if (!user->parseServerAddr(remoteStr.c_str(), pobj_remote)) {
        json_object_put(pobj_remote);
        return false;
    }
    
    user->addressInvalid = false;
    if(!user->isSaveCache) {
        json_object_put(pobj_remote);
        return true;
    }
    createCacheFileIfNotExist(user);
    if (user->cacheExist) {
        std::ofstream file(user->getCacheFile(), std::ios::out | std::ios::ate);
        if (file.is_open()) {
            str = remoteStr.c_str();
            str_size = remoteStr.size();
            file.write(str, str_size);
            file.close();
        }
    }
    json_object_put(pobj_remote);
    return true;
}

bool User::parseToken(const char* str, json_object*& pobj) {
    tokenFetchSucceed = false;
    pobj = json_tokener_parse(str);

    if (pobj == NULL) {
        XMDLoggerWrapper::instance()->info("User::parseToken, json_tokener_parse failed, pobj is NULL, user is %s", appAccount.c_str());
        return tokenFetchSucceed;
    }

    json_object* retobj = NULL;
    json_object_object_get_ex(pobj, "code", &retobj);
    int retCode = json_object_get_int(retobj);

    if (retCode != 200) {
        XMDLoggerWrapper::instance()->info("User::parseToken, parse failed, retCode is %d, user is %s", retCode, appAccount.c_str());
        return tokenFetchSucceed;
    }

    json_object* dataobj = NULL;
    json_object_object_get_ex(pobj, "data", &dataobj);
    json_object* dataitem_obj = NULL;
    const char* pstr = NULL;
    json_object_object_get_ex(dataobj, "appId", &dataitem_obj);
    pstr = json_object_get_string(dataitem_obj);
    if (pstr) {
        int64_t appId = atoll(pstr);
        if (appId != this->appId) {
            return tokenFetchSucceed;
        }
    }
    

    json_object_object_get_ex(dataobj, "appAccount", &dataitem_obj);
    pstr = json_object_get_string(dataitem_obj);
    if (pstr) {
        std::string appAccount = pstr;
        if (appAccount != this->appAccount) {
            return tokenFetchSucceed;
        }
    }
    

    json_object_object_get_ex(dataobj, "appPackage", &dataitem_obj);
    pstr = json_object_get_string(dataitem_obj);
    if (pstr) {
        this->appPackage = pstr;
    }

    dataitem_obj = NULL;
    json_object_object_get_ex(dataobj, "miChid", &dataitem_obj);
    if (dataitem_obj != NULL) {
        this->chid = json_object_get_int(dataitem_obj);
    }

    json_object_object_get_ex(dataobj, "miUserId", &dataitem_obj);
    pstr = json_object_get_string(dataitem_obj);
    if (!pstr) {
        return tokenFetchSucceed;
    } else {
        this->uuid = atoll(pstr);
    }

    json_object_object_get_ex(dataobj, "miUserSecurityKey", &dataitem_obj);
    pstr = json_object_get_string(dataitem_obj);
    if (!pstr) {
        return tokenFetchSucceed;
    } else {
        this->securityKey = pstr;
    }

    json_object_object_get_ex(dataobj, "token", &dataitem_obj);
    pstr = json_object_get_string(dataitem_obj);
    if (!pstr) {
        return tokenFetchSucceed;
    } else {
        this->token = pstr;
    }

    json_object_object_get_ex(dataobj, "regionBucket", &dataitem_obj);
    this->regionBucket = json_object_get_int(dataitem_obj);
 
    json_object_object_get_ex(dataobj, "feDomainName", &dataitem_obj);
    pstr = json_object_get_string(dataitem_obj);
    if (!pstr) {
        return tokenFetchSucceed;
    } else {
        this->feDomain = pstr;
    }

    json_object_object_get_ex(dataobj, "relayDomainName", &dataitem_obj);
    pstr = json_object_get_string(dataitem_obj);
    if (!pstr) {
        return tokenFetchSucceed;
    } else {
        this->relayDomain = pstr;
    }

    json_object_object_get_ex(dataobj, "ppsMiotDomainName", &dataitem_obj);
    pstr = json_object_get_string(dataitem_obj);
    if (pstr != NULL) {
        this->p2pDomain = pstr;
    } else {
        this->p2pDomain = PPS_DOMAIN;
        XMDLoggerWrapper::instance()->warn("User::parseToken: found empty p2pDomain, use default value(%s)",
                                           this->p2pDomain.c_str());
    }

    tokenFetchSucceed = true;

    XMDLoggerWrapper::instance()->info("User::parseToken, parse succeed, user is %s", appAccount.c_str());

    return tokenFetchSucceed;
}

bool User::parseServerAddr(const char* str, json_object*& pobj) {
    serverFetchSucceed = false;

    pobj = json_tokener_parse(str);

    if (pobj == NULL) {
        XMDLoggerWrapper::instance()->error("User::parseServerAddr, json_tokener_parse failed, pobj is NULL");
        return false;
    }

    json_object* dataobj = NULL;
    json_object_object_get_ex(pobj, this->feDomain.c_str(), &dataobj);
    if (dataobj && json_object_get_type(dataobj) == json_type_array) {
        int arraySize = json_object_array_length(dataobj);
        if (arraySize == 0) {
            return serverFetchSucceed;
        }
        this->feAddresses.clear();
        for (int i = 0; i < arraySize; i++) {
            json_object* dataitem_obj = json_object_array_get_idx(dataobj, i);
            const char* feAddress = json_object_get_string(dataitem_obj);
            this->feAddresses.push_back(feAddress);
        }
    } else {
        return serverFetchSucceed;
    }

    json_object_object_get_ex(pobj, this->relayDomain.c_str(), &dataobj);
    if (dataobj && json_object_get_type(dataobj) == json_type_array) {
        int arraySize = json_object_array_length(dataobj);
        if (arraySize == 0) {
            return serverFetchSucceed;
        }
        this->relayAddresses.clear();
        for (int i = 0; i < arraySize; i++) {
            json_object* dataitem_obj = json_object_array_get_idx(dataobj, i);
            const char* relayAddress = json_object_get_string(dataitem_obj);
            this->relayAddresses.push_back(relayAddress);
        }
    } else {
        return serverFetchSucceed;
    }

    json_object_object_get_ex(pobj, this->p2pDomain.c_str(), &dataobj);
    if (dataobj && (json_object_get_type(dataobj) == json_type_array)) {
        this->p2pAddresses.clear();
        int arraySize = json_object_array_length(dataobj);
        for (int i = 0; i < arraySize; i++) {
            json_object* dataitem_obj = json_object_array_get_idx(dataobj, i);
            std::string addr = json_object_get_string(dataitem_obj);
            XMDLoggerWrapper::instance()->debug("User::parseServerAddr: p2pAddrItem(%s)", addr.c_str());
            std::string p2pIp = addr.substr(0, addr.find(':'));
            this->p2pAddresses.push_back(p2pIp);
        }
        P2PProcessor::updateP2PServerList(this->p2pAddresses);
    }

    serverFetchSucceed = true;
    XMDLoggerWrapper::instance()->debug("User::parseServerAddr, serverFetchSucceed is true, user is %s", appAccount.c_str());
    
    return serverFetchSucceed;
}

std::string User::join(const std::map<std::string, std::string>& kvs) const {
    if (kvs.empty()) {
        return "";
    }

    std::ostringstream oss;
    for (std::map<std::string, std::string>::const_iterator iter = kvs.begin(); iter != kvs.end(); iter++) {
        const std::string& key = iter->first;
        const std::string& value = iter->second;
        oss << key << ":" << value;
        if (iter != kvs.end()) {
            oss << ",";
        }
    }

    return oss.str();
}
std::string User::sendOnlineMessage(const std::string& toAppAccount, const std::string& payload, const std::string& bizType, const bool isStore)
{
    if (onlineStatus == Offline || toAppAccount == "" ||  payload.size() > MIMC_MAX_PAYLOAD_SIZE) {
        return "";
    }
    mimc::MIMCUser *from, *to;
    from = new mimc::MIMCUser();
    from->set_appid(this->appId);
    from->set_appaccount(this->appAccount);
    from->set_uuid(this->uuid);
    from->set_resource(this->resource);

    to = new mimc::MIMCUser();
    to->set_appid(this->appId);
    to->set_appaccount(toAppAccount);

    mimc::MIMCP2PMessage * message = new mimc::MIMCP2PMessage();
    message->set_allocated_from(from);
    message->set_allocated_to(to);
    message->set_payload(std::string(payload));
    message->set_biztype(bizType);
    message->set_isstore(isStore);

    int message_size = message->ByteSize();
    char * messageBytes = new char[message_size];
    memset(messageBytes, 0, message_size);
    message->SerializeToArray(messageBytes, message_size);
    std::string messageBytesStr(messageBytes, message_size);

    delete message;
    message = NULL;


    std::string packetId = packetManager->createPacketId();
    mimc::MIMCPacket * packet = new mimc::MIMCPacket();
    packet->set_packetid(packetId);
    packet->set_package(appPackage);
    packet->set_type(mimc::ONLINE_MESSAGE);
    packet->set_payload(messageBytesStr);
    packet->set_timestamp(time(NULL));

    delete[] messageBytes;
    messageBytes = NULL;

    MIMCMessage mimcMessage(packetId, packet->sequence(), appAccount, resource, toAppAccount, "", payload, bizType, packet->timestamp());
    //(this->packetManager->packetsWaitToTimeout).insert(std::pair<std::string, MIMCMessage>(packetId, mimcMessage));

    struct waitToSendContent mimc_obj;
    mimc_obj.cmd = BODY_CLIENTHEADER_CMD_SECMSG;
    mimc_obj.type = C2S_SINGLE_DIRECTION;
    mimc_obj.message = packet;
    (this->packetManager->packetsWaitToSend).Push(mimc_obj);

    XMDLoggerWrapper::instance()->debug("User::sendOnlineMessage, to:%s, packetId:%s",toAppAccount.c_str(),packetId.c_str());

    return packetId;

}

std::string User::sendMessage(const std::string &toAppAccount, const std::string &payload, const std::string &bizType, bool isStore, bool isConversation)
{
    if (this->onlineStatus == Offline || toAppAccount == "" || payload == "" || payload.size() > MIMC_MAX_PAYLOAD_SIZE) {
        return "";
    }

    mimc::MIMCUser *from, *to;
    from = new mimc::MIMCUser();
    from->set_appid(this->appId);
    from->set_appaccount(this->appAccount);
    from->set_uuid(this->uuid);
    from->set_resource(this->resource);

    to = new mimc::MIMCUser();
    to->set_appid(this->appId);
    to->set_appaccount(toAppAccount);

    mimc::MIMCP2PMessage * message = new mimc::MIMCP2PMessage();
    message->set_allocated_from(from);
    message->set_allocated_to(to);
    message->set_payload(std::string(payload));
    message->set_biztype(bizType);
    message->set_isstore(isStore);

    int message_size = message->ByteSize();
    char * messageBytes = new char[message_size];
    memset(messageBytes, 0, message_size);
    message->SerializeToArray(messageBytes, message_size);
    std::string messageBytesStr(messageBytes, message_size);

    delete message;
    message = NULL;

    std::string packetId = this->packetManager->createPacketId();
    mimc::MIMCPacket * packet = new mimc::MIMCPacket();
    packet->set_packetid(packetId);
    packet->set_package(this->appPackage);
    packet->set_type(mimc::P2P_MESSAGE);
    packet->set_payload(messageBytesStr);
    packet->set_timestamp(time(NULL));
    packet->set_conversation(isConversation);

    delete[] messageBytes;
    messageBytes = NULL;

    MIMCMessage mimcMessage(packetId, packet->sequence(), this->appAccount, this->resource, toAppAccount, "", payload, bizType, packet->timestamp());
    (this->packetManager->packetsWaitToTimeout).insert(std::pair<std::string, MIMCMessage>(packetId, mimcMessage));

    struct waitToSendContent mimc_obj;
    mimc_obj.cmd = BODY_CLIENTHEADER_CMD_SECMSG;
    mimc_obj.type = C2S_DOUBLE_DIRECTION;
    mimc_obj.message = packet;
    (this->packetManager->packetsWaitToSend).Push(mimc_obj);

    XMDLoggerWrapper::instance()->info("User::sendMessage, to:%s, packetId:%s",toAppAccount.c_str(),packetId.c_str());

    return packetId;
}

void User::pull()
{
    if (this->onlineStatus == Offline)
    {
        return;
    }

    mimc::PullMessageRequest *pullMessageRequest = new mimc::PullMessageRequest();
    pullMessageRequest->set_uuid(this->uuid);
    pullMessageRequest->set_resource(this->resource);
    pullMessageRequest->set_appid(this->appId);

    int request_size = pullMessageRequest->ByteSize();
    char *requestBytes = new char[request_size];
    memset(requestBytes, 0, request_size);
    pullMessageRequest->SerializeToArray(requestBytes, request_size);
    std::string requestBytesStr(requestBytes, request_size);

    delete pullMessageRequest;
    pullMessageRequest = NULL;

    std::string packetId = this->packetManager->createPacketId();
    mimc::MIMCPacket *packet = new mimc::MIMCPacket();
    packet->set_packetid(packetId);
    packet->set_package(this->appPackage);
    packet->set_type(mimc::PULL);
    packet->set_payload(requestBytesStr);
    packet->set_timestamp(time(NULL));

    delete[] requestBytes;
    requestBytes = NULL;

    struct waitToSendContent mimc_obj;
    mimc_obj.cmd = BODY_CLIENTHEADER_CMD_SECMSG;
    mimc_obj.type = C2S_SINGLE_DIRECTION;
    mimc_obj.message = packet;
    (this->packetManager->packetsWaitToSend).Push(mimc_obj);
}

std::string User::sendGroupMessage(const int64_t topicId, const std::string &payload, const std::string &bizType, bool isStore, bool isConversation)
{
    if (this->messageHandler == NULL) {
        XMDLoggerWrapper::instance()->error("In sendGroupMessage, messageHandler is not registered!");
        return "";
    }

    if (this->onlineStatus == Offline || payload == "" || payload.size() > MIMC_MAX_PAYLOAD_SIZE) {
        return "";
    }

    mimc::MIMCUser *from;
    from = new mimc::MIMCUser();
    from->set_appid(this->appId);
    from->set_appaccount(this->appAccount);
    from->set_uuid(this->uuid);
    from->set_resource(this->resource);

    mimc::MIMCGroup* to = new mimc::MIMCGroup();
    to->set_appid(this->appId);
    to->set_topicid(topicId);

    mimc::MIMCP2TMessage* message = new mimc::MIMCP2TMessage();
    message->set_allocated_from(from);
    message->set_allocated_to(to);
    message->set_payload(std::string(payload));
    message->set_isstore(isStore);
    message->set_biztype(bizType);

    int message_size = message->ByteSize();
    char * messageBytes = new char[message_size];
    memset(messageBytes, 0, message_size);
    message->SerializeToArray(messageBytes, message_size);
    std::string messageBytesStr(messageBytes, message_size);

    delete message;
    message = NULL;

    std::string packetId = this->packetManager->createPacketId();
    mimc::MIMCPacket * packet = new mimc::MIMCPacket();
    packet->set_packetid(packetId);
    packet->set_package(this->appPackage);
    packet->set_type(mimc::P2T_MESSAGE);
    packet->set_payload(messageBytesStr);
    packet->set_timestamp(time(NULL));
    packet->set_conversation(isConversation);

    delete[] messageBytes;
    messageBytes = NULL;

    MIMCGroupMessage mimcGroupMessage(packetId, packet->sequence(), this->appAccount, this->resource,topicId, payload, bizType, packet->timestamp());
    (this->packetManager->groupPacketWaitToTimeout).insert(std::pair<std::string, MIMCGroupMessage>(packetId, mimcGroupMessage));

    struct waitToSendContent mimc_obj;
    mimc_obj.cmd = BODY_CLIENTHEADER_CMD_SECMSG;
    mimc_obj.type = C2S_DOUBLE_DIRECTION;
    mimc_obj.message = packet;
    (this->packetManager->packetsWaitToSend).Push(mimc_obj);

    return packetId;
}


uint64_t User::dialCall(const std::string & toAppAccount, const std::string & appContent, const std::string & toResource) {
    if (this->rtsCallEventHandler == NULL) {
        XMDLoggerWrapper::instance()->error("In dialCall, rtsCallEventHandler is not registered!");
        return 0;
    }
    if (toAppAccount == "") {
        XMDLoggerWrapper::instance()->error("In dialCall, toAppAccount can not be empty!");
        return 0;
    }
    if (this->getOnlineStatus() == Offline) {
        XMDLoggerWrapper::instance()->warn("In dialCall, user:%lld is offline!", uuid);
        return 0;
    }


    if (currentCallsMapSize() == maxCallNum) {
        XMDLoggerWrapper::instance()->warn("In dialCall, current Calls size has reached maxCallNum %d!", maxCallNum);
        return 0;
    }

    this->checkToRunXmdTranseiver();

    mimc::UserInfo toUser;
    toUser.set_appid(this->appId);
    toUser.set_appaccount(toAppAccount);
    if (toResource != "") {
        toUser.set_resource(toResource);
    }

   /*  RWMutexLock rwlock(&mutex_0);
    rwlock.wlock(); */
    pthread_rwlock_rdlock(&mutex_0);
    //判断是否是同一个call
    std::vector<uint64_t> &tmpCallVec = getCallVec();
    for (std::vector<uint64_t>::const_iterator iter = tmpCallVec.begin(); iter != tmpCallVec.end(); iter++) {
        P2PCallSession callSession;
        if (! getCallSessionNoSafe(*iter, callSession)) {
            continue;
        }
        const mimc::UserInfo& session_peerUser = callSession.getPeerUser();
        if (toUser.appid() == session_peerUser.appid() && toUser.appaccount() == session_peerUser.appaccount() && toUser.resource() == session_peerUser.resource()
            && appContent == callSession.getAppContent()) {
            XMDLoggerWrapper::instance()->warn("In dialCall, the call has connected!");
            pthread_rwlock_unlock(&mutex_0);
            return 0;
        }
    }

    uint64_t callId;
    do {
        //callId = Utils::generateRandomLong();
        callId = generateCallId();
    } while (countCurrentCallsMapNoSafe(callId) > 0 || callId == 0);
    pthread_rwlock_unlock(&mutex_0);

    XMDLoggerWrapper::instance()->info("In dialCall, callId is %llu", callId);

    if (this->relayLinkState == NOT_CREATED) {

        uint64_t relayConnId = RtsSendData::createRelayConn(this);
        XMDLoggerWrapper::instance()->info("In dialCall, relayConnId is %llu", relayConnId);
        if (relayConnId == 0) {
            XMDLoggerWrapper::instance()->error("In dialCall, launch create relay conn failed!");
            return 0;

        }
        insertIntoCurrentCallsMap(callId, P2PCallSession(callId, toUser, mimc::SINGLE_CALL, WAIT_SEND_CREATE_REQUEST, time(NULL), true, appContent));
        return callId;
    } else if (this->relayLinkState == BEING_CREATED) {
        insertIntoCurrentCallsMap(callId, P2PCallSession(callId, toUser, mimc::SINGLE_CALL, WAIT_SEND_CREATE_REQUEST, time(NULL), true, appContent));
        return callId;
    } else if (this->relayLinkState == SUCC_CREATED) {
        RWMutexLock rwlock(&mutex_0);
        rwlock.wlock();
        P2PCallSession callSession(callId, toUser, mimc::SINGLE_CALL, WAIT_CREATE_RESPONSE, time(NULL), true, appContent);
        insertIntoCurrentCallsMapNoSafe(callId, callSession);
        RtsSendSignal::sendCreateRequest(this, callId,callSession);
        updateCurrentCallsMapNoSafe(callId, callSession);
        return callId;
    } else {
        return 0;

    }
}

int User::sendRtsData(uint64_t callId, const std::string & data, const RtsDataType dataType, const RtsChannelType channelType, const std::string& ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
    int dataId = -1;
    if (data.size() > RTS_MAX_PAYLOAD_SIZE) {
        XMDLoggerWrapper::instance()->error("data size is %d too large!",data.size());
        return MimcError::INPUT_PARAMS_ERROR;
    }
    if (dataType != AUDIO && dataType != VIDEO && dataType != FILEDATA) {
        XMDLoggerWrapper::instance()->error("data type error,input type is %d!",dataType);
        return MimcError::INPUT_PARAMS_ERROR;
    }

    mimc::PKT_TYPE pktType = mimc::USER_DATA_AUDIO;
    if (dataType == VIDEO) {
        pktType = mimc::USER_DATA_VIDEO;
    } else if (dataType == FILEDATA) {
        pktType = mimc::USER_DATA_FILE;
    }
    //int64_t t0 = Utils::currentTimeMillis();

    P2PCallSession callSession;
    if (!getCallSession(callId, callSession)) {
        XMDLoggerWrapper::instance()->error("callSession is not exist,callid = %lld",callId);
        return MimcError::CALLSESSION_NOT_FOUND;
    }

    if (callSession.getCallState() != RUNNING) {
        XMDLoggerWrapper::instance()->error("callSession callState is %d not running",callSession.getCallState());
        return MimcError::CALLSESSION_NOT_RUNNING;
    }


    if (onlineStatus == Offline) {
        XMDLoggerWrapper::instance()->error("status is offline");
        return MimcError::DIAL_USER_NOT_LINE;
    }

    RtsContext* rtsContext = new RtsContext(callId, ctx);
    //long t1 = Utils::currentTimeMillis();
    //XMDLoggerWrapper::instance()->info("appAccount %s callId %lld In sendRtsData, t1-t0 is %ld",appAccount.c_str(),callId, t1-t0);
    if (channelType == RELAY) {
        dataId = RtsSendData::sendRtsDataByRelay(this, callId, data, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        if(dataId < 0) {
			XMDLoggerWrapper::instance()->error("send relay data error,dataId =  %d", dataId);
			delete rtsContext;
		}
    }
        //modified by huanghua on 2019-04-11 for p2p
    else if (channelType == P2P_INTRANET) {
        dataId = RtsSendData::sendRtsDataByP2PIntranet(this, callId, data, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        if(dataId < 0) {
			XMDLoggerWrapper::instance()->error("sendRtsDataByP2PIntranet error,dataId =  %d", dataId);
			delete rtsContext;
		}
    }
    else if (channelType == P2P_INTERNET) {
        dataId = RtsSendData::sendRtsDataByP2PInternet(this, callId, data, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        if(dataId < 0) {
			XMDLoggerWrapper::instance()->error("sendRtsDataByP2PInternet error,dataId =  %d", dataId);
			delete rtsContext;
		}
    }
    else {
        //added by huanghua on 2019-05-27
        netStatus relayStatus = xmdTranseiver->getNetStatus(getRelayConnId());
        netStatus p2pIntranetStatus = xmdTranseiver->getNetStatus(callSession.getP2PIntranetConnId());
        netStatus p2pInternetStatus = xmdTranseiver->getNetStatus(callSession.getP2PInternetConnId());
        XMDLoggerWrapper::instance()->debug("NetStatus,relay[rtt:%dms,lossRate:%f,packets:%d],p2pIntranet[rtt:%dms,lossRate:%f,packets:%d],p2pInternet[rtt:%dms,lossRate:%f,packets:%d]",
                                           relayStatus.rtt,relayStatus.packetLossRate,relayStatus.totalPackets,
                                           p2pIntranetStatus.rtt,p2pIntranetStatus.packetLossRate,p2pIntranetStatus.totalPackets,
                                           p2pInternetStatus.rtt,p2pInternetStatus.packetLossRate,p2pInternetStatus.totalPackets);
        if (callSession.isP2PIntranetOK() && (p2pIntranetStatus.totalPackets >= 2 * NETSTATUS_MIN_PACKETS) && ((p2pIntranetStatus.packetLossRate <= NETSTATUS_MAX_LOSSRATE) || (p2pIntranetStatus.packetLossRate <= relayStatus.packetLossRate * 2))) {
            dataId = RtsSendData::sendRtsDataByP2PIntranet(this, callId, data, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        } else if (callSession.isP2PInternetOK() && (p2pInternetStatus.totalPackets >= 2 * NETSTATUS_MIN_PACKETS) && ((p2pInternetStatus.packetLossRate <= NETSTATUS_MAX_LOSSRATE) || (p2pInternetStatus.packetLossRate <= relayStatus.packetLossRate * 2))) {
            dataId = RtsSendData::sendRtsDataByP2PInternet(this, callId, data, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        } else {
            dataId = RtsSendData::sendRtsDataByRelay(this, callId, data, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        }


        if(dataId < 0) {
			XMDLoggerWrapper::instance()->error("send data  error,dataId =  %d", dataId);
			delete rtsContext;
		}
    }

    return dataId;
}

//added by huanghua on 2019-08-20
int User::sendRtsData(uint64_t callId, const char* data, const unsigned int dataSize, const RtsDataType dataType, const RtsChannelType channelType, const std::string& ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
    int dataId = -1;
    if (!data) {
        XMDLoggerWrapper::instance()->error("data is null!");
        return dataId;
    }
    if (dataSize > RTS_MAX_PAYLOAD_SIZE) {
        XMDLoggerWrapper::instance()->error("data size is %d too large!",dataSize);
        return dataId;
    }
    if (dataType != AUDIO && dataType != VIDEO && dataType != FILEDATA) {
        XMDLoggerWrapper::instance()->error("data type error,input type is %d!",dataType);
        return dataId;
    }

    mimc::PKT_TYPE pktType = mimc::USER_DATA_AUDIO;
    if (dataType == VIDEO) {
        pktType = mimc::USER_DATA_VIDEO;
    } else if (dataType == FILEDATA) {
        pktType = mimc::USER_DATA_FILE;
    }
    //long t0 = Utils::currentTimeMillis();

    P2PCallSession callSession;
    if (!getCallSession(callId, callSession)) {
        XMDLoggerWrapper::instance()->error("callSession is not exist,callid = %lld",callId);
        return dataId;
    }

    if (callSession.getCallState() != RUNNING) {
        XMDLoggerWrapper::instance()->error("callSession callState is %d not running",callSession.getCallState());
        return dataId;
    }

    if (onlineStatus == Offline) {
        XMDLoggerWrapper::instance()->error("status is offline");
        return dataId;
    }

    RtsContext* rtsContext = new RtsContext(callId, ctx);
    //long t1 = Utils::currentTimeMillis();
    //XMDLoggerWrapper::instance()->info("appAccount %s callId %lld In sendRtsData, t1-t0 is %ld",appAccount.c_str(),callId, t1-t0);
    if (channelType == RELAY) {
        dataId = RtsSendData::sendRtsDataByRelay(this, callId, data, dataSize, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        if(dataId < 0) {
            XMDLoggerWrapper::instance()->error("send relay data error,dataId =  %d", dataId);
            delete rtsContext;
        }
    }
        //modified by huanghua on 2019-04-11 for p2p
    else if (channelType == P2P_INTRANET) {
        dataId = RtsSendData::sendRtsDataByP2PIntranet(this, callId, data, dataSize, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        if(dataId < 0) {
            XMDLoggerWrapper::instance()->error("sendRtsDataByP2PIntranet error,dataId =  %d", dataId);
            delete rtsContext;
        }
    }
    else if (channelType == P2P_INTERNET) {
        dataId = RtsSendData::sendRtsDataByP2PInternet(this, callId, data, dataSize, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        if(dataId < 0) {
            XMDLoggerWrapper::instance()->error("sendRtsDataByP2PInternet error,dataId =  %d", dataId);
            delete rtsContext;
        }
    }
    else {
        //added by huanghua on 2019-05-27
        netStatus relayStatus = xmdTranseiver->getNetStatus(getRelayConnId());
        netStatus p2pIntranetStatus = xmdTranseiver->getNetStatus(callSession.getP2PIntranetConnId());
        netStatus p2pInternetStatus = xmdTranseiver->getNetStatus(callSession.getP2PInternetConnId());
//        XMDLoggerWrapper::instance()->info("NetStatus,relay[rtt:%dms,lossRate:%f,packets:%d],"
//                                           "p2pIntranet[rtt:%dms,lossRate:%f,packets:%d,connId:%llu],"
//                                           "p2pInternet[rtt:%dms,lossRate:%f,packets:%d,connId:%llu]",
//                                            relayStatus.rtt,relayStatus.packetLossRate,relayStatus.totalPackets,
//                                            p2pIntranetStatus.rtt,p2pIntranetStatus.packetLossRate,p2pIntranetStatus.totalPackets,callSession.getP2PIntranetConnId(),
//                                            p2pInternetStatus.rtt,p2pInternetStatus.packetLossRate,p2pInternetStatus.totalPackets,callSession.getP2PInternetConnId());
        if (callSession.isP2PIntranetOK() && (p2pIntranetStatus.totalPackets >= 2 * NETSTATUS_MIN_PACKETS) && ((p2pIntranetStatus.packetLossRate <= NETSTATUS_MAX_LOSSRATE) || (p2pIntranetStatus.packetLossRate <= relayStatus.packetLossRate * 2))) {
            dataId = RtsSendData::sendRtsDataByP2PIntranet(this, callId, data, dataSize, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        } else if (callSession.isP2PInternetOK() && (p2pInternetStatus.totalPackets >= 2 * NETSTATUS_MIN_PACKETS) && ((p2pInternetStatus.packetLossRate <= NETSTATUS_MAX_LOSSRATE) || (p2pInternetStatus.packetLossRate <= relayStatus.packetLossRate * 2))) {
            dataId = RtsSendData::sendRtsDataByP2PInternet(this, callId, data, dataSize, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        } else {
            dataId = RtsSendData::sendRtsDataByRelay(this, callId, data, dataSize, pktType, (void *)rtsContext, canBeDropped, priority, resendCount);
        }


        if(dataId < 0) {
            XMDLoggerWrapper::instance()->error("send data  error,dataId =  %d", dataId);
            delete rtsContext;
        }
    }

//    XMDLoggerWrapper::instance()->debug("sendRtsData, return:%d, callId:%lld, channelType:%d, intranetState:%d, intranetId:%llu, internetState:%d, internetId:%llu",
//                                       dataId, callId, channelType, callSession.getIntranetBurrowState(), callSession.getP2PIntranetConnId(), callSession.getInternetBurrowState(), callSession.getP2PInternetConnId());

    //long t2 = Utils::currentTimeMillis();
    //XMDLoggerWrapper::instance()->info(" In sendRtsData, callId %lld  t2-t1 is %ld ",callId,t2-t1);
    //XMDLoggerWrapper::instance()->info("dataId = %d",dataId);

    return dataId;

}

void User::closeCall(uint64_t callId, std::string byeReason) {
    XMDLoggerWrapper::instance()->info("In closeCall, callId is %llu, byeReason is %s", callId, byeReason.c_str());
    if (countCurrentCallsMap(callId) == 0) {
        XMDLoggerWrapper::instance()->error("callId %llu count is 0",callId);
        return;
    }

    P2PCallSession callSession;
    if (!getCallSession(callId, callSession)) {
	    XMDLoggerWrapper::instance()->error("this callid not exist %llu", callId);
	    return;
	}
    if(!RtsSendSignal::sendByeRequest(this, callId, byeReason,callSession)){
        XMDLoggerWrapper::instance()->error("In closeCall,sendByeRequest failed");
    }
    pthread_t onlaunchCallThread;
    if (getLaunchCallThread(callId, onlaunchCallThread)) {
#ifndef __ANDROID__
        pthread_cancel(onlaunchCallThread);
#else
        if(pthread_kill(onlaunchCallThread, 0) != ESRCH)
    	{
        	pthread_kill(onlaunchCallThread, SIGTERM);
    	}
#endif
        deleteLaunchCall(callId);
    }

    {
    RWMutexLock rwlock(&mutex_0);
    rwlock.wlock();
    //added by huanghua on 2019-04-13 for p2p
    P2PProcessor::closeP2PConn(this, callId);
    deleteFromCurrentCallsMapNoSafe(callId);
    //RtsSendData::closeRelayConnWhenNoCall(this);    
    }

    RtsSendData::closeRelayConnWhenNoCall(this);
    rtsCallEventHandler->onClosed(callId, "CLOSED_INITIATIVELY");
}

bool User::login() {
    if (onlineStatus == Online) {
        return true;
    }
    if (tokenFetcher == NULL || statusHandler == NULL) {
        XMDLoggerWrapper::instance()->warn("login failed, user must register tokenFetcher and statusHandler first!");
        return false;
    }
    if (messageHandler == NULL && rtsCallEventHandler == NULL) {
        XMDLoggerWrapper::instance()->warn("login failed, user must register messageHandler or rtsCallEventHandler first!");
        return false;
    }
    permitLogin = true;
    return true;
}

bool User::logout() {
    XMDLoggerWrapper::instance()->info("logout,appcount: %s resource: %s",appAccount.c_str(),resource.c_str());
    if (onlineStatus == Offline) {
        return true;
    }

    /* RWMutexLock rwlock(&mutex_0);
    rwlock.rlock(); */
    pthread_rwlock_wrlock(&mutex_0);
    std::vector<uint64_t> &tmpCallVec = getCallVec();
    for (std::vector<uint64_t>::const_iterator iter = tmpCallVec.begin(); iter != tmpCallVec.end(); iter++) {
        uint64_t callId = *iter;
        P2PCallSession callSession;
        if (! getCallSessionNoSafe(callId, callSession)) {
            continue;
        }
        pthread_t onlaunchCallThread;
        if (getLaunchCallThread(callId, onlaunchCallThread)) {
#ifndef __ANDROID__
            pthread_cancel(onlaunchCallThread);
#else
            if(pthread_kill(onlaunchCallThread, 0) != ESRCH)
    		{
        		pthread_kill(onlaunchCallThread, SIGTERM);
    		}
#endif
            deleteLaunchCall(callId);
        }
        if (callSession.getCallState() >= RUNNING) {
            RtsSendSignal::sendByeRequest(this, callId, "CLIENT LOGOUT",callSession);
            rtsCallEventHandler->onClosed(callId, "SELF LOGOUT");
        }
        //added by huanghua on 2019-04-13 for p2p
        P2PProcessor::closeP2PConn(this, callId);
    }
    

    struct waitToSendContent logout_obj;
    logout_obj.cmd = BODY_CLIENTHEADER_CMD_UNBIND;
    logout_obj.type = C2S_DOUBLE_DIRECTION;
    logout_obj.message = NULL;
    packetManager->packetsWaitToSend.Push(logout_obj);

    clearCurrentCallsMapNoSafe();
    pthread_rwlock_unlock(&mutex_0);
    RtsSendData::closeRelayConnWhenNoCall(this);

    permitLogin = false;

    return true;
}

void User::resetRelayLinkState() {
    XMDLoggerWrapper::instance()->info("In resetRelayLinkState, relayConnId to be reset is %llu", this->relayConnId);
    this->relayConnId = 0;
    this->relayControlStreamId = 0;
    this->relayAudioStreamId = 0;
    this->relayVideoStreamId = 0;
    this->relayFileStreamId = 0;
    this->relayLinkState = NOT_CREATED;
    this->latestLegalRelayLinkStateTs = time(NULL);
    delete this->bindRelayResponse;
    this->bindRelayResponse = NULL;
}

void User::handleXMDConnClosed(uint64_t connId, ConnCloseType type) {
    if (connId == this->relayConnId) {
        XMDLoggerWrapper::instance()->warn("XMDConnection(RELAY) is closed abnormally, connId is %llu, ConnCloseType is %d", connId, type);
        resetRelayLinkState();
    } else {
        RWMutexLock rwlock(&mutex_0);
        rwlock.rlock();
        for (std::map<uint64_t, P2PCallSession>::iterator iter = currentCalls->begin(); iter != currentCalls->end(); iter++) {
            uint64_t callId = iter->first;
            P2PCallSession& callSession = iter->second;
            if (connId == callSession.getP2PIntranetConnId()) {
                XMDLoggerWrapper::instance()->warn("XMDConnection(P2PIntranet) is closed abnormally, connId is %llu, ConnCloseType is %d, callId is %llu", connId, type, callId);
                callSession.resetP2PIntranetConn();
            } else if (connId == callSession.getP2PInternetConnId()) {
                XMDLoggerWrapper::instance()->warn("XMDConnection(P2PInternet) is closed abnormally, connId is %llu, ConnCloseType is %d, callId is %llu", connId, type, callId);
                callSession.resetP2PInternetConn();
            }
        }
    }

    checkAndCloseCalls();
}

void User::checkAndCloseCalls() {
    if (this->relayConnId != 0) {
        return;
    }

    RWMutexLock rwlock(&mutex_0);
    rwlock.wlock();
    std::vector<uint64_t> &tmpCallVec = getCallVec();
    for (std::vector<uint64_t>::iterator iter = tmpCallVec.begin(); iter != tmpCallVec.end();) {
        uint64_t callId = *iter;
        P2PCallSession callSession;
        if (!getCallSessionNoSafe(callId, callSession)) {
            iter++;
            continue;
        }
        if (callSession.getP2PIntranetConnId() == 0 && callSession.getP2PInternetConnId() == 0) {
            pthread_t onlaunchCallThread;
            if (getLaunchCallThread(callId, onlaunchCallThread)) {
#ifndef __ANDROID__
                pthread_cancel(onlaunchCallThread);
#else
                if(pthread_kill(onlaunchCallThread, 0) != ESRCH)
    			{
        			pthread_kill(onlaunchCallThread, SIGTERM);
    			}
#endif
                deleteLaunchCall(callId);
            }
            if (callSession.getCallState() >= RUNNING) {
                RtsSendSignal::sendByeRequest(this, callId, ALL_DATA_CHANNELS_CLOSED,callSession);
                rtsCallEventHandler->onClosed(callId, ALL_DATA_CHANNELS_CLOSED);
            }
            deleteFromCurrentCallsMapNoSafe(callId);
        } else {
            iter++;
        }
    }
}

uint64_t User::getP2PIntranetConnId(uint64_t callId) {
    P2PCallSession callSession;
    if (!getCallSession(callId, callSession)) {
        return 0;
    }
    return callSession.getP2PIntranetConnId();
}

uint64_t User::getP2PInternetConnId(uint64_t callId) {
    P2PCallSession callSession;
    if (!getCallSession(callId, callSession)) {
        return 0;
    }
    return callSession.getP2PInternetConnId();
}

void User::checkToRunXmdTranseiver() {
    if (!this->xmdTranseiver) {
        this->xmdTranseiver = new XMDTransceiver(1);
        this->xmdTranseiver->start();
        this->rtsConnectionHandler = new RtsConnectionHandler(this);
        this->rtsStreamHandler = new RtsStreamHandler(this);
        //modified by huanghua on 2019-04-02
        this->rtsDatagramHandler = new RtsDatagramHandler(this);
        this->xmdTranseiver->registerConnHandler(this->rtsConnectionHandler);
        this->xmdTranseiver->registerStreamHandler(this->rtsStreamHandler);
        this->xmdTranseiver->registerRecvDatagramHandler(this->rtsDatagramHandler);
        this->xmdTranseiver->SetAckPacketResendIntervalMicroSecond(500);
        this->xmdTranseiver->setTestPacketLoss(this->testPacketLoss);
        if (this->xmdSendBufferSize > 0) {
            this->xmdTranseiver->setSendBufferSize(this->xmdSendBufferSize);
        }
        if (this->xmdRecvBufferSize > 0) {
            this->xmdTranseiver->setRecvBufferSize(this->xmdRecvBufferSize);
        }
        this->xmdTranseiver->run();
        //usleep(100000);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void User::initAudioStreamConfig(const RtsStreamConfig& audioStreamConfig) {
    this->audioStreamConfig = audioStreamConfig;
}

void User::initVideoStreamConfig(const RtsStreamConfig& videoStreamConfig) {
    this->videoStreamConfig = videoStreamConfig;
}

void User::setSendBufferMaxSize(int size) {
    if (size > 0) {
        if(this->xmdTranseiver) {
            this->xmdTranseiver->setSendBufferSize(size);
        } else {
            this->xmdSendBufferSize = size;
        }
    }
}

void User::setRecvBufferMaxSize(int size) {
    if (size > 0) {
        if(this->xmdTranseiver) {
            this->xmdTranseiver->setRecvBufferSize(size);
        } else {
            this->xmdRecvBufferSize = size;
        }
    }
}

int User::getSendBufferUsedSize() {
    return this->xmdTranseiver ? this->xmdTranseiver->getSendBufferUsedSize() : 0;
}

int User::getSendBufferMaxSize() {
    return this->xmdTranseiver ? this->xmdTranseiver->getSendBufferMaxSize() : xmdSendBufferSize;
}

int User::getRecvBufferMaxSize() {
    //to do
    return this->xmdTranseiver ? this->xmdTranseiver->getRecvBufferMaxSize() : xmdRecvBufferSize;
}

int User::getRecvBufferUsedSize() {
    return this->xmdTranseiver ? this->xmdTranseiver->getRecvBufferUsedSize() : 0;
}

float User::getSendBufferUsageRate() {
    return this->xmdTranseiver ? this->xmdTranseiver->getSendBufferUsageRate() : 0;
}

float User::getRecvBufferUsageRate() {
    return this->xmdTranseiver ? this->xmdTranseiver->getRecvBufferUsageRate() : 0;
}

void User::clearSendBuffer() {
    if(this->xmdTranseiver) {
        this->xmdTranseiver->clearSendBuffer();
    }
}

void User::clearRecvBuffer() {
    if(this->xmdTranseiver) {
        this->xmdTranseiver->clearRecvBuffer();
    }
}

void User::setTestPacketLoss(int testPacketLoss) {
    this->testPacketLoss = testPacketLoss;
    if(this->xmdTranseiver) {
        this->xmdTranseiver->setTestPacketLoss(testPacketLoss);
    }
}

void User::insertIntoCurrentCallsMap(uint64_t callId, P2PCallSession callSession) {
    RWMutexLock rwlock(&mutex_0);
    rwlock.wlock();
    currentCalls->insert(std::pair<uint64_t, P2PCallSession>(callId, callSession));
    currentCallVec.push_back(callId);
}

void User::insertIntoCurrentCallsMapNoSafe(uint64_t callId, P2PCallSession callSession) {
    /* RWMutexLock rwlock(&mutex_0);
    rwlock.wlock(); */
    currentCalls->insert(std::pair<uint64_t, P2PCallSession>(callId, callSession));
    currentCallVec.push_back(callId);
}

bool User::updateCurrentCallsMapNoSafe(uint64_t callId, P2PCallSession callSession) {
    /* RWMutexLock rwlock(&mutex_0);
    rwlock.wlock(); */
    if (this->currentCalls->count(callId) == 0) {
        return false;
    }
    (*currentCalls)[callId] = callSession;
    return true;
}

bool User::updateCurrentCallsMap(uint64_t callId, P2PCallSession callSession) {
    RWMutexLock rwlock(&mutex_0);
    rwlock.wlock();
    if (this->currentCalls->count(callId) == 0) {
        return false;
    }
    (*currentCalls)[callId] = callSession;
    return true;
}


bool User::deleteFromCurrentCallsMap(uint64_t callId) {
    RWMutexLock rwlock(&mutex_0);
    rwlock.wlock();
    if (this->currentCalls->count(callId) == 0) {
        return false;
    }
    bool result = currentCalls->erase(callId);

    std::vector<uint64_t>::iterator it = currentCallVec.begin();
    for (; it != currentCallVec.end(); it++) {
        if (*it == callId) {
            currentCallVec.erase(it);
            break;
        }
    }

    return result;
}

bool User::deleteFromCurrentCallsMapNoSafe(uint64_t callId) {
    /* RWMutexLock rwlock(&mutex_0);
    rwlock.wlock(); */
    if (this->currentCalls->count(callId) == 0) {
        return false;
    }
    bool result = currentCalls->erase(callId);

    std::vector<uint64_t>::iterator it = currentCallVec.begin();
    for (; it != currentCallVec.end(); it++) {
        if (*it == callId) {
            currentCallVec.erase(it);
            break;
        }
    }

    return result;
}

size_t User::currentCallsMapSize() {
    RWMutexLock rwlock(&mutex_0);
    rwlock.rlock();
    size_t size = currentCalls->size();
    return size;
}
size_t User::currentCallsMapSizeNoSafe() {
    /* RWMutexLock rwlock(&mutex_0);
    rwlock.rlock(); */
    size_t size = currentCalls->size();
    return size;
}

uint32_t User::countCurrentCallsMapNoSafe(uint64_t callId) {
    /* RWMutexLock rwlock(&mutex_0);
    rwlock.rlock(); */
    uint32_t result = currentCalls->count(callId);

    return result;
}

uint32_t User::countCurrentCallsMap(uint64_t callId) {
    RWMutexLock rwlock(&mutex_0);
    rwlock.rlock();
    uint32_t result = currentCalls->count(callId);

    return result;
}

bool User::getCallSession(uint64_t callId, P2PCallSession& callSession) const {
    RWMutexLock rwlock((pthread_rwlock_t*)&mutex_0);
    rwlock.rlock();
    if (this->currentCalls->count(callId) == 0) {
        return false;
    }
    callSession = this->currentCalls->at(callId);
    return true;
}

bool User::getCallSessionNoSafe(uint64_t callId, P2PCallSession& callSession) const {
    if (this->currentCalls->count(callId) == 0) {
        return false;
    }
    callSession = this->currentCalls->at(callId);
    return true;
}
/*
P2PCallSession User::getCallSession(uint64_t callId) {
    RWMutexLock rwlock(&mutex_0);
    rwlock.rlock();
    P2PCallSession callSession = this->currentCalls->at(callId);
    return callSession;
}*/


void User::clearCurrentCallsMap() {
    RWMutexLock rwlock(&mutex_0);
    rwlock.wlock();
    currentCalls->clear();
    currentCallVec.clear();
}
void User::clearCurrentCallsMapNoSafe() {
    currentCalls->clear();
    currentCallVec.clear();
}
/*
std::map<uint64_t, P2PCallSession> User::getCallSessionMap() {
    RWMutexLock rwlock(&mutex_0);
    rwlock.rlock();
    return *currentCalls;
}*/

std::vector<uint64_t>& User::getCallVec() {
    /* RWMutexLock rwlock(&mutex_0);
    rwlock.rlock(); */
    return currentCallVec;
}


bool User::getLaunchCallThread(uint64_t callId, pthread_t& thread) {
    RWMutexLock rwlock((pthread_rwlock_t*)&mutex_onlaunch);
    rwlock.rlock();
    if (this->onlaunchCalls->count(callId) == 0) {
        return false;
    }
    thread = this->onlaunchCalls->at(callId);
    return true;
}

bool User::deleteLaunchCall(uint64_t callId) {
    RWMutexLock rwlock(&mutex_onlaunch);
    rwlock.wlock();
    if (this->onlaunchCalls->count(callId) == 0) {
        return false;
    }
    bool result = onlaunchCalls->erase(callId);
    return result;
}

void User::insertLaunchCall(uint64_t callId, pthread_t thread) {
    RWMutexLock rwlock(&mutex_onlaunch);
    rwlock.wlock();
    onlaunchCalls->insert(std::pair<uint64_t, pthread_t>(callId, thread));
}

uint64_t User::generateCallId() {
    int64_t timesT = Utils::currentTimeMicros();
    srand(timesT);
    int64_t l = rand();
    uint64_t id = uuid << 32;

    timesT <<= 48;
    id |= timesT;
    id |= l;
    id = id >> 1; //适应java long类型

    return id;
}

// void User::setBaseOfBackoffWhenFetchToken(TimeUnit timeUnit, long base)
// {
//     backoffWhenFetchToken.setBase(timeUnit, base);
// }

// void User::setCapOfBackoffWhenFetchToken(TimeUnit timeUnit, long cap)
// {
//     backoffWhenFetchToken.setCap(timeUnit, cap);
// }

// void User::setBaseOfBackoffWhenConnectFe(TimeUnit timeUnit, long base)
// {
//     backoffWhenConnectFe.setBase(timeUnit, base);
// }

// void User::setCapOfBackoffWhenConnectFe(TimeUnit timeUnit, long cap)
// {
//     backoffWhenConnectFe.setCap(timeUnit, cap);
// }

void mimcBackoff::init()
{
    backoffWhenFetchToken.setBase(MILLISECONDS, 100);
    backoffWhenFetchToken.setCap(SECONDS, 30);
    backoffWhenFetchToken.setJitter(0.5);
    backoffWhenConnectFe.setBase(SECONDS, 1);
    backoffWhenConnectFe.setCap(SECONDS, 30);
    backoffWhenConnectFe.setJitter(0.5);
}