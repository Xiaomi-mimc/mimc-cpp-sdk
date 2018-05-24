/* Copyright (c) 2012-2015, Bin Wei <bin@vip.qq.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * The name of of its contributors may not be used to endorse or 
 * promote products derived from this software without specific prior 
 * written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "ccbase/cpu_token_bucket.h"

LIB_NAMESPACE_BEGIN

CpuTokenBucket::CpuTokenBucket(uint32_t max_tokens_per_sec, uint32_t max_cpu_usage)
  : CpuTokenBucket(max_tokens_per_sec, max_tokens_per_sec / 5, max_cpu_usage) {}

CpuTokenBucket::CpuTokenBucket(uint32_t max_tokens_per_sec, uint32_t max_bucket_size, uint32_t max_cpu_usage)
  : CpuTokenBucket(max_tokens_per_sec, max_bucket_size, max_bucket_size, max_cpu_usage) {}

CpuTokenBucket::CpuTokenBucket(uint32_t max_tokens_per_sec, uint32_t max_bucket_size,
                                uint32_t init_tokens, uint32_t max_cpu_usage)
{
  max_tokens_per_sec_ = max_tokens_per_sec;
  tokens_per_sec_ = max_tokens_per_sec;
  max_bucket_size_ = max_bucket_size ? max_bucket_size : MIN_BUCKET_SIZE;
  bucket_size_ = max_bucket_size ? max_bucket_size : MIN_BUCKET_SIZE;
  token_count_ = init_tokens;
  max_cpu_usage_ = max_cpu_usage;
  cpu_usage_step_size_ = (float)max_cpu_usage_ / CPU_TO_TOKEN_SIZE;
  cpu_to_token_[0] = 120;
  uint32_t dec_step = 11;
  for (int32_t i = 1; i < CPU_TO_TOKEN_SIZE; i++) {
    if (0 == i % 2) {
        dec_step -= 1;
    }
    cpu_to_token_[i] = cpu_to_token_[i-1] - dec_step;
  }

}

bool CpuTokenBucket::Init(const struct timeval* tv_now) 
{
  struct timeval tv;
  char buffer[256] = {0};

  pid_t pid = getpid();
  sprintf(stat_file_, "/proc/%d/stat", pid);
  stat_fd_ = open(stat_file_, O_RDONLY);
  if (0 > stat_fd_) {
    return false;
  }

  if (!tv_now) {
    tv_now = &tv;
    gettimeofday(&tv, nullptr);
  }

  if (0 >= read(stat_fd_, buffer, sizeof(buffer) - 1)) {
      close(stat_fd_);
      return false;
  }

  uint32_t n = sscanf(buffer,
          "%*s %*s %*s %*s "
          "%*s %*s %*s %*s "
          "%*s %*s %*s %*s %*s "
          "%lu %lu %lu %lu ",
          &last_stat_.utime,
          &last_stat_.stime,
          &last_stat_.cutime,
          &last_stat_.cstime);

  if (4 != n) {
      return false;
  }

  last_gen_time_ = TV2US(tv_now);
  last_adjust_time_ = TV2MS(tv_now);

  return true;
}

void CpuTokenBucket::Mod(uint32_t max_tokens_per_sec, uint32_t max_bucket_size, uint32_t max_cpu_usage)
{
  max_tokens_per_sec_ = max_tokens_per_sec;
  max_bucket_size_ = max_bucket_size ? max_bucket_size : MIN_BUCKET_SIZE;
  max_cpu_usage_ = max_cpu_usage;
  cpu_usage_step_size_ = (float)max_cpu_usage_ / CPU_TO_TOKEN_SIZE;
}

void CpuTokenBucket::mod_tokens_bucket(uint32_t max_tokens_per_sec, uint32_t max_bucket_size) {
    Mod(max_tokens_per_sec, max_bucket_size, max_cpu_usage_);
}

void CpuTokenBucket::mod_tokens(uint32_t max_tokens_per_sec) {
    Mod(max_tokens_per_sec, max_tokens_per_sec / 5, max_cpu_usage_);
}

void CpuTokenBucket::mod_max_cpu_usage(uint32_t max_cpu_usage) {
    Mod(max_tokens_per_sec_, max_bucket_size_, max_cpu_usage);
}

void CpuTokenBucket::Adjust()
{
  struct timeval tv;
  uint64_t ms_now;
  char buffer[256] = {0};
  time_jiffies now_stat;

  if (0 > stat_fd_) {
    stat_fd_ = open(stat_file_, O_RDONLY);
    if (0 > stat_fd_) {
        return;
    }
  }

  gettimeofday(&tv, nullptr);
  ms_now = TV2MS(&tv);

  if (0 != lseek(stat_fd_, 0, SEEK_SET)) {
      close(stat_fd_);
      stat_fd_ = open(stat_file_, O_RDONLY);
      if (0 > stat_fd_) {
        return;
      }
      return;
  }

  uint32_t n = read(stat_fd_, buffer, sizeof(buffer) - 1);
  if (0 > n) {
      close(stat_fd_);
      stat_fd_ = open(stat_file_, O_RDONLY);
      if (0 > stat_fd_) {
        return;
      }
      return;
  } else if (0 == n) {
      return;
  }

  n = sscanf(buffer,
          "%*s %*s "
          "%*s %*s %*s %*s "
          "%*s %*s %*s %*s %*s "
          "%lu %lu %lu %lu ",
          &now_stat.utime,
          &now_stat.stime,
          &now_stat.cutime,
          &now_stat.cstime);

  if (4 != n) {
      return;
  }

  if ((ms_now < last_adjust_time_) || (ms_now - last_adjust_time_ > 10000) || !(now_stat >= last_stat_)) {
      last_adjust_time_ = ms_now;
      last_stat_ = now_stat;
      return;
  }

  uint64_t p_use_time = (now_stat - last_stat_) * 1000 / HZ;
  uint32_t p_cpu_usage = (p_use_time * 100) / (ms_now - last_adjust_time_);
  p_cpu_usage_ = p_cpu_usage;
  int32_t i = p_cpu_usage / cpu_usage_step_size_;
  int32_t token_adjust_percent = (i >= CPU_TO_TOKEN_SIZE ? MAX_DEC_PERCENT : cpu_to_token_[i]);

  token_adjust_per_ = token_adjust_percent;
  tokens_per_sec_ = tokens_per_sec_ * ((float)(100 + token_adjust_percent) / 100);
  bucket_size_ = bucket_size_ * ((float)(100 + token_adjust_percent) / 100);
  if (tokens_per_sec_ > max_tokens_per_sec_) {
      tokens_per_sec_ = max_tokens_per_sec_;
  } else if (tokens_per_sec_ < MIN_BUCKET_SIZE) {
      tokens_per_sec_ = MIN_BUCKET_SIZE;
  }

  if ((bucket_size_ > max_bucket_size_) || (bucket_size_ > tokens_per_sec_)) {
      bucket_size_ = (max_bucket_size_ < tokens_per_sec_ ? max_bucket_size_ : tokens_per_sec_);
  } else if (bucket_size_ < MIN_BUCKET_SIZE) {
      bucket_size_ = MIN_BUCKET_SIZE;
  }

  last_adjust_time_ = ms_now;
  last_stat_ = now_stat;

  return;
}

void CpuTokenBucket::Gen(const struct timeval* tv_now)
{
  struct timeval tv;
  uint64_t us_now, us_past, ms_now;
  uint64_t new_tokens, calc_delta;
  int64_t new_token_count;

  if (tv_now == nullptr) {
    tv_now = &tv;
    gettimeofday(&tv, nullptr);
  }

  us_now = TV2US(tv_now);
  if (us_now < last_gen_time_) {
    last_gen_time_ = us_now;
    return;
  }

  ms_now = TV2MS(tv_now);
  if ((ms_now - last_adjust_time_ >= MIN_ADJUST_TIME) || (ms_now < last_adjust_time_)) {
      Adjust();
  }

  us_past = us_now - last_gen_time_;
  new_tokens = (((uint64_t)tokens_per_sec_ * us_past + last_calc_delta_)
                 / 1000000);
  calc_delta = (((uint64_t)tokens_per_sec_ * us_past + last_calc_delta_)
                 % 1000000);

  last_gen_time_ = us_now;
  last_calc_delta_ = calc_delta;
  new_token_count = token_count_ + new_tokens;
  if (new_token_count < token_count_) {
    token_count_ = bucket_size_;
    return;
  }
  if (new_token_count > bucket_size_) {
    new_token_count = bucket_size_;
  }
  token_count_ = new_token_count;

  return;
}

bool CpuTokenBucket::Check(uint32_t need_tokens)
{
  if (token_count_ < (int64_t)need_tokens) {
    return false;
  }
  return true;
}

bool CpuTokenBucket::Get(uint32_t need_tokens)
{
  if (token_count_ < (int64_t)need_tokens) {
    return false;
  }
  token_count_ -= need_tokens;
  return true;
}

int CpuTokenBucket::Overdraft(uint32_t need_tokens)
{
  token_count_ -= need_tokens;
  return (token_count_ < 0 ? -token_count_ : 0);
}

LIB_NAMESPACE_END

