/* Copyright 2015 Xiaomi Inc. All rights reserved.
 * Author: weibin1@xiaomi.com
 * a simple Double Buffering container
 */
#ifndef _CCB_DOUBLE_BUF_H
#define _CCB_DOUBLE_BUF_H

#include <atomic>
#include "ccbase/common.h"

LIB_NAMESPACE_BEGIN

template <class T>
class DoubleBuf
{
public:
  DoubleBuf() : switch_(true) {
    bufs_ = new T[2]{};
  }
  ~DoubleBuf() {
    delete[] bufs_;
  }
  const T& buf() const {
    return bufs_[switch_ ? 0 : 1];
  }
  T& mutable_buf() {
    return bufs_[switch_ ? 1 : 0];
  }
  void Commit() {
    switch_ = !switch_;
  }
  void Update(const T& val) {
    mutable_buf() = val;
    Commit();
  }
private:
  // true : read bufs[0], write bufs[1]
  // false: read bufs[1], write bufs[0]
  std::atomic<bool> switch_;
  T* bufs_;
};

LIB_NAMESPACE_END

#endif
