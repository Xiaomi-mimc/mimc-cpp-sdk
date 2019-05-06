#include "MutexLock.h"

MutexLock::MutexLock(std::mutex * mutex /* = NULL */)
    : mutex(mutex) {
    lock();
}

MutexLock::~MutexLock() {
    unlock();
}

void MutexLock::setMutex(std::mutex * mutex) {
    this->mutex = mutex;
}

void MutexLock::lock() {
    if (mutex == NULL) {
        return;
    }

    mutex->lock();
}

void MutexLock::unlock() {
    if (mutex == NULL) {
        return;
    }

    mutex->unlock();
}

