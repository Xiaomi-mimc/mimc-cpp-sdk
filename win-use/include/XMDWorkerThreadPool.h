#ifndef WORKERTHREADPOOL_H
#define WORKERTHREADPOOL_H
#include <vector>
#include "XMDWorkerThread.h"

class XMDWorkerThreadPool {
private:
    std::vector<WorkerThread*> worker_thread_pool_;
    int pool_size_;
public:
    XMDWorkerThreadPool(int pool_size, XMDCommonData* commonData, PacketDispatcher* dispatcher);
    ~XMDWorkerThreadPool();
    void run();
    void stop();
    void join();
};

#endif //WORKERTHREADPOOL_H


