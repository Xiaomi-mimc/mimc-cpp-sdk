#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <memory>
#include <arpa/inet.h>

#include "XMDRecvThread.h"
#include "LoggerWrapper.h"


XMDRecvThread::XMDRecvThread(int port,  XMDCommonData* commonData) {
    port_ = port;
    commonData_ = commonData;
    stopFlag_ = false;
    testNetFlag_ = false;

    listenfd_ = 0;
    if ((listenfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LoggerWrapper::instance()->error("Failed to create listen socket.");
        exit(2);
    }
    
    int on = 1;
    if (setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
        LoggerWrapper::instance()->error("Failed to set listen socket option SO_REUSEADDR.");
        exit(2);
    }
}

XMDRecvThread::~XMDRecvThread() {
    commonData_ = NULL;
    //delete packetDecoder_;
}

struct sockaddr_in* XMDRecvThread::getSvrAddr() {
    struct sockaddr_in* svrAddr = new sockaddr_in();
    memset(svrAddr, 0, sizeof(struct sockaddr_in));
    svrAddr->sin_family = AF_INET;
    svrAddr->sin_addr.s_addr = htonl(INADDR_ANY);
    svrAddr->sin_port = htons(port_);

    return svrAddr;
}

void XMDRecvThread::Bind(int fd) {
    struct sockaddr_in* svrAddr = getSvrAddr();
    if (bind(listenfd_, (struct sockaddr*) svrAddr, sizeof(struct sockaddr_in)) < 0) {
        LoggerWrapper::instance()->warn("Failed to bind port [%d], errmsg:%s,", port_, strerror(errno));
        exit(3);
    }
    delete svrAddr;
}

void XMDRecvThread::Recvfrom(int fd) {
    while (!stopFlag_) {
        static const int BUF_LEN = 4096;
        unsigned char buf[BUF_LEN];
        memset(buf, 0, BUF_LEN);
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        memset(&clientAddr, 0, addrLen);
        
        ssize_t len = recvfrom(fd, buf, BUF_LEN, MSG_DONTWAIT, (struct sockaddr*) (struct sockaddr *)&clientAddr, &addrLen);

        if (len < 0) {
            usleep(1000);
            continue;
        }

        //adapt dpdk keepalive packet
        uint32_t* dpdkPakcet = (uint32_t*)buf;
        uint32_t dpdkping = ntohl(*dpdkPakcet);
        if (len == 4 && dpdkping == 0x000c120f) {
            int ret = sendto(fd, (char*)buf, len, MSG_DONTWAIT, (struct sockaddr*)&clientAddr, addrLen);
            if (ret < 0) {
                LoggerWrapper::instance()->warn("dpdk ack send fail, errmsg:%s,", strerror(errno));
            }
            continue;
        }

        if (testNetFlag_) {
            continue;
        }

        
        uint16_t port = ntohs(clientAddr.sin_port);
        LoggerWrapper::instance()->debug("XMDRecvThread recv data,len=%d, port=%d", len, port);
        //std::cout<<"time="<<current_ms()<<std::endl;

        SocketData* socketData = new SocketData(clientAddr.sin_addr.s_addr, port, len, buf);
        commonData_->socketRecvQueuePush(socketData);
    }
}

void* XMDRecvThread::process() {
    if (port_ < 0) {
        LoggerWrapper::instance()->error("You should bind port > 0.");
        exit(1);
    }

    LoggerWrapper::instance()->info("XMDRecvThread started");

    std::string threadName = "rt" + std::to_string(port_) + "-recv";
    //XMDThread::SetThreadName(threadName);

    Bind(listenfd_);
    Recvfrom(listenfd_);

    return NULL;
}


void XMDRecvThread::stop() {
    stopFlag_ = true;
    close(listenfd_);
}

