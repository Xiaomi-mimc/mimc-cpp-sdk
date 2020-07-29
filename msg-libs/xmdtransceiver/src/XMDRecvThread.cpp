#include "XMDRecvThread.h"
#include <cstdlib>
#include <cstring>

#include <errno.h>
#include <memory>

#include <signal.h>

#include "XMDLoggerWrapper.h"
#include "XMDTransceiver.h"
#include "sleep_define.h"
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
    int old_fd = listenfd_;
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
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
	if (iResult == SOCKET_ERROR) {
		XMDLoggerWrapper::instance()->error("Failed to set listen socket option SO_REUSEADDR.");
		iResult = closesocket(listenfd_);
		if (iResult == SOCKET_ERROR) {
			XMDLoggerWrapper::instance()->error("Failed to close socket.");
		}
		WSACleanup();
		return -1;
	}
	u_long iMode = 1;
	iResult = ioctlsocket(listenfd_, FIONBIO, &iMode);
	if (iResult != NO_ERROR) {
		XMDLoggerWrapper::instance()->error("Failed to set non-blocking mode.");
		iResult = closesocket(listenfd_);
		if (iResult == SOCKET_ERROR) {
			XMDLoggerWrapper::instance()->error("Failed to close socket.");
		}
		WSACleanup();
		return -1;
	}
#else
    
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
#endif
    
#ifdef XMD_IOS_
    int value = 1;
    if(setsockopt(listenfd_, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value)) != 0) {
        XMDLoggerWrapper::instance()->error("Failed to set listen socket option SO_NOSIGPIPE.");
        if (old_fd != 0) {
            shutdown(old_fd, SHUT_RDWR);
            close(old_fd);
        }
        return errno;
    }
#endif

    int ret = Bind(listenfd_);
    if(ret != 0) {
        if (old_fd != 0) {
#ifdef _WIN32
			closesocket(listenfd_);
#else
			shutdown(old_fd, SHUT_RDWR);
			close(old_fd);
#endif // DEBUG
        }
        return ret;
    }

    if (old_fd != 0) {
#ifdef _WIN32
		if (closesocket(listenfd_) != 0) {
			XMDLoggerWrapper::instance()->error("close old fd err.errmsg:%s", strerror(errno));
		}
#else
		shutdown(old_fd, SHUT_RDWR);
		if (close(old_fd) != 0) {
			XMDLoggerWrapper::instance()->error("close old fd err.errmsg:%s", strerror(errno));
		}
#endif // _WIN32
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

int XMDRecvThread::Bind(int fd) {
    struct sockaddr_in* svrAddr = getSvrAddr();
#ifdef _WIN32
	if (bind(fd, (struct sockaddr*) svrAddr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
#else
	if (bind(fd, (struct sockaddr*) svrAddr, sizeof(struct sockaddr_in)) < 0) {
#endif // _WIN32

        XMDLoggerWrapper::instance()->warn("Failed to bind port [%d], errmsg:%s,", port_, strerror(errno));
        return errno;
    }
    delete svrAddr;

    return 0;
}

void XMDRecvThread::Recvfrom(int fd) {
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
        
    while (!stopFlag_) {
#ifdef _WIN32
		int len = recvfrom(fd, buf, BUF_LEN, 0, (struct sockaddr*)&clientAddr, (int*)&addrLen);
		if (len == SOCKET_ERROR) {
			std::this_thread::sleep_for(std::chrono::microseconds(XMD_RECV_FROM_SLEEP_MS));
			continue;
		}
#else
		ssize_t len = recvfrom(fd, buf, BUF_LEN, 0, (struct sockaddr*) (struct sockaddr *)&clientAddr, &addrLen);
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

			usleep(XMD_RECV_FROM_SLEEP_MS);
			continue;
		}
#endif // _WIN32

        if (rand32() % 100 < testPacketLoss_) {
            XMDLoggerWrapper::instance()->debug("test drop received packet.");
            continue;
        }

        
        uint16_t port = ntohs(clientAddr.sin_port);
        XMDLoggerWrapper::instance()->debug("XMDRecvThread recv data,len=%d, port=%d", len, port);

        XMDPacket* xmdPacket = (XMDPacket*)buf;
        XMDType xmdType = (XMDType)xmdPacket->getType();
        uint64_t connId = 0;
        if (xmdType != DATAGRAM) {
            PacketType packetType = (PacketType)xmdPacket->getPacketType();
            if (packetType == CONN_BEGIN) {
                uint64_t tmpId = 0;
                trans_uint64_t(tmpId, (char*)(xmdPacket->data + 2));
                //uint64_t* tmpId = (uint64_t*)(xmdPacket->data + 2);
                connId = xmd_ntohll(tmpId);
            } else {
                uint64_t tmpId = 0;
                trans_uint64_t(tmpId, (char*)xmdPacket->data);
                //uint64_t* tmpId = (uint64_t*)xmdPacket->data;
                connId = xmd_ntohll(tmpId);
            }
        }

#ifdef _WIN32
		SocketData* socketData = new SocketData(clientAddr.sin_addr.s_addr, port, len, (unsigned char*)buf);
#else
		SocketData* socketData = new SocketData(clientAddr.sin_addr.s_addr, port, len, buf);
#endif // _WIN32

       WorkerThreadData* workerData = new WorkerThreadData(WORKER_RECV_DATA, socketData, sizeof(SocketData) + len);
       commonData_->workerQueuePush(workerData, connId);
    }
}

void* XMDRecvThread::process() {
    if (port_ < 0) {
        XMDLoggerWrapper::instance()->error("You should bind port > 0.");
        return NULL;
    }

    XMDLoggerWrapper::instance()->info("XMDRecvThread started");

    //std::string threadName = "rt" + std::to_string(port_) + "-recv";
    char tname[16];
    memset(tname, 0, 16);
    strcpy(tname, "xmd recv");

#ifdef _WIN32
#else
#ifdef _IOS_MIMC_USE_ 
    pthread_setname_np(tname);
#else       
    prctl(PR_SET_NAME, tname);
#endif    
#endif // _WIN32

    Recvfrom(listenfd_);

    XMDLoggerWrapper::instance()->debug("xmd recv thread stop end");
    return NULL;
}


void XMDRecvThread::stop() {
    stopFlag_ = true;
#ifdef _WIN32
	closesocket(listenfd_);
	WSACleanup();
#else
	shutdown(listenfd_, SHUT_RDWR);
	close(listenfd_);
#endif // _WIN32

}

