#ifndef SDX_CPPKERNEL_TOP_H_
#define SDX_CPPKERNEL_TOP_H_ 

#define MATRIX_RANK 16
#define DATA_SIZE MATRIX_RANK*MATRIX_RANK

typedef float Matrix_data_type;

struct input_data_type {
    Matrix_data_type data_in[DATA_SIZE];
};

struct output_data_type {
    Matrix_data_type data_out[DATA_SIZE];
};

void mmult_fn (float *a, float *b, float *results);

#ifdef XOCC_CPP_KERNEL 
extern "C" {
#endif
void sdx_cppKernel_top(input_data_type *a, input_data_type *b, output_data_type *a_inv);
#ifdef XOCC_CPP_KERNEL 
}
#endif

#endif
