#ifndef FEC_H
#define FEC_H

#include "galois.h"
#include "XMDLoggerWrapper.h"


class Fec {
private:
    uint16_t origin_packet_num_;
    uint16_t redundancy_packet_num_;
    int* matrix_;
    int* re_matrix_;

public:
    Fec(uint16_t o_p, uint16_t r_p);
    ~Fec();

    uint16_t get_origin_packet_num() { return origin_packet_num_; }

    uint16_t get_redundancy_packet_num() { return redundancy_packet_num_; }


    int fec_encode(unsigned char* input, int len, unsigned char* output);
    int fec_decode(unsigned char* input, int len, unsigned char* output);
    bool reverse_matrix(int* input);
    int* get_matrix() { return matrix_; }


private:
    void init_matrix();
    void swap(int* input, int* output, int dimension, int row1, int row2);
};



#endif //FEC_H
