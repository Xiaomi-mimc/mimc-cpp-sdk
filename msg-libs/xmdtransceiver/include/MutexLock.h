#ifndef MUTEXLOCK_H_
#define MUTEXLOCK_H_

#include <mutex>

class MutexLock {
public:
    MutexLock(std::mutex * mutex = NULL);
    virtual ~MutexLock();

public:
    void setMutex(std::mutex * mutex);
    void lock();
    void unlock();

protected:
    std::mutex * mutex;
};

#endif

