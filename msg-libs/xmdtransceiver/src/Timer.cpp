#include "Timer.h"
#include "common.h"


void Timer::setTimer(int ts, void (*callback)(void* args,...), void* args, ...) {
    TimerInfo tInfo;
    tInfo.ts= current_ms() + ts;
    tInfo.callback = callback;
    va_start(tInfo.args, args);

    
}


void Timer::stopTimer(uint32_t id) {
}



