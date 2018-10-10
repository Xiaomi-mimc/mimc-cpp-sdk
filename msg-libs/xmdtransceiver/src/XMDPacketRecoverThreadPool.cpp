#include "XMDPacketRecoverThreadPool.h"

XMDPacketRecoverThreadPool::XMDPacketRecoverThreadPool(int pool_size, XMDCommonData* commonData) {
    if (pool_size <= 0) {
        pool_size_ = 1;
    } else {
        pool_size_ = pool_size;
    }

    for (int i = 0; i < pool_size_; i++) {
        XMDPacketRecoverThread* recoverThread = new XMDPacketRecoverThread(i, commonData);
        recover_thread_pool_.push_back(recoverThread);
    }
}

XMDPacketRecoverThreadPool::~XMDPacketRecoverThreadPool() {
    for (std::size_t i = 0; i < recover_thread_pool_.size(); i++) {
        XMDPacketRecoverThread* thread = recover_thread_pool_[i];
        delete thread;
        recover_thread_pool_[i] = NULL;
    }
    recover_thread_pool_.clear();
}


void XMDPacketRecoverThreadPool::run() {
    for (std::size_t i = 0; i < recover_thread_pool_.size(); i++) {
        recover_thread_pool_[i]->run();
    }
}

void XMDPacketRecoverThreadPool::join() {
    for (std::size_t i = 0; i < recover_thread_pool_.size(); i++) {
        recover_thread_pool_[i]->join();
    }
}


void XMDPacketRecoverThreadPool::stop() {
    for (std::size_t i = 0; i < recover_thread_pool_.size(); i++) {
        recover_thread_pool_[i]->stop();
    }
}


