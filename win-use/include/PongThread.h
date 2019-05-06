#ifndef PONGTHREAD_H
#define PONGTHREAD_H

#include "xmd_thread.h"
#include "XMDCommonData.h"


class PongThread : public XMDThread {
public:
    PongThread(XMDCommonData* commonData);
    ~PongThread();
    virtual void* process();
    void stop();

private:
    bool stopFlag_;
    XMDCommonData* commonData_;
};

#endif //PONGTHREAD_H



