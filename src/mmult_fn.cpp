void mmult_fn (float *a, float *b, float *results) {
#pragma HLS DATAFLOW

  const int rank = 16;
  int running = 0;

  for (unsigned int c=0;c<rank;c++){
    for (unsigned int r=0;r<rank;r++){
#pragma HLS PIPELINE
      running=0;
      for (int index=0; index<rank; index++) {
#pragma HLS PIPELINE
//#pragma HLS pipeline
        int aIndex = r*rank + index;
        int bIndex = index*rank + c;
        running += a[aIndex] * b[bIndex];
      }
      results[r*rank + c] = running;
    }
  }

  return;

}
