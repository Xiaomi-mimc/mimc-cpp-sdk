#ifndef TIMERTHREAD_H
#define TIMERTHREAD_H

#include "Thread.h"
#include "XMDCommonData.h"


class TimerThread : public XMDThread {
public:
    virtual void* process();
    TimerThread(int id, XMDCommonData* commonData);
    ~TimerThread();

    void stop();
    

private:
    bool stopFlag_;
    int thread_id_;
    XMDCommonData* commonData_;
};

#endif //TIMERTHREAD_H




