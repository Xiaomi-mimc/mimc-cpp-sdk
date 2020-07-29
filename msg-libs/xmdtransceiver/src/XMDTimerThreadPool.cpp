#include "XMDTimerThreadPool.h"

XMDTimerThreadPool::XMDTimerThreadPool(int pool_size, XMDCommonData* commonData) {
    if (pool_size <= 0) {
        pool_size_ = 1;
    } else {
        pool_size_ = pool_size;
    }

    for (int i = 0; i < pool_size_; i++) {
        TimerThread* timerThread = new TimerThread(i, commonData);
        timer_thread_pool_.push_back(timerThread);
    }
}

XMDTimerThreadPool::~XMDTimerThreadPool() {
    for (std::size_t i = 0; i < timer_thread_pool_.size(); i++) {
        TimerThread* timerThread = timer_thread_pool_[i];
        delete timerThread;
        timer_thread_pool_[i] = NULL;
    }
    timer_thread_pool_.clear();
}


void XMDTimerThreadPool::run() {
    for (std::size_t i = 0; i < timer_thread_pool_.size(); i++) {
        timer_thread_pool_[i]->run();
    }
}

void XMDTimerThreadPool::join() {
    for (std::size_t i = 0; i < timer_thread_pool_.size(); i++) {
        timer_thread_pool_[i]->join();
    }
}


void XMDTimerThreadPool::stop() {
    for (std::size_t i = 0; i < timer_thread_pool_.size(); i++) {
        timer_thread_pool_[i]->stop();
    }
}





