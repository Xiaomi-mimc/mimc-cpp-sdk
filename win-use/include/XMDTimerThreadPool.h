#ifndef TIMERTHREADPOOL_H
#define TIMERTHREADPOOL_H
#include <vector>
#include "XMDTimerThread.h"

class XMDTimerThreadPool {
private:
    std::vector<TimerThread*> timer_thread_pool_;
    int pool_size_;
public:
    XMDTimerThreadPool(int pool_size, XMDCommonData* commonData);
    ~XMDTimerThreadPool();
    void run();
    void stop();
    void join();
};

#endif //TIMERTHREADPOOL_H



