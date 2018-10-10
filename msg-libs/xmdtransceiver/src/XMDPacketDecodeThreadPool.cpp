#include "XMDPacketDecodeThreadPool.h"

XMDPacketDecodeThreadPool::XMDPacketDecodeThreadPool(int pool_size, XMDCommonData* commonData, PacketDispatcher* dispatcher) {
    if (pool_size <= 0) {
        pool_size_ = 1;
    } else {
        pool_size_ = pool_size;
    }

    for (int i = 0; i < pool_size_; i++) {
        PackketDecodeThread* decodeThread = new PackketDecodeThread(i, commonData, dispatcher);
        decode_thread_pool_.push_back(decodeThread);
    }
}

XMDPacketDecodeThreadPool::~XMDPacketDecodeThreadPool() {
    for (std::size_t i = 0; i < decode_thread_pool_.size(); i++) {
        PackketDecodeThread* thread = decode_thread_pool_[i];
        delete thread;
        decode_thread_pool_[i] = NULL;
    }
    decode_thread_pool_.clear();
}


void XMDPacketDecodeThreadPool::run() {
    for (std::size_t i = 0; i < decode_thread_pool_.size(); i++) {
        decode_thread_pool_[i]->run();
    }
}

void XMDPacketDecodeThreadPool::join() {
    for (std::size_t i = 0; i < decode_thread_pool_.size(); i++) {
        decode_thread_pool_[i]->join();
    }
}


void XMDPacketDecodeThreadPool::stop() {
    for (std::size_t i = 0; i < decode_thread_pool_.size(); i++) {
        decode_thread_pool_[i]->stop();
    }
}



