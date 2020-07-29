#include "MutexLock.h"

MutexLock::MutexLock(pthread_mutex_t * mutex /* = NULL */)
    : mutex(mutex) {
    lock();
}

MutexLock::~MutexLock() {
    unlock();
}

void MutexLock::setMutex(pthread_mutex_t * mutex) {
    this->mutex = mutex;
}

void MutexLock::lock() {
    if (mutex == NULL) {
        return;
    }

    pthread_mutex_lock(mutex);
}

void MutexLock::unlock() {
    if (mutex == NULL) {
        return;
    }

    pthread_mutex_unlock(mutex);
}

