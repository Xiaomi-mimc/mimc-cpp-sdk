#ifndef MUTEXLOCK_H_
#define MUTEXLOCK_H_

#include <pthread.h>

class RWMutexLock {
public:
    RWMutexLock(pthread_rwlock_t * mutex = NULL);
    virtual ~RWMutexLock();

public:
    void setMutex(pthread_rwlock_t * mutex);
    void rlock();
    void wlock();
    void unlock();

protected:
    pthread_rwlock_t * mutex;
};

class MIMCMutexLock {
public:
    MIMCMutexLock(pthread_mutex_t * mutex = NULL);
    virtual ~MIMCMutexLock();

public:
    void setMutex(pthread_mutex_t * mutex);
    void lock();
    void unlock();

protected:
    pthread_mutex_t * mutex;
};


#endif


