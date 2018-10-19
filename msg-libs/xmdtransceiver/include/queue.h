#ifndef XMD_QUEUE_H
#define XMD_QUEUE_H

#include <memory>
#include <queue>
#include <pthread.h>


template <typename T>
class STLSafeQueue
{
private:
    std::queue<T> queue_;
    pthread_mutex_t queue_mutex_;
public:
    STLSafeQueue();
    ~STLSafeQueue();

    bool Push(T& val);
    bool Pop(T& ptr);
    bool empty();
    bool Front(T& ptr);
};

template <typename T>
STLSafeQueue<T>::STLSafeQueue() {
    queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
}

template <typename T>
STLSafeQueue<T>::~STLSafeQueue() {
}

template <typename T>
bool STLSafeQueue<T>::Push(T& val) {
    pthread_mutex_lock(&queue_mutex_);
    queue_.push(val);
    pthread_mutex_unlock(&queue_mutex_);
    return true;
}

template <typename T>
bool STLSafeQueue<T>::Pop(T& data) {
    pthread_mutex_lock(&queue_mutex_);
    if (queue_.empty()) {
        pthread_mutex_unlock(&queue_mutex_);
        return false;
    }
    data = queue_.front();
    queue_.pop();
    pthread_mutex_unlock(&queue_mutex_);
    return true;
}

template <typename T>
bool STLSafeQueue<T>::empty() {
    return queue_.empty();
}

template <typename T>
bool STLSafeQueue<T>::Front(T& data) {
    pthread_mutex_lock(&queue_mutex_);
    if (queue_.empty()) {
        pthread_mutex_unlock(&queue_mutex_);
        return false;
    }
    data = queue_.front();
    pthread_mutex_unlock(&queue_mutex_);
    return true;
}


/*
template <typename T1, typename T2>
class SafePriorityQueue
{
private:
    std::priority_queue<T1, std::vector<T1>, T2> queue_;
    pthread_mutex_t queue_mutex_;
    int max_queue_len_;
    int size_;

public:
    SafePriorityQueue();
    ~SafePriorityQueue();
    bool Push(T1& val);
    bool Pop(T1& ptr);
};

template <typename T1, typename T2>
SafePriorityQueue<T1,T2>::SafePriorityQueue(int len)
{
    queue_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    max_queue_len_ = len;
}

SafePriorityQueue<T1,T2>::~SafePriorityQueue()
{
}
*/


#endif // _CCB_DISPATCH_QUEUE_H
