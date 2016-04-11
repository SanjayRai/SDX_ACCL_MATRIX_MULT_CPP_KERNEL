#include <stdio.h>
#include <math.h>

#include "sdaccel_utils.h" 
#include "sdx_cppKernel_top.h"

////////////////////////////////////////////////////////////////////////////////

// Use a static matrix for simplicity
//


////////////////////////////////////////////////////////////////////////////////

#ifndef NUMBER_OF_COMPUTE_UNITS
#define NUMBER_OF_COMPUTE_UNITS 1 
#endif

#define MAX_ITERATION 20 
#define GLOBAL_SIZE MAX_ITERATION*NUMBER_OF_COMPUTE_UNITS

double getCPUTime();

int main(int argc, char** argv)
{

  double startTime;
  double endTime;
  double ElapsedTime;

  unsigned int correct;               // number of correct results returned

  input_data_type *a_ptr;                   // original data set given to device
  input_data_type *a_ptr_head;                   // original data set given to device
  input_data_type *b_ptr;                   // original data set given to device
  input_data_type *b_ptr_head;                   // original data set given to device
  output_data_type *results_ptr;             // results returned from device
  output_data_type *results_ptr_head;             // results returned from device
  output_data_type *sw_results_ptr;          // results returned from device
  output_data_type *sw_results_ptr_head;          // results returned from device
  int in_args_size[10];               // Array of input Argument sizes
  int out_args_size[10];              // Array of output Argument sizes
  int num_input_args;
  int num_output_args;
  int init_success;
  int run_success;
  int sdaccel_clean_success;


  num_input_args = 2;
  num_output_args = 1;

  a_ptr = new input_data_type[GLOBAL_SIZE];
  a_ptr_head = a_ptr;
  b_ptr = new input_data_type[GLOBAL_SIZE];
  b_ptr_head = b_ptr;
  results_ptr = new output_data_type[GLOBAL_SIZE];
  results_ptr_head = results_ptr;
  sw_results_ptr = new output_data_type[GLOBAL_SIZE];
  sw_results_ptr_head = sw_results_ptr;

  // Fill our data sets with pattern
  //
  int i = 0;
  int cu = 0;
for (int num_itn = 0 ; num_itn < MAX_ITERATION; num_itn++) {
  for (cu = 0; cu <  NUMBER_OF_COMPUTE_UNITS;cu++) { 
      for(i = 0; i < DATA_SIZE; i++) {
        a_ptr->data_in[i] = (Matrix_data_type)(i+cu+num_itn);
        b_ptr->data_in[i] = (Matrix_data_type)(i+cu+num_itn);
        results_ptr->data_out[i] = (Matrix_data_type)0;
      }
  a_ptr++;
  b_ptr++;
  results_ptr++;
  }
}

    a_ptr = a_ptr_head;
    b_ptr = b_ptr_head;
    results_ptr = results_ptr_head;



  // Connect to first platform
  //

#ifdef GPP_ONLY_FLOW  
    startTime = getCPUTime();
    for (int num_itn = 0 ; num_itn < MAX_ITERATION; num_itn++) {
        for (cu = 0; cu <  NUMBER_OF_COMPUTE_UNITS;cu++) { 
            sdx_cppKernel_top(a_ptr, b_ptr, results_ptr);
            a_ptr++;
            b_ptr++;
            results_ptr++;
        }
    }

    endTime = getCPUTime();

    a_ptr = a_ptr_head;
    b_ptr = b_ptr_head;
    results_ptr = results_ptr_head;


#else 
    fpga_hw_accel<input_data_type,output_data_type, NUMBER_OF_COMPUTE_UNITS, MAX_ITERATION> Alpha_Data_ku060_card;

        if (argc != 2){
        printf("%s <inputfile>\n", argv[0]);
        return EXIT_FAILURE;
        }


      in_args_size[0] = sizeof(input_data_type)*(MAX_ITERATION);
      in_args_size[1] = sizeof(input_data_type)*(MAX_ITERATION);
      out_args_size[0] = sizeof(output_data_type)*(MAX_ITERATION);


        init_success = Alpha_Data_ku060_card.initalize_fpga_hw_accel(argv[1], "sdx_cppKernel_top", in_args_size, out_args_size, num_input_args, num_output_args); 
        if (init_success ) {

            a_ptr = a_ptr_head;
            b_ptr = b_ptr_head;
            results_ptr = results_ptr_head;

            startTime = getCPUTime();

            run_success =  Alpha_Data_ku060_card.run_fpga_accel (a_ptr, b_ptr, results_ptr);

            endTime = getCPUTime();
            if (!run_success ) {
                printf("Error: SdAccel CPP Kernel execution Failed !!\n");
            }
        } else {
            printf("Error: SdAccel provisioning Failed !!\n");
        }
        sdaccel_clean_success = Alpha_Data_ku060_card.clean_fpga_hw_accel();
        if (!sdaccel_clean_success) {
            printf("Error: SdAccel resource cleanip Failed !!\n");
        }

#endif


    a_ptr = a_ptr_head;
    b_ptr = b_ptr_head;
    results_ptr = results_ptr_head;
    sw_results_ptr = sw_results_ptr_head;
  for (cu = 0; cu <  NUMBER_OF_COMPUTE_UNITS;cu++) { 

      // Validate our results
      //
      correct = 0;
      for(i = 0; i < DATA_SIZE; i++)
      {
        int row = i/MATRIX_RANK;
        int col = i%MATRIX_RANK;
        int running = 0;
        int index;
        for (index=0;index<MATRIX_RANK;index++) {
          int aIndex = row*MATRIX_RANK + index;
          int bIndex = col + index*MATRIX_RANK;
          running += a_ptr->data_in[aIndex] * b_ptr->data_in[bIndex];
        }
        sw_results_ptr->data_out[i] = running;
        if(results_ptr->data_out[i] == sw_results_ptr->data_out[i])
          correct++;
      }
      a_ptr++;
      b_ptr++;
      results_ptr++;
      sw_results_ptr++;

      printf("Test_vector %d : Computed '%d/%d' correct values!\n", cu, correct, DATA_SIZE);
  }



    a_ptr = a_ptr_head;
    b_ptr = b_ptr_head;
    results_ptr = results_ptr_head;
    sw_results_ptr = sw_results_ptr_head;

  if(correct == DATA_SIZE){
  for (cu = 0; cu <  NUMBER_OF_COMPUTE_UNITS;cu++) { 
    printf("Test passed!\n");
      printf("A[%d]\n", cu);
      for (i=0;i<DATA_SIZE;i++) {
        printf("%f ",(float)a_ptr->data_in[i]);
        if (((i+1) % 16) == 0)
          printf("\n");
      }
      printf("B[%d]\n", cu);
      for (i=0;i<DATA_SIZE;i++) {
        printf("%f ",(float)b_ptr->data_in[i]);
        if (((i+1) % 16) == 0)
          printf("\n");
      }
      printf("res[%d]\n", cu);
      for (i=0;i<DATA_SIZE;i++) {
        printf("%f ",(float)results_ptr->data_out[i]);
        if (((i+1) % 16) == 0)
          printf("\n");
      }
      printf("Software result[%d]\n", cu);
      for (i=0;i<DATA_SIZE;i++) {
        printf("%f ",(float)sw_results_ptr->data_out[i]);
        if (((i+1) % 16) == 0)
          printf("\n");
      }
      a_ptr++;
      b_ptr++;
      results_ptr++;
      sw_results_ptr++;
    }
  }
  else{
    printf("Test failed\n");
  }
    free(a_ptr_head);
    free(b_ptr_head);
    free(results_ptr_head);
    free(sw_results_ptr_head);
    ElapsedTime = (endTime - startTime);
    printf (" Elapsed Time for algorithm = %1f sec\n", ElapsedTime);
    return 0;
}
