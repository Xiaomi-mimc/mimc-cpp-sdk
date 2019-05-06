#ifndef XMD_BLOCK_QUEUE_H
#define XMD_BLOCK_QUEUE_H

#include <memory>
#include <queue>
#include <pthread.h>
#include <condition_variable>
#include <mutex>
#include "XMDLoggerWrapper.h"


template <typename T>
class STLBlockQueue
{
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable con_var_;
public:
    using MutexLockGuard = std::lock_guard<std::mutex>;
    
    STLBlockQueue();
    ~STLBlockQueue();

    STLBlockQueue(const STLBlockQueue<T>& queue);

    bool Push(T& val);
    bool Pop(T& ptr);
    bool empty();
    void Notify();
    bool Front(T& ptr);
    size_t Size();
};

template <typename T>
STLBlockQueue<T>::STLBlockQueue() {
}

template <typename T>
STLBlockQueue<T>::~STLBlockQueue() {
}

template <typename T>
STLBlockQueue<T>::STLBlockQueue(const STLBlockQueue<T>& queue) {
}


template <typename T>
bool STLBlockQueue<T>::Push(T& val) {
    {
        MutexLockGuard lock(mutex_);
        queue_.push(val);
    }
    con_var_.notify_all();
    return true;
}

template <typename T>
bool STLBlockQueue<T>::Pop(T& data) {
    std::unique_lock <std::mutex> lck(mutex_);
    if (queue_.empty()) {
        con_var_.wait(lck);
        if (queue_.empty()) {
            return false;
        }
    }
    data = queue_.front();
    queue_.pop();
    return true;
}

template <typename T>
bool STLBlockQueue<T>::empty() {
    return queue_.empty();
}

template <typename T>
bool STLBlockQueue<T>::Front(T& data) {
    MutexLockGuard lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    data = queue_.front();
    return true;
}

template <typename T>
size_t STLBlockQueue<T>::Size() {
    return queue_.size();
}

template <typename T>
void STLBlockQueue<T>::Notify() {
    con_var_.notify_all();
}



#endif // _CCB_DISPATCH_QUEUE_H

