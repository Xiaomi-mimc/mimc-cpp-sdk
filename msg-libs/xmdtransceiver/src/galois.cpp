#include "galois.h"
#include <math.h>

Galois::Galois() {
    degree = 8;
    gen_poly = 0b100011101;
    order = 0x1U << degree;
    exps = new uint32_t[int(pow(2, degree))];
    logs = new uint32_t[int(pow(2, degree))];

    exps[0] = 1;
    exps[order - 1] = exps[0];
    logs[0] = 0;
    logs[1] = 0;
    for (uint32_t i = 1; i < order-1; ++i) {
        uint32_t h = exps[i-1] >> (degree-1); // get the sign
        uint32_t s = (exps[i-1] << 1);        // a^n = a^(n-1)*X, a shift
        exps[i] = s ^ (gen_poly & (((uint32_t)0) - h)); // a^n mod genpoly: h == 0 ? s : gen_poly^s
        logs[exps[i]] = i;
    }
}

Galois::~Galois() {
    if (exps) {
        delete[] exps;
        exps = NULL;
    }
    if (logs) {
        delete[] logs;
        logs = NULL;

    }
}


Galois* Galois::instance_;
pthread_mutex_t Galois::mutex_ = PTHREAD_MUTEX_INITIALIZER;

Galois* Galois::get_instance() {
    if (!instance_) {
        pthread_mutex_lock(&mutex_);
        if (!instance_) {
            instance_ = new Galois();
        }
        pthread_mutex_unlock(&mutex_);
    }
    return instance_;
}





