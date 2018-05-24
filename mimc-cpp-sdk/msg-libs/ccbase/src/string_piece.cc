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
// Author: wilsonh@google.com (Wilson Hsieh)
//

#include "ccbase/string_piece.h"
#include <limits.h>
#include <string.h>
#include <algorithm>

namespace ccb {

// defined in implementation only for shorter typing
typedef StringPiece::size_type size_type;
const size_type StringPiece::npos;

static inline bool MemoryEqual(const void* a1, const void* a2, size_t size)
{
  return memcmp(a1, a2, size) == 0;
}

bool operator==(const StringPiece& x, const StringPiece& y)
{
  return ((x.size() == y.size()) && MemoryEqual(x.data(), y.data(), x.size()));
}

int StringPiece::compare(const StringPiece& x) const {
  int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
  if (r != 0)
    return r;
  if (length_ < x.length_)
    return -1;
  else if (length_ > x.length_)
    return 1;
  return 0;
}

#if 0
int StringPiece::ignore_case_compare(const StringPiece& x) const {
  int r = memcasecmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
  if (r != 0)
    return r;
  if (length_ < x.length_)
    return -1;
  else if (length_ > x.length_)
    return 1;
  return 0;
}

bool StringPiece::ignore_case_equal(const StringPiece& other) const {
  return size() == other.size() && memcasecmp(data(), other.data(), size()) == 0;
}
#endif

// Does "this" start with "x"
bool StringPiece::starts_with(const StringPiece& x) const {
  return ((length_ >= x.length_) && MemoryEqual(ptr_, x.ptr_, x.length_));
}

// Does "this" end with "x"
bool StringPiece::ends_with(const StringPiece& x) const {
  return ((length_ >= x.length_) &&
      (MemoryEqual(ptr_ + length_ - x.length_, x.ptr_, x.length_)));
}

size_type StringPiece::copy(char* buf, size_type n, size_type pos) const {
  size_type ret = std::min(length_ - pos, n);
  memcpy(buf, ptr_ + pos, ret);
  return ret;
}

size_type StringPiece::find(const StringPiece& s, size_type pos) const {
  if (pos > length_)
    return npos;

  const char* result = std::search(ptr_ + pos, ptr_ + length_,
      s.ptr_, s.ptr_ + s.length_);
  const size_type xpos = result - ptr_;
  return xpos + s.length_ <= length_ ? xpos : npos;
}

size_type StringPiece::find(char c, size_type pos) const {
  if (pos >= length_)
    return npos;

  const void* result = memchr(ptr_ + pos, c, length_ - pos);
  if (result)
    return reinterpret_cast<const char*>(result) - ptr_;
  return npos;
}

size_type StringPiece::rfind(const StringPiece& s, size_type pos) const {
  if (length_ < s.length_)
    return npos;

  if (s.empty())
    return std::min(length_, pos);

  const char* last = ptr_ + std::min(length_ - s.length_, pos) + s.length_;
  const char* result = std::find_end(ptr_, last, s.ptr_, s.ptr_ + s.length_);
  return result != last ? static_cast<size_t>(result - ptr_) : npos;
}

size_type StringPiece::rfind(char c, size_type pos) const {
  if (length_ == 0)
    return npos;

  for (size_type i = std::min(pos, length_ - 1); ; --i) {
    if (ptr_[i] == c)
      return i;
    if (i == 0)
      break;
  }
  return npos;
}

// For each character in characters_wanted, sets the index corresponding
// to the ASCII code of that character to 1 in table.  This is used by
// the m_find.*_of methods below to tell whether or not a character is in
// the lookup table in constant time.
// The argument `table' must be an array that is large enough to hold all
// the possible values of an unsigned char.  Thus it should be be declared
// as follows:
//   bool table[UCHAR_MAX + 1]
static inline void BuildLookupTable(const StringPiece& characters_wanted,
    bool* table) {
  const size_type length = characters_wanted.length();
  const char* const data = characters_wanted.data();
  for (size_type i = 0; i < length; ++i) {
    table[static_cast<unsigned char>(data[i])] = true;
  }
}

size_type StringPiece::find_first_of(const StringPiece& s,
    size_type pos) const {
  if (length_ == 0 || s.length_ == 0)
    return npos;

  // Avoid the cost of BuildLookupTable() for a single-character search.
  if (s.length_ == 1)
    return find_first_of(s.ptr_[0], pos);

  bool lookup[UCHAR_MAX + 1] = { false };
  BuildLookupTable(s, lookup);
  for (size_type i = pos; i < length_; ++i) {
    if (lookup[static_cast<unsigned char>(ptr_[i])]) {
      return i;
    }
  }
  return npos;
}

size_type StringPiece::find_first_not_of(const StringPiece& s,
    size_type pos) const {
  if (length_ == 0)
    return npos;

  if (s.length_ == 0)
    return 0;

  // Avoid the cost of BuildLookupTable() for a single-character search.
  if (s.length_ == 1)
    return find_first_not_of(s.ptr_[0], pos);

  bool lookup[UCHAR_MAX + 1] = { false };
  BuildLookupTable(s, lookup);
  for (size_type i = pos; i < length_; ++i) {
    if (!lookup[static_cast<unsigned char>(ptr_[i])]) {
      return i;
    }
  }
  return npos;
}

size_type StringPiece::find_first_not_of(char c, size_type pos) const {
  if (length_ == 0)
    return npos;

  for (; pos < length_; ++pos) {
    if (ptr_[pos] != c) {
      return pos;
    }
  }
  return npos;
}

size_type StringPiece::find_last_of(const StringPiece& s, size_type pos) const {
  if (length_ == 0 || s.length_ == 0)
    return npos;

  // Avoid the cost of BuildLookupTable() for a single-character search.
  if (s.length_ == 1)
    return find_last_of(s.ptr_[0], pos);

  bool lookup[UCHAR_MAX + 1] = { false };
  BuildLookupTable(s, lookup);
  for (size_type i = std::min(pos, length_ - 1); ; --i) {
    if (lookup[static_cast<unsigned char>(ptr_[i])])
      return i;
    if (i == 0)
      break;
  }
  return npos;
}

size_type StringPiece::find_last_not_of(const StringPiece& s,
    size_type pos) const {
  if (length_ == 0)
    return npos;

  size_type i = std::min(pos, length_ - 1);
  if (s.length_ == 0)
    return i;

  // Avoid the cost of BuildLookupTable() for a single-character search.
  if (s.length_ == 1)
    return find_last_not_of(s.ptr_[0], pos);

  bool lookup[UCHAR_MAX + 1] = { false };
  BuildLookupTable(s, lookup);
  for (; ; --i) {
    if (!lookup[static_cast<unsigned char>(ptr_[i])])
      return i;
    if (i == 0)
      break;
  }
  return npos;
}

size_type StringPiece::find_last_not_of(char c, size_type pos) const {
  if (length_ == 0)
    return npos;

  for (size_type i = std::min(pos, length_ - 1); ; --i) {
    if (ptr_[i] != c)
      return i;
    if (i == 0)
      break;
  }
  return npos;
}

StringPiece StringPiece::substr(size_type pos, size_type n) const {
  if (pos > length_)
    pos = length_;
  if (n > length_ - pos)
    n = length_ - pos;
  return StringPiece(ptr_ + pos, n);
}

} // namespace ccb
