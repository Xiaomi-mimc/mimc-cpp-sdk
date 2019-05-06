#ifndef XMD_QUEUE_H
#define XMD_QUEUE_H

#include <memory>
#include <queue>
#include <mutex>

template <typename T>
class STLSafeQueue
{

private:
    std::queue<T> queue_;
	std::mutex queue_mutex_;

public:
    STLSafeQueue();
    STLSafeQueue(const STLSafeQueue<T>& queue);
    ~STLSafeQueue();

    bool Push(T& val);
    bool Pop(T& ptr);
    bool empty();
    bool Front(T& ptr);
    size_t Size();
};

template <typename T>
STLSafeQueue<T>::STLSafeQueue() {

}

template <typename T>
STLSafeQueue<T>::STLSafeQueue(const STLSafeQueue<T>& queue) {

}

template <typename T>
STLSafeQueue<T>::~STLSafeQueue() {
}

template <typename T>
bool STLSafeQueue<T>::Push(T& val) {
	queue_mutex_.lock();
    queue_.push(val);
	queue_mutex_.unlock();
    return true;
}

template <typename T>
bool STLSafeQueue<T>::Pop(T& data) {
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
bool STLSafeQueue<T>::empty() {
    return queue_.empty();
}

template <typename T>
bool STLSafeQueue<T>::Front(T& data) {
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
size_t STLSafeQueue<T>::Size() {
    return queue_.size();
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
