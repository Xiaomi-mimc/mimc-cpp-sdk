#ifndef GALOIS_H
#define GALOIS_H

#include <math.h>
#include <mutex>
#include <stdint.h>

class Galois {
private:
    size_t degree;
    uint32_t order;
    uint32_t gen_poly;
    uint32_t * exps;
    uint32_t * logs;

    static Galois* instance_;
	static std::mutex mutex_;

    class DestructLogger {
        public:
            ~DestructLogger()
            {
                if( Galois::instance_ )
                  delete Galois::instance_;
            }
     };

     static DestructLogger destruct;
    

public:
    Galois();
    ~Galois();
    static Galois* get_instance();

    
    inline uint32_t galois_element(uint32_t i) {
        return exps[i % (order - 1)];
    }
    
    inline uint32_t galois_index(uint32_t i) {
        return logs[i];
    }
    
    inline uint32_t galois_add(uint32_t a, uint32_t b) {
        return a ^ b;
    }
    
    inline uint32_t galois_mul(uint32_t a, uint32_t b) {
        if (a == 0 || b == 0) return 0;
        return galois_element(logs[a] + logs[b]);
    }
    
    inline uint32_t galois_div(uint32_t a, uint32_t b) {
        if (a == 0 || b == 0) return a / b;
        return galois_element(order - 1 + logs[a] - logs[b]);
    }

    uint32_t galois_pow(uint32_t a, uint32_t b) {
        if (0 == b) {
            return 1;
        }
        
        uint32_t tmp = a;
        for (int i = 1; i < b; i ++) {
            tmp = galois_mul(tmp, a);
        }

        return tmp;
    }
};



#endif //GALOIS_H
