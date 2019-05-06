#ifndef SAFEMAP_H
#define SAFEMAP_H

#include <unordered_map>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#else
#include <pthread.h>
#endif

template<typename T1, typename T2>
class STLSafeHashMap
{
private:
	std::unordered_map<T1, T2> map_;

#ifdef _WIN32
	SRWLOCK map_mutex_;
#else
	std::mutex map_mutex_;
#endif // _WIN32

public:
	STLSafeHashMap();
	~STLSafeHashMap();

	bool getMapValue(T1 key, T2& value);
	bool updateMapVaule(T1 key, T2 value);
	bool deleteMapVaule(T1 key);
};

template<typename T1, typename T2>
STLSafeHashMap<T1, T2>::STLSafeHashMap()
{
#ifdef _WIN32
	InitializeSRWLock(&map_mutex_);
#else
	//pthread_rwlock_init(&map_mutex_, NULL);
#endif // _WIN32	
}

template<typename T1, typename T2>
STLSafeHashMap<T1, T2>::~STLSafeHashMap()
{
#ifdef _WIN32
	//Need to do something?
#else
	//pthread_rwlock_destroy(&map_mutex_);
#endif // _WIN32
}


template<typename T1, typename T2>
bool STLSafeHashMap<T1, T2>::getMapValue(T1 key, T2& value)
{
#ifdef _WIN32
	AcquireSRWLockShared(&map_mutex_);
#else
	map_mutex_.lock();
#endif // _WIN32
	
	typename std::unordered_map<T1, T2>::iterator it = map_.find(key);
	if (it == map_.end()) {

#ifdef _WIN32
		ReleaseSRWLockShared(&map_mutex_);
#else
		map_mutex_.unlock();
#endif // _WIN32

		return false;
	}
	else {
		value = it->second;
		
#ifdef _WIN32
		ReleaseSRWLockShared(&map_mutex_);
#else
		map_mutex_.unlock();
#endif // _WIN32
	}

	return true;
}

template<typename T1, typename T2>
bool STLSafeHashMap<T1, T2>::updateMapVaule(T1 key, T2 value)
{
#ifdef _WIN32
	AcquireSRWLockExclusive(&map_mutex_);
#else
	map_mutex_.lock();
#endif // _WIN32
	
	map_[key] = value;

#ifdef _WIN32
	ReleaseSRWLockExclusive(&map_mutex_);
#else
	map_mutex_.unlock();
#endif // _WIN32

	return true;
}


template<typename T1, typename T2>
bool STLSafeHashMap<T1, T2>::deleteMapVaule(T1 key)
{
#ifdef _WIN32
	AcquireSRWLockExclusive(&map_mutex_);
#else
	map_mutex_.lock();
#endif // _WIN32

	typename std::unordered_map<T1, T2>::iterator it = map_.find(key);
	if (it == map_.end()) {

#ifdef _WIN32
		ReleaseSRWLockExclusive(&map_mutex_);
#else
		map_mutex_.unlock();
#endif // _WIN32

		return false;
	}
	else {
		map_.erase(key);
#ifdef _WIN32
		ReleaseSRWLockExclusive(&map_mutex_);
#else
		map_mutex_.unlock();
#endif // _WIN32

		return true;
	}
}



#endif 
