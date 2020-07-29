#ifndef MUTEXLOCK_H_
#define MUTEXLOCK_H_

#include <pthread.h>

class MutexLock {
public:
    MutexLock(pthread_mutex_t * mutex = NULL);
    virtual ~MutexLock();

public:
    void setMutex(pthread_mutex_t * mutex);
    void lock();
    void unlock();

protected:
    pthread_mutex_t * mutex;
};

#endif

