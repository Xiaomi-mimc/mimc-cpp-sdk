#ifndef MIMC_CPP_SDK_SAFEQUEUE_H
#define MIMC_CPP_SDK_SAFEQUEUE_H

#include <unistd.h>
#include <pthread.h>

const int QUEUE_SIZE = 100;

template <class T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue();
	void push(T new_data);
	void pop(T** result);
	~ThreadSafeQueue();

private:
	T queue[QUEUE_SIZE];
	int head, tail;
	pthread_mutex_t mutex;
};

template<class T>
void ThreadSafeQueue<T>::push(T new_data) {
	while((tail + 1) % QUEUE_SIZE == head) {
		usleep(10000);
	}
	pthread_mutex_lock(&mutex);
	queue[tail] = new_data;
	tail = (tail + 1) % QUEUE_SIZE;
	pthread_mutex_unlock(&mutex);
}

template <class T>
void ThreadSafeQueue<T>::pop(T** result) {
	if (head == tail) {
		*result = NULL;
		return;
	}
	int pos = head;
	head = (head + 1) % QUEUE_SIZE;
	*result = &(queue[pos]);
}

template <class T>
ThreadSafeQueue<T>::ThreadSafeQueue()
	: head(0), tail(0), mutex(PTHREAD_MUTEX_INITIALIZER)
{

}

template <class T>
ThreadSafeQueue<T>::~ThreadSafeQueue() {}

#endif