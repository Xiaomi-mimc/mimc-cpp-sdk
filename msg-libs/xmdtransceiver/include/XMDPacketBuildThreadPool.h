#ifndef PACKETBUILDETHREADPOOL_H
#define PACKETBUILDETHREADPOOL_H
#include <vector>
#include "XMDPacketBuildThread.h"


class XMDPacketBuildThreadPool {
private:
    std::vector<PackketBuildThread*> build_thread_pool_;
    int pool_size_;
public:
    XMDPacketBuildThreadPool(int pool_size, XMDCommonData* commonData);
    ~XMDPacketBuildThreadPool();
    void run();
    void stop();
    void join();
};

#endif //PACKETBUILDETHREADPOOL_H

