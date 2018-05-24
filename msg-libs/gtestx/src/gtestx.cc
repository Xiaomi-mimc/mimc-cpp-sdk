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
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <thread>
#include "ccbase/token_bucket.h"
#include "gtestx.h"

DEFINE_uint64(hz, 0, "loops per second");
DEFINE_uint64(time, 1500, "testing time long (ms)");

namespace gtestx {

void Perf::Run()
{
	struct timeval tv_begin, tv_end, tv_use;
	unsigned long long count = 0;

	stop_ = false;
	hz_ = FLAGS_hz;
	time_ = FLAGS_time;

	ccb::TokenBucket tb(hz_, hz_/5, hz_/500);
	std::thread alarm([this] {
		int unit = 10;
		for (int ms = time_; !stop_ && ms > 0; ms -= unit)
			std::this_thread::sleep_for(std::chrono::milliseconds(ms > unit ? unit : ms));
		stop_ = true;
	});

	gettimeofday(&tv_begin, NULL);

	while (!stop_) {
		if (!hz_ || tb.Get(1)) {
			TestCode();
			count++;
		} else {
			usleep(1000);
			tb.Gen();
		}
	}

	gettimeofday(&tv_end, NULL);
	timersub(&tv_end, &tv_begin, &tv_use);

	setlocale(LC_ALL, "");
	printf("      count: %'llu\n", count);
	printf("       time: %'d.%06d s\n", (int)tv_use.tv_sec, (int)tv_use.tv_usec);
	printf("         HZ: %'f\n", count/(tv_use.tv_sec+0.000001*tv_use.tv_usec));
	printf("       1/HZ: %'.9f s\n", (tv_use.tv_sec+0.000001*tv_use.tv_usec)/count);

	alarm.join();
}

void Perf::Abort()
{
	stop_ = true;
}

}

PERF_TEST_MAIN

