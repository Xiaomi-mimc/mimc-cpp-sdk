#ifndef TIMER_H
#define TIMER_H
#include "xmd_thread.h"
#include <list>
#include <cstdarg>


typedef void (*Func)(void* args,...);

struct TimerInfo {
    uint64_t ts;
    uint32_t id;
    va_list args;
    void (*callback)(void* args,...);
};


class Timer : public XMDThread {
public:
    virtual void* process();
    void setTimer(int ts, void (*callback)(void* args,...), void* args, ...);
    void stopTimer(uint32_t id);
private:
    std::list<TimerInfo> timerList_;
};



#endif //TIMER_H
