#ifndef SAFEMAP_H
#define SAFEMAP_H

#include <pthread.h>
#include <unordered_map>

template<typename T1, typename T2>
class STLSafeHashMap
{
private:
    std::unordered_map<T1,T2> map_;
    pthread_rwlock_t map_mutex_;

public:
    STLSafeHashMap();
    ~STLSafeHashMap();
    
    bool getMapValue(T1 key, T2& value);
    bool updateMapVaule(T1 key, T2 value);
    bool deleteMapVaule(T1 key);
};

template<typename T1, typename T2>
STLSafeHashMap<T1,T2>::STLSafeHashMap()
{
    pthread_rwlock_init(&map_mutex_, NULL);
}

template<typename T1, typename T2>
STLSafeHashMap<T1,T2>::~STLSafeHashMap()
{
    pthread_rwlock_destroy(&map_mutex_);
}


template<typename T1, typename T2>
bool STLSafeHashMap<T1,T2>::getMapValue(T1 key, T2& value)
{
    pthread_rwlock_rdlock(&map_mutex_);
    typename std::unordered_map<T1,T2>::iterator it = map_.find(key);
    if (it == map_.end()) {
        pthread_rwlock_unlock(&map_mutex_);
        return false;
    } else {
        value = it->second;
        pthread_rwlock_unlock(&map_mutex_);
    }

    return true;
}

template<typename T1, typename T2>
bool STLSafeHashMap<T1,T2>::updateMapVaule(T1 key, T2 value) 
{
    pthread_rwlock_wrlock(&map_mutex_);
    map_[key] = value;
    pthread_rwlock_unlock(&map_mutex_);
    return true;
}


template<typename T1, typename T2>
bool STLSafeHashMap<T1,T2>::deleteMapVaule(T1 key)
{
    pthread_rwlock_wrlock(&map_mutex_);
    typename std::unordered_map<T1,T2>::iterator it = map_.find(key);
    if (it == map_.end()) {
        pthread_rwlock_unlock(&map_mutex_);
        return false;
    } else {
        map_.erase(key);
        pthread_rwlock_unlock(&map_mutex_);
        return true;
    }
}






template<typename T1, typename T2>
class STLHashMap
{
private:
    std::unordered_map<T1,T2> map_;

public:
    STLHashMap();
    ~STLHashMap();
    
    bool getMapValue(T1 key, T2& value);
    bool updateMapVaule(T1 key, T2 value);
    bool deleteMapVaule(T1 key);
    size_t size();
};

template<typename T1, typename T2>
STLHashMap<T1,T2>::STLHashMap()
{
}

template<typename T1, typename T2>
STLHashMap<T1,T2>::~STLHashMap()
{
}


template<typename T1, typename T2>
bool STLHashMap<T1,T2>::getMapValue(T1 key, T2& value)
{
    typename std::unordered_map<T1,T2>::iterator it = map_.find(key);
    if (it == map_.end()) {
        return false;
    } else {
        value = it->second;
    }

    return true;
}

template<typename T1, typename T2>
bool STLHashMap<T1,T2>::updateMapVaule(T1 key, T2 value) 
{
    map_[key] = value;
    return true;
}


template<typename T1, typename T2>
bool STLHashMap<T1,T2>::deleteMapVaule(T1 key)
{
    typename std::unordered_map<T1,T2>::iterator it = map_.find(key);
    if (it == map_.end()) {
        return false;
    } else {
        map_.erase(key);
        return true;
    }
}


template<typename T1, typename T2>
size_t STLHashMap<T1,T2>::size() {
    return map_.size();
}

#endif 
