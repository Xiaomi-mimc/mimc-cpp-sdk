#include "XMDWorkerThreadPool.h"

XMDWorkerThreadPool::XMDWorkerThreadPool(int pool_size, XMDCommonData* commonData, PacketDispatcher* dispatcher) {
    if (pool_size <= 0) {
        pool_size_ = 1;
    } else {
        pool_size_ = pool_size;
    }

    for (int i = 0; i < pool_size_; i++) {
        WorkerThread* workerThread = new WorkerThread(i, commonData, dispatcher);
        worker_thread_pool_.push_back(workerThread);
    }
}

XMDWorkerThreadPool::~XMDWorkerThreadPool() {
    for (std::size_t i = 0; i < worker_thread_pool_.size(); i++) {
        WorkerThread* workerThread = worker_thread_pool_[i];
        delete workerThread;
        worker_thread_pool_[i] = NULL;
    }
    worker_thread_pool_.clear();
}


void XMDWorkerThreadPool::run() {
    for (std::size_t i = 0; i < worker_thread_pool_.size(); i++) {
        worker_thread_pool_[i]->run();
    }
}

void XMDWorkerThreadPool::join() {
    for (std::size_t i = 0; i < worker_thread_pool_.size(); i++) {
        worker_thread_pool_[i]->join();
    }
}


void XMDWorkerThreadPool::stop() {
    for (std::size_t i = 0; i < worker_thread_pool_.size(); i++) {
        worker_thread_pool_[i]->stop();
    }
}




