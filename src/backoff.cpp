#define NOMINMAX

#include "mimc/backoff.h"
#include <algorithm>
#include <thread>
#include <cmath>
#include "XMDTransceiver.h"
#include <chrono>

Backoff::Backoff() : base(DEFAULT_BASE),
                     factor(DEFAULT_FACTOR),
                     cap(DEFAULT_CAP),
                     maxAttempts(DEFAULT_MAX_ATTEMPTS),
                     attempts(0),
                     jitter(DEFAULT_JITTER) {
    srand(time(NULL));
}

Backoff::Backoff(long base, int factor, long cap, int maxAttempts, double jitter)
{
    this->base = base;
    this->factor = factor;
    this->cap = cap;
    this->maxAttempts = maxAttempts;
    this->jitter = jitter;
    srand(time(NULL));
}

void Backoff::reset() {
    if (attempts != 0) {
        attempts = 0;
    }
}

void Backoff::sleep() {
    if (attempts < maxAttempts) {
        sleep(attempts++);
    } else {
        sleep(maxAttempts);
    }
}

void Backoff::sleep(long attempt) {
    std::this_thread::sleep_for(std::chrono::milliseconds(backoff(attempt)));
}

long Backoff::backoff(long attempt) {
    long duration = base * (long)pow(factor, attempt);
    if (jitter != 0.0) {
        // [0.000000, 1.000000)
        double random = (rand() % 1000000) * 1.0 / 1000000;
        long deviation = (long)std::floor(random * duration * jitter);
        if ((((int)std::floor(random * 10)) & 1) == 0) {
            duration -= deviation;
        } else {
            duration += deviation;
        }

        if (duration < 0)
        {
            duration = std::numeric_limits<long>::max();
        }

        duration = std::min(std::max(duration, base), cap);
        if (duration == cap)
        {
            deviation = (int)std::floor(random * duration * jitter);
            duration -= deviation;
        }
        
        return std::max(duration, base);
    } else {
        if (duration < 0)
        {
            duration = std::numeric_limits<long>::max();
        }

        return std::min(std::max(duration, base), cap);
    }
}

void Backoff::setBase(TimeUnit timeUnit, long duration) {
    base = Backoff::toMillis(timeUnit, duration);
}

void Backoff::setFactor(int factor) {
    this->factor = factor;
}

void Backoff::setCap(TimeUnit timeUnit, long duration) {
    cap = Backoff::toMillis(timeUnit, duration);
}

void Backoff::setMaxAttempts(int maxAttempts) {
    this->maxAttempts = maxAttempts;
}

void Backoff::setJitter(double jitter)
{
    this->jitter = jitter;
}

long Backoff::toMillis(TimeUnit timeUnit, long duration)
{
    switch (timeUnit)
    {
    case MILLISECONDS:
        return duration;
    case SECONDS:
        return duration * 1000;
    case MINUTES:
        return duration * 60 * 1000;
    }
}