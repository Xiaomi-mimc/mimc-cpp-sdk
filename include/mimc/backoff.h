#ifndef MIMC_CPP_SDK_BACKOFF_H
#define MIMC_CPP_SDK_BACKOFF_H

#include <limits>

enum TimeUnit
{
    MILLISECONDS,
    SECONDS,
    MINUTES
};

class Backoff
{
private:
    long base;  // ms
    int factor;
    long cap;
    int maxAttempts;
    long attempts;
    double jitter;  // [0, 1)
    

private:
    static const long DEFAULT_BASE = 100;
    static const int DEFAULT_FACTOR = 2;
    static const long DEFAULT_CAP = (std::numeric_limits<long>::max)();
    static const int DEFAULT_MAX_ATTEMPTS = 16;
    static constexpr double DEFAULT_JITTER = 0.0;

public:
    Backoff();
    Backoff(long base, int factor, long cap, int maxAttempts, double jitter = 0.0);
    void reset();
    void sleep();
    void sleep(long attempt);
    long backoff(long attempt);

    void setBase(TimeUnit timeUnit, long duration);
    void setFactor(int factor);
    void setCap(TimeUnit timeUnit, long duration);
    void setMaxAttempts(int maxAttempts);
    void setJitter(double jitter);
    static long toMillis(TimeUnit timeUnit, long duration);
};

#endif