#include <mimc/connection.h>
#include <mimc/constant.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <time.h>
#include <crypto/rc4_crypto.h>

Connection::Connection()
    : version(BODY_PAYLOAD_CONN_VERSION), model(""), os(""), udid(Utils::generateRandomString(10)), sdk(BODY_PAYLOAD_CONN_SDK), connpt(""), host(""), port(""), locale(""), challenge(""), body_key(""), state(NOT_CONNECTED), andver(0), nextResetSockTimestamp(-1)
{

}

void Connection::resetSock() {
    close(socketfd);
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
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return false;
    }
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(FE_PORT);
#ifndef STAGING
   struct hostent *pHostEnt = NULL;
   pHostEnt = gethostbyname(FE_DOMAIN);
   dest_addr.sin_addr.s_addr = *((unsigned long *)pHostEnt->h_addr_list[0]);
#else
    dest_addr.sin_addr.s_addr = inet_addr(FE_IP);
#endif
    if (::connect(socketfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        close(socketfd);
        return false;
    } else {
        return true;
    }
}

ssize_t Connection::writen(int fd, const void *buf, size_t nbytes) {
    if (fd < 0 || buf == NULL || nbytes == 0) {
        return -1;
    }
    size_t nLeft = nbytes;
    const unsigned char *bufTemp = (unsigned char *)buf;
    while (nLeft > 0) {
        ssize_t nWrite = 0;
        nWrite = write(fd, bufTemp, nLeft);
        if (nWrite <= 0) {
            if (errno == EINTR) {
                continue;
            }
            
            return -1;
        }
        nLeft -= nWrite;
        bufTemp += nWrite;
    }
    return nbytes;
}

ssize_t Connection::readn(int fd, void *buf, size_t nbytes) {
    if (fd < 0 || buf == NULL || nbytes == 0) {
        return -1;
    }
    size_t nLeft = nbytes;
    char *bufTemp = (char *)buf;
    while (nLeft > 0) {
        ssize_t nRead = 0;
        nRead = read(fd, bufTemp, nLeft);
        if (nRead < 0) {
            if (errno == EINTR) {
                continue;
            }
            
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