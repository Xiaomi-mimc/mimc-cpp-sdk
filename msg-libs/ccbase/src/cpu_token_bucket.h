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
#ifndef _CPU_TOKEN_BUCKET_H
#define _CPU_TOKEN_BUCKET_H

#include <sys/time.h>
#include <stdint.h>
#include "ccbase/common.h"

namespace ccb {

#define MAX_DEC_PERCENT     (-5)
#define MIN_BUCKET_SIZE     (1)
#define CPU_TO_TOKEN_SIZE   (22)
#define MIN_ADJUST_TIME     (2000)
#define HZ ((double)sysconf(_SC_CLK_TCK))
#define TV2US(ptv) ((ptv)->tv_sec * 1000000 + (ptv)->tv_usec)
#define TV2MS(ptv) ((ptv)->tv_sec * 1000 + (ptv)->tv_usec / 1000)

typedef struct time_jiffies_t {
  uint64_t utime;
  uint64_t stime;
  uint64_t cutime;
  uint64_t cstime;

  bool operator>=(const time_jiffies_t& t) {
    return (utime >= t.utime && stime >= t.stime && cutime >= t.cutime && cstime >= t.cstime);
  }
  uint64_t operator-(const time_jiffies_t& t) {
    return (utime - t.utime) + (stime - t.stime) + (cutime - t.cutime) + (cstime - t.cstime);
  }
}time_jiffies;

class CpuTokenBucket
{
public:
  CpuTokenBucket(uint32_t max_tokens_per_sec, uint32_t max_cpu_usage);
  CpuTokenBucket(uint32_t max_tokens_per_sec, uint32_t max_bucket_size, uint32_t max_cpu_usage);
  CpuTokenBucket(uint32_t tokens_per_sec, uint32_t bucket_size, uint32_t init_tokens, 
          uint32_t except_cpu_usage);
  ~CpuTokenBucket() {
      if (0 <= stat_fd_) {
          close(stat_fd_);
      }
  }
  // copyable
  CpuTokenBucket(const CpuTokenBucket&) = default;
  CpuTokenBucket& operator=(const CpuTokenBucket&) = default;

  bool Init(const struct timeval* tv_now = nullptr);
  void Gen(const struct timeval* tv_now = nullptr);
  bool Get(uint32_t need_tokens = 1);
  void Mod(uint32_t max_tokens_per_sec, uint32_t max_bucket_size, uint32_t max_cpu_usage);
  void mod_tokens_bucket(uint32_t max_tokens_per_sec, uint32_t max_bucket_size);
  void mod_tokens(uint32_t max_tokens_per_sec);
  void mod_max_cpu_usage(uint32_t max_cpu_usage);
  void Adjust();
  uint32_t tokens() const;
  uint32_t tokens_per_sec() const;
  uint32_t bucket_size() const;
  uint32_t cpu_usage() const;
  int32_t token_adjust_per() const;
  bool Check(uint32_t need_tokens);
  int Overdraft(uint32_t need_tokens);
  //void print_cpu_static() const;  //for test
private:
  // not movable
  CpuTokenBucket(CpuTokenBucket&&) = delete;
  CpuTokenBucket& operator=(CpuTokenBucket&&) = delete;

  uint32_t max_tokens_per_sec_;
  uint32_t max_bucket_size_;
  uint32_t max_cpu_usage_;
  uint32_t tokens_per_sec_;
  uint32_t bucket_size_;
  float cpu_usage_step_size_;
  uint32_t p_cpu_usage_;
  int32_t token_adjust_per_;
  int64_t token_count_;
  uint64_t last_gen_time_;
  uint64_t last_calc_delta_;
  time_jiffies last_stat_;
  uint64_t last_adjust_time_;
  int32_t cpu_to_token_[CPU_TO_TOKEN_SIZE];
  char stat_file_[32];
  int64_t stat_fd_;
  //uint64_t cpu_statis[CPU_TO_TOKEN_SIZE+1]; //for test
};

inline uint32_t CpuTokenBucket::tokens() const
{
  return (uint32_t)(token_count_ <= 0 ? 0 : token_count_);
}

inline uint32_t CpuTokenBucket::tokens_per_sec() const
{
  return tokens_per_sec_;
}

inline uint32_t CpuTokenBucket::bucket_size() const
{
  return bucket_size_;
}

inline uint32_t CpuTokenBucket::cpu_usage() const
{
  return p_cpu_usage_;
}

inline int32_t CpuTokenBucket::token_adjust_per() const
{
  return token_adjust_per_;
}

#if 0   //for test
inline void CpuTokenBucket::print_cpu_static() const
{
  uint32_t i = 0;
  uint64_t cpu_all = 0;
  for (i = 0; i <= CPU_TO_TOKEN_SIZE; i++) {
      cpu_all += cpu_statis[i];
  }

  for (i = 0; i <= CPU_TO_TOKEN_SIZE; i++) {
      printf("cpu statis [%d] = %ld  %.2lf\n", i, cpu_statis[i], (double)cpu_statis[i]/cpu_all);
  }

}
#endif

} // namespace ccb

#endif
