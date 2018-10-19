#include "fec.h"
#include <string.h>

Fec::Fec(uint16_t o_p, uint16_t r_p) {
    origin_packet_num_ = o_p;
    redundancy_packet_num_ = r_p;
    init_matrix();
}

Fec::~Fec() {
    if (matrix_) {
        delete[] matrix_;
        matrix_ = NULL;
    }

    if (re_matrix_) {
        delete[] re_matrix_;
        re_matrix_ = NULL;
    }
}

void Fec::init_matrix() {
    matrix_ = new int [redundancy_packet_num_ * origin_packet_num_];

    for (int i = 0; i < redundancy_packet_num_; i++) {
        for (int j = 0; j < origin_packet_num_; j++) {
            matrix_[i * origin_packet_num_ + j] = Galois::get_instance()->galois_pow(j + 1, i);
        }
    }

    re_matrix_ = new int [origin_packet_num_ * origin_packet_num_];

    for (int i = 0; i < origin_packet_num_; i++) {
        for (int j = 0; j < origin_packet_num_; j++) {
            re_matrix_[i * origin_packet_num_ + j] = 0;
        }
    }
}

bool Fec::reverse_matrix(int* input) {
    int n = origin_packet_num_;
    for (int i = 0; i < n; ++i) {
        re_matrix_[i * n + i] = 1;
    }

    for (int i = 0; i < n; ++i) {
        for (int j = i; j < n; ++j) {
            if (input[j * n + i] != 0) {
                swap(input, re_matrix_, n, i, j);
                break;
            }
        }
        if (input[i * n + i] == 0) {
            XMDLoggerWrapper::instance()->warn("the input matrix has no reverse matrix.");
            return false;
        }

        for (int k = n - 1; k >= 0; --k) {
            re_matrix_[i * n + k] = Galois::get_instance()->galois_div(re_matrix_[i * n + k], input[i * n + i]);
        }
        for (int k = n - 1; k>= i; --k) {
            input[i * n + k] = Galois::get_instance()->galois_div(input[i * n + k], input[i * n + i]);
        }

        for (int j = 0; j < n; ++j) {
            if (j != i) {
                for (int k = n - 1; k >= 0; --k) {
                    re_matrix_[j * n + k] = Galois::get_instance()->galois_add(re_matrix_[j * n + k], 
                                       Galois::get_instance()->galois_mul(input[j * n + i], re_matrix_[i * n + k]));
                }

                for (int k = n - 1; k >= i; --k) {
                    input[j * n + k] = Galois::get_instance()->galois_add(input[j * n + k], 
                                  Galois::get_instance()->galois_mul(input[j * n + i], input[i * n + k]));
                }
            }
        }
    }


    for (int i = n - 1; i > 0; --i) {
        for (int j = i - 1; j >= 0; --j) {
            for (int k = 0; k < n; ++k) {
                re_matrix_[j * n + k] = Galois::get_instance()->galois_add(re_matrix_[j * n + k], 
                                   Galois::get_instance()->galois_mul(input[j * n + i], re_matrix_[i * n + k]));
            }
            input[j * n + i] = 0;
        }
    }

    return true;
}

void Fec::swap(int* input, int* output, int dimension, int row1, int row2) {
    int tmpInput[dimension];
    int tmpOutput[dimension];

    for (int i = 0; i < dimension; i++) {
        tmpInput[i] = input[row1 * dimension + i];
        tmpOutput[i] = output[row1 * dimension + i];
    }


    for (int i = 0; i < dimension; i++) {
        input[row1 * dimension + i] = input[row2 * dimension + i];
        output[row1 * dimension + i] = output[row2 * dimension + i];

        input[row2 * dimension + i] = tmpInput[i];
        output[row2 * dimension + i] = tmpOutput[i];
    }
}


int Fec::fec_encode(unsigned char* input, int len, unsigned char* output) {
    if (NULL == input) {
        XMDLoggerWrapper::instance()->warn("fec encode input invalid.");
        return -1;
    }
    if (NULL == output) {
        XMDLoggerWrapper::instance()->warn("fec encode output invalid.");
        return -1;
    }

    memset(output, 0, len * origin_packet_num_);
    for (int i = 0; i < redundancy_packet_num_; i++) {
        for (int j = 0; j < origin_packet_num_; j++) {
            for (int k = 0; k < len; k++) {
                output[i * len + k] = Galois::get_instance()->galois_add(output[i * len + k], 
                    Galois::get_instance()->galois_mul(matrix_[i * origin_packet_num_ + j], input[j * len + k]));
            }
        }
    }

    return 0;
}

int Fec::fec_decode(unsigned char* input, int len, unsigned char* output) {
    if (NULL == input) {
        XMDLoggerWrapper::instance()->warn("fec decode input invalid.");
        return -1;
    }
    if (NULL == output) {
        XMDLoggerWrapper::instance()->warn("fec decode output invalid.");
        return -1;
    }

    XMDLoggerWrapper::instance()->warn("do fec decode.");
    for (int i = 0; i < origin_packet_num_; ++i) {
        for (int j = 0; j < origin_packet_num_; ++j) {
            for (int k = 0; k < len; ++k) {
                output[i * len + k] = Galois::get_instance()->galois_add(output[i * len + k], 
                               Galois::get_instance()->galois_mul(re_matrix_[i * origin_packet_num_ + j], input[j * len + k]));
            }
        }
    }

    return 0;
}


