#ifndef CALLBACKTHREAD_H
#define CALLBACKTHREAD_H

#include "Thread.h"
#include "PacketDispatcher.h"
#include "XMDCommonData.h"


class XMDCallbackThread : public XMDThread {
public:
    XMDCallbackThread(PacketDispatcher* dispatcher, XMDCommonData* commonData);
    ~XMDCallbackThread();
    virtual void* process();
    void stop();

private:
    bool stopFlag_;
    XMDCommonData* commonData_;
    PacketDispatcher* dispatcher_;
};

#endif //CALLBACKTHREAD_H

