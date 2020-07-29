#ifndef DATAGRAMHANDLER_H
#define DATAGRAMHANDLER_H

#include <stdint.h>

class DatagramRecvHandler {
public:
    virtual void handle(char* ip, int port, char* data, uint32_t len, unsigned char packetType) {}
    virtual ~DatagramRecvHandler() {}
};


#endif //DATAGRAMHANDLER_H

