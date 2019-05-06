#include <mimc/connection.h>
#include <mimc/constant.h>
#include <string.h>
#include <sys/types.h>
#include <cerrno>
#include <cstdlib>
#include <time.h>
#include <crypto/rc4_crypto.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif // _WIN32

Connection::Connection()
    : version(BODY_PAYLOAD_CONN_VERSION), model(""), os(""), udid(Utils::generateRandomString(10)), sdk(BODY_PAYLOAD_CONN_SDK), connpt(""), host(""), port(""), locale(""), challenge(""), body_key(""), state(NOT_CONNECTED), andver(0), nextResetSockTimestamp(-1)
{

}

void Connection::resetSock() {
#ifdef _WIN32
	closesocket(socketfd);
#else
	close(socketfd);
#endif // _WIN32

    setState(NOT_CONNECTED);
    user->setLastLoginTimestamp(0);
    user->setLastCreateConnTimestamp(0);
    user->setOnlineStatus(Offline);
    user->getStatusHandler()->statusChange(Offline, "", "NETWORK_RESET", "NETWORK_RESET");
    challenge = "";
    body_key = "";
    clearNextResetSockTs();
}

bool Connection::connect() {
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;

#ifndef STAGING
	pthread_mutex_lock(&user->getAddressMutex());
	std::vector<std::string>& feAddresses = user->getFeAddresses();
	for (std::vector<std::string>::iterator iter = feAddresses.begin(); iter != feAddresses.end(); iter++) {

#ifdef _WIN32
		WORD sockVersion = MAKEWORD(2, 2);
		WSADATA data;
		if (WSAStartup(sockVersion, &data) != 0){
			continue;
		}
		socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (socketfd == INVALID_SOCKET){
			continue;
		}
#else
		if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			continue;
		}
#endif // _WIN32

		
		std::string& feAddr = *iter;
		int pos = feAddr.find(":");
		if (pos == std::string::npos) {
			continue;
		}
		std::string feIp = feAddr.substr(0, pos);
		std::string fePort = feAddr.substr(pos + 1);

#ifdef _WIN32
		dest_addr.sin_addr.S_un.S_addr = inet_addr(feIp.c_str());
		dest_addr.sin_port = htons(atoi(fePort.c_str()));

		socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (::connect(socketfd, (sockaddr *)&dest_addr, sizeof(dest_addr)) == SOCKET_ERROR){  //连接失败 
			closesocket(socketfd);
			iter = feAddresses.erase(iter);
		}
		else {
			pthread_mutex_unlock(&user->getAddressMutex());
			return true;
		}

#else
		dest_addr.sin_addr.s_addr = inet_addr(feIp.c_str());
		dest_addr.sin_port = htons(atoi(fePort.c_str()));
		if (::connect(socketfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
			close(socketfd);
			iter = feAddresses.erase(iter);
		}
		else {
			pthread_mutex_unlock(&user->getAddressMutex());
			return true;
		}
#endif // _WIN32

}

#ifdef _WIN32
	user->setAddressInvalid(true);
	pthread_mutex_unlock(&user->getAddressMutex());
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA data;
	if (WSAStartup(sockVersion, &data) != 0) {
		return false;
	}
	socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketfd == INVALID_SOCKET) {
		return false;
	}

	struct hostent *pHostEnt = NULL;
	pHostEnt = gethostbyname(FE_DOMAIN);
	dest_addr.sin_addr.S_un.S_addr = *((unsigned long *)pHostEnt->h_addr_list[0]);
	dest_addr.sin_port = htons(FE_PORT);
	if (::connect(socketfd, (sockaddr *)&dest_addr, sizeof(dest_addr)) == SOCKET_ERROR) {
		closesocket(socketfd);
		return false;
	}
	else {
		return true;
	}
#else
	user->setAddressInvalid(true);
	pthread_mutex_unlock(&user->getAddressMutex());
	if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return false;
	}
	struct hostent *pHostEnt = NULL;
	pHostEnt = gethostbyname(FE_DOMAIN);
	dest_addr.sin_addr.s_addr = *((unsigned long *)pHostEnt->h_addr_list[0]);
	dest_addr.sin_port = htons(FE_PORT);
	if (::connect(socketfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
		close(socketfd);
		return false;
	}
	else {
		return true;
	}
#endif // _WIN32

	
    
#else

#ifdef _WIN32
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA data;
	if (WSAStartup(sockVersion, &data) != 0)
	{
		return false;
	}
	socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketfd == INVALID_SOCKET)
	{
		return false;
	}
	dest_addr.sin_addr.S_un.S_addr = inet_addr(FE_IP);
	dest_addr.sin_port = htons(FE_PORT);
	if (::connect(socketfd, (sockaddr *)&dest_addr, sizeof(dest_addr)) == SOCKET_ERROR)
	{  //连接失败 
		closesocket(socketfd);
		return false;
	}
	return true;
	
#else
	if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return false;
}
	dest_addr.sin_addr.s_addr = inet_addr(FE_IP);
	dest_addr.sin_port = htons(FE_PORT);
	if (::connect(socketfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
		close(socketfd);
		return false;
	}
	else {
		return true;
	}
#endif // _WIN32

    
#endif
}

ssize_t Connection::writen(const void *buf, size_t nbytes) {
    if (socketfd < 0 || buf == NULL || nbytes == 0) {
        return -1;
    }
    size_t nLeft = nbytes;
    const unsigned char *bufTemp = (unsigned char *)buf;
    while (nLeft > 0) {
        ssize_t nWrite = 0;
#ifdef _WIN32
		nWrite = send(socketfd, (const char*)bufTemp, (int)nLeft, NULL);
		if (nWrite == SOCKET_ERROR) {
			closesocket(socketfd);
			WSACleanup();
			return -1;
	}
#else
		nWrite = write(socketfd, bufTemp, nLeft);
		if (nWrite <= 0) {
			if (errno == EINTR) {
				continue;
			}

			return -1;
		}
#endif // _WIN32

        nLeft -= nWrite;
        bufTemp += nWrite;
    }
    return nbytes;
}

ssize_t Connection::readn(void *buf, size_t nbytes) {
    if (socketfd < 0 || buf == NULL || nbytes == 0) {
        return -1;
    }
    size_t nLeft = nbytes;
    char *bufTemp = (char *)buf;
    while (nLeft > 0) {
        ssize_t nRead = 0;
#ifdef _WIN32
		nRead = recv(socketfd, bufTemp, nLeft, 0);
#else
		nRead = read(socketfd, bufTemp, nLeft);
#endif // _WIN32


        if (nRead < 0) {
#ifdef _WIN32
			if (errno == WSAEINTR) {
				continue;
			}
#else
			if (errno == EINTR) {
				continue;
			}
#endif // _WIN32

            return -1;
        }
        if (nRead == 0) {
            break;
        }
        nLeft -= nRead;
        bufTemp += nRead;
    }
    return nbytes - nLeft;
}

void Connection::trySetNextResetTs() {
    if (this->nextResetSockTimestamp > 0) {
        return;
    }
    this->nextResetSockTimestamp = time(NULL) + RESETSOCK_TIMEOUT;
}

void Connection::setChallengeAndBodyKey(const std::string &challenge) {
    this->challenge = challenge;
    std::string tmp_key = challenge.substr(challenge.length() / 2) + udid.substr(udid.length() / 2);
    ccb::CryptoRC4Util::Encrypt(tmp_key, this->body_key, challenge);
}