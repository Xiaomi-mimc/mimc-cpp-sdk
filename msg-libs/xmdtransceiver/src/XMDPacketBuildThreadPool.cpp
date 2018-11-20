#include "XMDPacketBuildThreadPool.h"

XMDPacketBuildThreadPool::XMDPacketBuildThreadPool(int pool_size, XMDCommonData* commonData, PacketDispatcher* dispatcher) {
    if (pool_size <= 0) {
        pool_size_ = 1;
    } else {
        pool_size_ = pool_size;
    }

    for (int i = 0; i < pool_size_; i++) {
        PackketBuildThread* buildThread = new PackketBuildThread(i, commonData, dispatcher);
        build_thread_pool_.push_back(buildThread);
    }
}

XMDPacketBuildThreadPool::~XMDPacketBuildThreadPool() {
    for (std::size_t i = 0; i < build_thread_pool_.size(); i++) {
        PackketBuildThread* buildThread = build_thread_pool_[i];
        delete buildThread;
        build_thread_pool_[i] = NULL;
    }
    build_thread_pool_.clear();
}


void XMDPacketBuildThreadPool::run() {
    for (std::size_t i = 0; i < build_thread_pool_.size(); i++) {
        build_thread_pool_[i]->run();
    }
}

void XMDPacketBuildThreadPool::join() {
    for (std::size_t i = 0; i < build_thread_pool_.size(); i++) {
        build_thread_pool_[i]->join();
    }
}


void XMDPacketBuildThreadPool::stop() {
    for (std::size_t i = 0; i < build_thread_pool_.size(); i++) {
        build_thread_pool_[i]->stop();
    }
}



