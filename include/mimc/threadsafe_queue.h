#ifndef MIMC_CPP_SDK_SAFEQUEUE_H
#define MIMC_CPP_SDK_SAFEQUEUE_H

#ifdef _WIN32
#else
#include <unistd.h>
#endif // _WIN32

#include <pthread.h>
#include <time.h>
#include <thread>
#include <chrono>
#include "mimc/mutex_lock.h"

template <typename T>
class ThreadSafeQueue
{

private:
    std::queue<T> queue_;
	std::mutex queue_mutex_;

public:
    ThreadSafeQueue();
    ThreadSafeQueue(const ThreadSafeQueue<T>& queue);
    ~ThreadSafeQueue();

    bool Push(T& val);
    bool Pop(T& ptr);
    bool empty();
    bool Front(T& ptr);
    size_t Size();
};

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue() {

}

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(const ThreadSafeQueue<T>& queue) {

}

template <typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue() {
}

template <typename T>
bool ThreadSafeQueue<T>::Push(T& val) {
	queue_mutex_.lock();
    queue_.push(val);
	queue_mutex_.unlock();
    return true;
}

template <typename T>
bool ThreadSafeQueue<T>::Pop(T& data) {
	queue_mutex_.lock();
    if (queue_.empty()) {
		queue_mutex_.unlock();
        return false;
    }
    data = queue_.front();
    queue_.pop();
	queue_mutex_.unlock();
    return true;
}

template <typename T>
bool ThreadSafeQueue<T>::empty() {
    return queue_.empty();
}

template <typename T>
bool ThreadSafeQueue<T>::Front(T& data) {
	queue_mutex_.lock();
    if (queue_.empty()) {
		queue_mutex_.unlock();
        return false;
    }
    data = queue_.front();
	queue_mutex_.unlock();
    return true;
}

template <typename T>
size_t ThreadSafeQueue<T>::Size() {
    return queue_.size();
}
#endif
