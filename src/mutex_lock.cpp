#include "mimc/mutex_lock.h"
#include <iostream>

RWMutexLock::RWMutexLock(pthread_rwlock_t * mutex /* = NULL */)
    : mutex(mutex) {
}

RWMutexLock::~RWMutexLock() {
    unlock();
}



void RWMutexLock::rlock() {
    if (mutex == NULL) {
        return;
    }

    pthread_rwlock_rdlock(mutex);
}

void RWMutexLock::wlock() {
    if (mutex == NULL) {
        return;
    }

    pthread_rwlock_wrlock(mutex);
}


void RWMutexLock::unlock() {
    if (mutex == NULL) {
        return;
    }
    pthread_rwlock_unlock(mutex);
}


MIMCMutexLock::MIMCMutexLock(pthread_mutex_t * mutex /* = NULL */)
    : mutex(mutex) {
    lock();
}

MIMCMutexLock::~MIMCMutexLock() {
    unlock();
}

void MIMCMutexLock::setMutex(pthread_mutex_t * mutex) {
    this->mutex = mutex;
}

void MIMCMutexLock::lock() {
    if (mutex == NULL) {
        return;
    }

    pthread_mutex_lock(mutex);
}

void MIMCMutexLock::unlock() {
    if (mutex == NULL) {
        return;
    }

    pthread_mutex_unlock(mutex);
}



