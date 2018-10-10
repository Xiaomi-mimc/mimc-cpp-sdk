#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <string>


class XMDThread {
public:
    XMDThread() {
        threadId_ = 0;
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
