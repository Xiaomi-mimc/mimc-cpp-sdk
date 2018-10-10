#include "Thread.h"


void* XMDThread::Func(void* obj)
{
    XMDThread* th = (XMDThread*)obj;
    return th->process();
}

int XMDThread::run()
{
    int ret = pthread_create(&threadId_, NULL, XMDThread::Func, this);
    return ret;
}

int XMDThread::join()
{
    int ret = pthread_join(threadId_, NULL);
    return ret;
}


