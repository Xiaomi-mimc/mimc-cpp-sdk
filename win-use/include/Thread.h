#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <string>

#ifdef _WIN32
#else
#ifndef _IOS_MIMC_USE_
#include <sys/prctl.h>
#endif
#endif // _WIN32

class XMDThread {
public:
    XMDThread() {
#ifdef _WIN32
		threadId_.x = 0;
#else
		threadId_ = 0;
#endif // _WIN32
    }
    
    static void* Func(void* param);
    virtual void* process() = 0;
    virtual ~XMDThread() { }

    int run();
    int join();
    pthread_t threadId() { return threadId_; }
    

private:
    pthread_t threadId_;
};



#endif //THREAD_H
