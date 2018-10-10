#ifndef RECOVERTHREADPOOL_H
#define RECOVERTHREADPOOL_H
#include <vector>
#include "XMDPacketRecoverThread.h"

class XMDPacketRecoverThreadPool {
private:
    std::vector<XMDPacketRecoverThread*> recover_thread_pool_;
    int pool_size_;
public:
    XMDPacketRecoverThreadPool(int pool_size, XMDCommonData* commonData);
    ~XMDPacketRecoverThreadPool();
    void run();
    void stop();
    void join();
};

#endif //RECOVERTHREADPOOL_H