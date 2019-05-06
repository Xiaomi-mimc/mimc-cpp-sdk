#include "XMDRecvThread.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <memory>
#include <errno.h>
#include <thread>
#include <chrono>
#include "XMDLoggerWrapper.h"
#include "XMDTransceiver.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif // _WIN32



XMDRecvThread::XMDRecvThread(int port,  XMDCommonData* commonData, PacketDispatcher* dispactcher, XMDTransceiver* transceiver) {
    port_ = port;
    commonData_ = commonData;
    stopFlag_ = false;
    testPacketLoss_ = 0;
    listenfd_ = 0;
    transceiver_ = transceiver;
    is_reset_socket_ = true;
    recv_fail_count_ = 0;
    dispatcher_ = dispactcher;
}

XMDRecvThread::~XMDRecvThread() {
    commonData_ = NULL;
    //delete packetDecoder_;
}

int XMDRecvThread::InitSocket() {
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
		XMDLoggerWrapper::instance()->error("WSAStartup(MAKEWORD(2, 2), &wsaData) execute failed!");
		return -1;
	}
	listenfd_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (listenfd_ == INVALID_SOCKET) {
		XMDLoggerWrapper::instance()->error("Failed to open socket.");
		WSACleanup();
		return -1;
	}
	int on = 1;
	int iResult = setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if (iResult == SOCKET_ERROR){
		XMDLoggerWrapper::instance()->error("Failed to set listen socket option SO_REUSEADDR.");
		return -1;
	}
	u_long iMode = 1;
	iResult = ioctlsocket(listenfd_, FIONBIO, &iMode);
	if (iResult != NO_ERROR) {
		XMDLoggerWrapper::instance()->error("Failed to set non-blocking mode.");
		return -1;
	}
#else
	int old_fd = listenfd_;
	if ((listenfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		XMDLoggerWrapper::instance()->error("Failed to create listen socket.");
		if (old_fd != 0) {
            shutdown(old_fd, SHUT_RDWR);
            close(old_fd);
        }
		return errno;
	}

	int on = 1;
	if (setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
		XMDLoggerWrapper::instance()->error("Failed to set listen socket option SO_REUSEADDR.");
		if (old_fd != 0) {
            shutdown(old_fd, SHUT_RDWR);
            close(old_fd);
        }
        return errno;
	}
	
	if (old_fd != 0) {
        shutdown(old_fd, SHUT_RDWR);
        if (close(old_fd) != 0) {
            XMDLoggerWrapper::instance()->error("close old fd err.errmsg:%s", strerror(errno));
        }
    }
#endif // _WIN32



    int ret = Bind(listenfd_);
    if(ret != 0) {
        return ret;
    }


    return 0;
}

struct sockaddr_in* XMDRecvThread::getSvrAddr() {
    struct sockaddr_in* svrAddr = new sockaddr_in();
    memset(svrAddr, 0, sizeof(struct sockaddr_in));
    svrAddr->sin_family = AF_INET;
    svrAddr->sin_addr.s_addr = htonl(INADDR_ANY);
    svrAddr->sin_port = htons(port_);

    return svrAddr;
}

int XMDRecvThread::Bind(int fd) {    //fd is useless?
    struct sockaddr_in* svrAddr = getSvrAddr();
#ifdef _WIN32
	if (bind(fd, (struct sockaddr*) svrAddr, sizeof(struct sockaddr_in)) ==  SOCKET_ERROR) {
#else
	if (bind(fd, (struct sockaddr*) svrAddr, sizeof(struct sockaddr_in)) < 0) {
#endif // _WIN32
        XMDLoggerWrapper::instance()->warn("Failed to bind port [%d], errmsg:%s,", port_, strerror(errno));
        return -1;
    }
    delete svrAddr;

    return 0;
}

void XMDRecvThread::Recvfrom(int fd) {
    while (!stopFlag_) {
        static const int BUF_LEN = 4096;
#ifdef _WIN32
		char buf[BUF_LEN];
#else
		unsigned char buf[BUF_LEN];
#endif // _WIN32

        memset(buf, 0, BUF_LEN);
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        memset(&clientAddr, 0, addrLen);

#ifdef _WIN32
		int len = recvfrom(fd, buf, BUF_LEN, 0, (struct sockaddr*)&clientAddr, (int*)&addrLen);
		if(len == SOCKET_ERROR){
		    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
#else
		signed long len = recvfrom(fd, buf, BUF_LEN, MSG_DONTWAIT, (struct sockaddr*) (struct sockaddr *)&clientAddr, &addrLen);
		if (len <= 0) {
		    if (is_reset_socket_) {
                recv_fail_count_++;
                if (recv_fail_count_ >= 2) {
                    dispatcher_->handleSocketError(errno, "socket recv err");
                    is_reset_socket_ = false;
                }
                
                if (!stopFlag_ && transceiver_->resetSocket() != 0) {
                    is_reset_socket_ = false;
                }
            }
            usleep(1000);
            continue;
        }
#endif // _WIN32

        recv_fail_count_ = 0;
        is_reset_socket_ = true;
        dispatcher_->setIsCallBackSocketErr(true);

        //adapt dpdk keepalive packet
        //uint32_t* dpdkPakcet = (uint32_t*)buf;
        uint32_t dpdkPacket = 0;
        trans_uint32_t(dpdkPacket, (char*)buf);
        uint32_t dpdkping = ntohl(dpdkPacket);
        if (len == 4 && dpdkping == 0x000c120f) {
#ifdef _WIN32
			int ret = sendto(fd, (char*)buf, len, 0, (struct sockaddr*)&clientAddr, addrLen);	
			if (ret == SOCKET_ERROR) {
			   XMDLoggerWrapper::instance()->warn("dpdk ack send fail, errmsg:%s,", strerror(errno));
            }
            continue;
#else
			int ret = sendto(fd, (char*)buf, len, MSG_DONTWAIT, (struct sockaddr*)&clientAddr, addrLen);
			if (ret < 0) {
			    XMDLoggerWrapper::instance()->warn("dpdk ack send fail, errmsg:%s,", strerror(errno));
                continue;
            }
#endif // _WIN32
        }


        if (rand32() % 100 < testPacketLoss_) {
			XMDLoggerWrapper::instance()->info("test drop received packet.");
            continue;
        }

        
        uint16_t port = ntohs(clientAddr.sin_port);
        //XMDLoggerWrapper::instance()->debug("XMDRecvThread recv data,len=%d, port=%d", len, port);

#ifdef _WIN32
		SocketData* socketData = new SocketData(clientAddr.sin_addr.s_addr, port, len, (unsigned char*)buf);
#else
		SocketData* socketData = new SocketData(clientAddr.sin_addr.s_addr, port, len, buf);
#endif // _WIN32

        commonData_->socketRecvQueuePush(socketData);
    }
}

void* XMDRecvThread::process() {
    if (port_ < 0) {
        XMDLoggerWrapper::instance()->error("You should bind port > 0.");
        return NULL;
    }

    XMDLoggerWrapper::instance()->info("XMDRecvThread started");

    char str[15] = {0};
    sprintf(str, "rt%u-recv", port_);
    std::string threadName = str;
//    std::string threadName = "rt" + std::to_string(port_) + "-recv";


    Recvfrom(listenfd_);

    return NULL;
}


void XMDRecvThread::stop() {
    stopFlag_ = true;
#ifdef _WIN32
	closesocket(listenfd_);
	WSACleanup();
#else
    shutdown(listenfd_,SHUT_RDWR);
	close(listenfd_);
#endif // _WIN32
}

