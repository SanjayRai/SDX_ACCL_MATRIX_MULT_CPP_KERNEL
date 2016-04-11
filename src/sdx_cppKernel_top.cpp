#include <string.h>

#include "sdx_cppKernel_top.h"


void sdx_cppKernel_top(input_data_type *a, input_data_type *b, output_data_type *output)
{
#pragma HLS DATAFLOW
#pragma HLS INTERFACE m_axi port=a offset=slave bundle=gmem depth=8192
#pragma HLS INTERFACE m_axi port=b offset=slave bundle=gmem1 depth=8192
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem depth=8192
#pragma HLS INTERFACE s_axilite port=a bundle=control
#pragma HLS INTERFACE s_axilite port=b bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

  Matrix_data_type bufa[DATA_SIZE];
#pragma HLS ARRAY_PARTITION variable=bufa factor=8 dim=1
  Matrix_data_type bufb[DATA_SIZE];
#pragma HLS ARRAY_PARTITION variable=bufb factor=8 dim=1
  Matrix_data_type bufc[DATA_SIZE];
#pragma HLS ARRAY_PARTITION variable=bufc factor=8 dim=1

  memcpy(bufa, a, sizeof(input_data_type));
  memcpy(bufb, b, sizeof(input_data_type));

  mmult_fn(bufa, bufb, bufc);

  memcpy(output, bufc, sizeof(output_data_type));
  return;
}
