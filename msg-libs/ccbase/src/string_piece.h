// Copyright (c) 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sanjay Ghemawat
//
// A string like object that points into another piece of memory.
// Useful for providing an interface that allows clients to easily
// pass in either a "const char*" or a "string".
//
// Arghh!  I wish C++ literals were automatically of type "string".

// from pcrecpp

#ifndef _CCB_STRING_PIECE_H
#define _CCB_STRING_PIECE_H

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <ostream>
#include <string>

namespace ccb {

class StringPiece {
public:
  // standard STL container boilerplate
  typedef size_t size_type;
  typedef char value_type;
  typedef const char* pointer;
  typedef const char& reference;
  typedef const char& const_reference;
  typedef ptrdiff_t difference_type;
  typedef const char* const_iterator;
  typedef const char* iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;

  static const size_type npos = ~size_type(0);

public:
  // We provide non-explicit singleton constructors so users can pass
  // in a "const char*" or a "std::string" wherever a "StringPiece" is
  // expected.
  StringPiece()
    : ptr_(NULL), length_(0) { }
  StringPiece(const char* str) // NOLINT(runtime/explicit)
    : ptr_(str), length_(strlen(ptr_)) { }
  StringPiece(const unsigned char* str) // NOLINT(runtime/explicit)
    : ptr_(reinterpret_cast<const char*>(str)),
    length_(strlen(ptr_)) { }
  StringPiece(const std::string& str) // NOLINT(runtime/explicit)
    : ptr_(str.data()), length_(str.size()) { }
  StringPiece(const char* offset, size_t len)
    : ptr_(offset), length_(len) { }
  StringPiece(const unsigned char* offset, size_t len)
    : ptr_(reinterpret_cast<const char*>(offset)), length_(len) { }

  // data() may return a pointer to a buffer with embedded NULs, and the
  // returned buffer may or may not be null terminated.  Therefore it is
  // typically a mistake to pass data() to a routine that expects a NUL
  // terminated string.  Use "as_string().c_str()" if you really need to do
  // this.  Or better yet, change your routine so it does not rely on NUL
  // termination.
  const char* data() const { return ptr_; }
  size_t size() const { return length_; }
  size_t length() const { return length_; }
  bool empty() const { return length_ == 0; }

  void clear() {
    ptr_ = NULL;
    length_ = 0;
  }
  void set(const char* buffer, size_t len) {
    ptr_ = buffer;
    length_ = len;
  }
  void set(const char* str) {
    ptr_ = str;
    length_ = str ? strlen(str) : 0;
  }
  void set(const void* buffer, size_t len) {
    ptr_ = reinterpret_cast<const char*>(buffer);
    length_ = len;
  }
  void set(const std::string& str) {
    set(str.data(), str.size());
  }

  void assign(const char* buffer, size_t len) {
    set(buffer, len);
  }
  void assign(const char* str) {
    set(str);
  }
  void assign(const void* buffer, size_t len) {
    set(buffer, len);
  }
  void assign(const std::string& str) {
    set(str);
  }

  char operator[](size_type i) const {
    return ptr_[i];
  }

  void resize(size_t n) {
    length_ = n;
  }

  void remove_prefix(ptrdiff_t n) {
    assert((ptrdiff_t) length_ >= n);
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(ptrdiff_t n) {
    assert((ptrdiff_t) length_ >= n);
    length_ -= n;
  }

  int compare(const StringPiece& x) const;

  std::string as_string() const {
    return std::string(data() ? data() : "", size());
  }

  void copy_to_string(std::string* target) const {
    target->assign(ptr_, length_);
  }

  void append_to_string(std::string* target) const {
    if (!empty())
      target->append(data(), size());
  }

  // Does "this" start with "x"
  bool starts_with(const StringPiece& x) const;

  // Does "this" end with "x"
  bool ends_with(const StringPiece& x) const;

  iterator begin() const { return ptr_; }
  iterator end() const { return ptr_ + length_; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(ptr_ + length_);
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(ptr_);
  }

  size_type max_size() const { return length_; }
  size_type capacity() const { return length_; }

  size_type copy(char* buf, size_type n, size_type pos = 0) const;

  size_type find(const StringPiece& s, size_type pos = 0) const;
  size_type find(char c, size_type pos = 0) const;
  size_type rfind(const StringPiece& s, size_type pos = npos) const;
  size_type rfind(char c, size_type pos = npos) const;

  size_type find_first_of(const StringPiece& s, size_type pos = 0) const;
  size_type find_first_of(char c, size_type pos = 0) const {
    return find(c, pos);
  }
  size_type find_first_not_of(const StringPiece& s, size_type pos = 0) const;
  size_type find_first_not_of(char c, size_type pos = 0) const;
  size_type find_last_of(const StringPiece& s, size_type pos = npos) const;
  size_type find_last_of(char c, size_type pos = npos) const {
    return rfind(c, pos);
  }
  size_type find_last_not_of(const StringPiece& s, size_type pos = npos) const;
  size_type find_last_not_of(char c, size_type pos = npos) const;

  StringPiece substr(size_type pos, size_type n = npos) const;

public:
  template <typename CharType, size_t Size>
  static StringPiece FromArray(const CharType (&array)[Size]) {
    return StringPiece(&array[0], Size);
  }

private:
  const char*   ptr_;
  size_t        length_;
};

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying std::string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template<> struct __type_traits<StringPiece> {
    typedef __true_type    has_trivial_default_constructor;
    typedef __true_type    has_trivial_copy_constructor;
    typedef __true_type    has_trivial_assignment_operator;
    typedef __true_type    has_trivial_destructor;
    typedef __true_type    is_POD_type;
};
#endif

bool operator==(const StringPiece& x, const StringPiece& y);

inline bool operator!=(const StringPiece& x, const StringPiece& y) {
    return !(x == y);
}

inline bool operator<(const StringPiece& x, const StringPiece& y) {
    return x.compare(y) < 0;
}

inline bool operator>(const StringPiece& x, const StringPiece& y) {
    return y < x;
}

inline bool operator<=(const StringPiece& x, const StringPiece& y) {
    return !(x > y);
}

inline bool operator>=(const StringPiece& x, const StringPiece& y) {
    return !(x < y);
}

// allow StringPiece to be logged
inline std::ostream& operator<<(std::ostream& o, const StringPiece& piece) {
    return o.write(piece.data(), piece.length());
}

} // namespace ccb

#endif // _CCB_STRING_PIECE_H
