#ifndef MIMC_CPP_SDK_SAFEQUEUE_H
#define MIMC_CPP_SDK_SAFEQUEUE_H

#include <unistd.h>
#include <pthread.h>
#include <time.h>

const int QUEUE_SIZE = 100;

template <class T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue(unsigned int capacity = QUEUE_SIZE);
	void push(T new_data);
	void pop(long timeout, T** result);
	void pop(T** result);
	bool empty();
	void clear();
	unsigned int size();
	unsigned int capacity();
	~ThreadSafeQueue();

private:
	T* queue;
	unsigned int capacity_;
	int head, tail;
	pthread_mutex_t mutex;
};

template<class T>
void ThreadSafeQueue<T>::push(T new_data) {
	while ((tail + 1) % capacity_ == head) {
		usleep(10000);
	}
	pthread_mutex_lock(&mutex);
	queue[tail] = new_data;
	tail = (tail + 1) % capacity_;
	pthread_mutex_unlock(&mutex);
}

template <class T>
void ThreadSafeQueue<T>::pop(long timeout, T** result) {
	time_t start = time(NULL);
	while (empty()) {
		if (time(NULL) - start >= timeout) {
			*result = NULL;
			return;
		} else {
			usleep(100000);
		}
	}
	int pos = head;
	head = (head + 1) % capacity_;
	*result = &(queue[pos]);
}

template <class T>
void ThreadSafeQueue<T>::pop(T** result) {
	if (empty()) {
		*result = NULL;
		return;
	}
	int pos = head;
	head = (head + 1) % capacity_;
	*result = &(queue[pos]);
}

template <class T>
bool ThreadSafeQueue<T>::empty() {
	if (head == tail) {
		return true;
	}
	return false;
}

template <class T>
void ThreadSafeQueue<T>::clear() {
	head = 0;
	tail = 0;
}

template <class T>
unsigned int ThreadSafeQueue<T>::size() {
	if (tail >= head) {
		return tail - head;
	} else {
		return head - tail;
	}
}

template <class T>
unsigned int ThreadSafeQueue<T>::capacity() {
	return capacity_;
}

template <class T>
ThreadSafeQueue<T>::ThreadSafeQueue(unsigned int capacity)
	: capacity_(capacity), head(0), tail(0), mutex(PTHREAD_MUTEX_INITIALIZER)
{
	queue = new T[capacity_];
}

template <class T>
ThreadSafeQueue<T>::~ThreadSafeQueue() {
	delete[] queue;
	queue = NULL;
}

#endif