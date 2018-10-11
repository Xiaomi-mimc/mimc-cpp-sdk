#ifndef MIMC_CPP_SDK_CONNECTION_H
#define MIMC_CPP_SDK_CONNECTION_H

#include <string>
#include <mimc/user.h>
#include <mimc/utils.h>

enum ConnState {
    NOT_CONNECTED,
    SOCK_CONNECTED,
    HANDSHAKE_CONNECTED
};
class Connection {
public:
    Connection();
    bool connect();
    void resetSock();
    ssize_t writen(int fd, const void *buf, size_t nbytes);
    ssize_t readn(int fd, void *buf, size_t nbytes);

    void setState(ConnState state) { this->state = state; }
    void setUser(User * user) { this->user = user; }
    void setChallengeAndBodyKey(const std::string &challenge);

    unsigned int getVersion() { return version; }
    int getSdk() { return sdk; }
    int getAndver() const{ return andver; }
    int getSock() { return socketfd; }
    std::string getModel() const{ return model; }
    std::string getOs() const{ return os; }
    std::string getUdid() const{ return udid; }
    std::string getConnpt() const{ return connpt; }
    std::string getHost() const{ return host; }
    std::string getPort() const{ return port; }
    std::string getLocale() const{ return locale; }
    std::string getChallenge() const{ return challenge; }
    std::string getBodyKey() const{ return body_key; }
    User * getUser() const{ return user; }
    ConnState getState() { return state; }

    long getNextResetSockTs() { return nextResetSockTimestamp; }
    void clearNextResetSockTs() { this->nextResetSockTimestamp = -1; }
    void trySetNextResetTs();
private:
    unsigned int version;
    int sdk;
    int andver;
    int socketfd;
    long nextResetSockTimestamp;
    std::string model;
    std::string os;
    std::string udid;
    std::string connpt;
    std::string host;
    std::string port;
    std::string locale;
    std::string challenge;
    std::string body_key;
    ConnState state;
    User * user;
};

#endif //MIMC_CPP_SDK_CONNECTION_H