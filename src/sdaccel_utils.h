#ifndef SDACCEL_UTILITIES_H_
#define SDACCEL_UTILITIES_H_
//

#include <CL/opencl.h>

template <class input_data_type, class output_data_type, int NUM_OF_COMPUTE_UNITS, int MAX_ITERATION>
class fpga_hw_accel {


    private:

        cl_platform_id platform_id;         // platform id
        cl_device_id device_id;             // compute device id
        cl_context context;                 // compute context
        cl_command_queue commands;          // compute command queue
        cl_program program;                 // compute program
        cl_kernel kernel[NUM_OF_COMPUTE_UNITS];               // compute kernel
        cl_mem inputs[NUM_OF_COMPUTE_UNITS][10];                     // device memory used for the input array
        cl_mem outputs[NUM_OF_COMPUTE_UNITS][10];                      // device memory used for the output array
        int num_input_args;
        int num_output_args;
        int in_args_size_vec[10];
        int out_args_size_vec[10];


    int load_file_to_memory(const char *filename, char **result)
    {
      unsigned int size = 0;
      FILE *f = fopen(filename, "rb");
      if (f == NULL)
      {
        *result = NULL;
        return -1; // -1 means file opening fail
      }
      fseek(f, 0, SEEK_END);
      size = ftell(f);
      fseek(f, 0, SEEK_SET);
      *result = (char *)malloc(size+1);
      if (size != fread(*result, sizeof(char), size, f))
      {
        free(*result);
        return -2; // -2 means file reading fail
      }
      fclose(f);
      (*result)[size] = 0;
      return size;
    }



        

    // For C/C++ Kernels Partition the "Global" Dataset to multiple "Local" dataset and call clSetKernelArg and clEnqueueTask multiple times.

    public:

    fpga_hw_accel(void):num_input_args(0),num_output_args(0){}

    int initalize_fpga_hw_accel(const char *filename, const char *kernel_name, int in_args_size[10], int out_args_size[10], int num_in_args, int num_out_args) {
        int i;
        int err;
        int compute_unit;
        bool SUCESSFUL_EXIT_CODE;
        int status;
        char cl_platform_vendor[1001];
        char cl_platform_name[1001];
        size_t len;
        char buffer[2048];


        unsigned char *kernelbinary;

        num_input_args = num_in_args;
        num_output_args = num_out_args;

        for (i = 0 ; i < num_input_args; i++ ) {
            in_args_size_vec[i] = in_args_size[i];
        }

        for (i = 0 ; i < num_output_args; i++ ) {
            out_args_size_vec[i] = out_args_size[i];
        }

        SUCESSFUL_EXIT_CODE = 1;
        err = clGetPlatformIDs(1,&platform_id,NULL);
        if (err != CL_SUCCESS) {
            printf("Error: Failed to find an OpenCL platform!\n");
            printf("Test failed\n");
            return 0;
        } else {
            err = clGetPlatformInfo(platform_id,CL_PLATFORM_VENDOR,1000,(void *)cl_platform_vendor,NULL);
            if (err != CL_SUCCESS) {
                printf("Error: clGetPlatformInfo(CL_PLATFORM_VENDOR) failed!\n");
                printf("Test failed\n");
                return 0;
            } else {
                printf("CL_PLATFORM_VENDOR %s\n",cl_platform_vendor);
                err = clGetPlatformInfo(platform_id,CL_PLATFORM_NAME,1000,(void *)cl_platform_name,NULL);
                if (err != CL_SUCCESS) {
                    printf("Error: clGetPlatformInfo(CL_PLATFORM_NAME) failed!\n");
                    printf("Test failed\n");
                    return 0;
                } else {
                    printf("CL_PLATFORM_NAME %s\n",cl_platform_name);
                    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ACCELERATOR, 1, &device_id, NULL);
                    if (err != CL_SUCCESS) {
                        printf("Error: Failed to create a device group!\n");
                        printf("Test failed\n");
                        return 0;
                    } else {
                        context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
                        if (!context) {
                            printf("Error: Failed to create a compute context!\n");
                            printf("Error: code %i\n",err);
                            printf("Test failed\n");
                            return 0;
                        } else {
                            // Load binary from disk
                            printf("loading %s\n", filename);
                            int n_i = load_file_to_memory(filename, (char **) &kernelbinary);
                            if (n_i < 0) {
                                printf("failed to load kernel from xclbin: %s\n", filename);
                                printf("Test failed\n");
                                return 0;
                            } else {
                                size_t n = n_i;
                                // Create the compute program from offline
                                printf("clCreateProgramWithBinary\n");
                                program = clCreateProgramWithBinary(context, 1, &device_id, &n, (const unsigned char **) &kernelbinary, &status, &err);
                                if ((!program) || (err!=CL_SUCCESS)) {
                                    printf("Error: Failed to create compute program from binary %d!\n", err);
                                    printf("Test failed\n");
                                    return 0;
                                } else {
                                    // Build the program executable
                                    printf("clBuildProgram\n");
                                    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
                                    if (err != CL_SUCCESS) {
                                        printf("Error: Failed to build program executable!\n");
                                        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
                                        printf("%s\n", buffer);
                                        printf("Test failed\n");
                                        return 0;
                                    } else {
                                        // Create a command commands
                                        commands = clCreateCommandQueue(context, device_id, 0, &err);
                                        if (!commands) {
                                            printf("Error: Failed to create a command commands!\n");
                                            printf("Error: code %i\n",err);
                                            printf("Test failed\n");
                                            return 0;
                                        } else {
                                            // Create the compute kernel in the program we wish to run
                                            for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
                                                kernel[compute_unit] = clCreateKernel(program, kernel_name, &err);
                                                if (!kernel[compute_unit] || err != CL_SUCCESS ) {
                                                    SUCESSFUL_EXIT_CODE = 0;
                                                }
                                            }
                                            if (!SUCESSFUL_EXIT_CODE) {
                                                printf("Error: Failed to create compute kernel!\n");
                                                printf("Test failed\n");
                                                return 0;
                                            } else {
                                                // Create the input and output arrays in device memory for our calculation
                                                for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
                                                    for (i = 0 ; i < num_input_args; i++ ) {
                                                        inputs[compute_unit][i] = clCreateBuffer(context, CL_MEM_READ_ONLY, in_args_size_vec[i], NULL, NULL);
                                                    }
                                                    for (i = 0 ; i < num_output_args; i++ ) {
                                                        outputs[compute_unit][i] =  clCreateBuffer(context, CL_MEM_WRITE_ONLY, out_args_size_vec[i], NULL, NULL);
                                                    }
                                                }
                                                // Set the arguments to our compute kernel
                                                err = CL_SUCCESS;
                                                for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
                                                    for (i = 0 ; i < num_input_args; i++ ) {
                                                            err |= clSetKernelArg(kernel[compute_unit], i, sizeof(cl_mem), &inputs[compute_unit][i]);
                                                    }
                                                    for (i = 0 ; i < num_output_args; i++ ) {
                                                            err |= clSetKernelArg(kernel[compute_unit], (num_input_args+i), sizeof(cl_mem), &outputs[compute_unit][i]);
                                                    }
                                                }
                                                if (err != CL_SUCCESS) {
                                                    printf("Error: Failed to set kernel arguments! %d\n", err);
                                                    printf("Test failed\n");
                                                    return 0;
                                                } else {
                                                    return 1;
                                                }  // __SRAI clCreateBuffer and clSetKernelArg for all IO's
                                            } // __SRAI  clCreateKernel
                                        } // __SRAI  clCreateCommandQueue
                                    } // __SRAI clBuildProgram
                                } // __SRAI clCreateProgramWithBinary
                            } // __SRAI load_file_to_memory(filename, (char **) &kernelbinary);
                        } // __SRAI  clCreateContext
                    } // __SRAI  clGetDeviceIDs
                } // __SRAI  clGetPlatformInfo ->  CL_PLATFORM_NAME
            } // __SRAI clGetPlatformInfo
        } // __SRAI clGetPlatformIDs
    }

    int run_fpga_accel (input_data_type *in_args_0,  input_data_type *in_args_1, output_data_type *results_out) {
        int err_k[NUM_OF_COMPUTE_UNITS];
        int compute_unit;
        bool SUCESSFUL_EXIT_CODE;
        bool enqueue_error;

        cl_event readevent[NUM_OF_COMPUTE_UNITS];

        
        SUCESSFUL_EXIT_CODE = 1;
        enqueue_error = 0;

        // Write our data set into the input array in device memory
        enqueue_error = 0;
        for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
            err_k[compute_unit] = clEnqueueWriteBuffer(commands, inputs[compute_unit][0], CL_TRUE, 0, in_args_size_vec[0], in_args_0, 0, NULL, NULL);
            in_args_0++;
            err_k[compute_unit] |= clEnqueueWriteBuffer(commands, inputs[compute_unit][1], CL_TRUE, 0, in_args_size_vec[1], in_args_1, 0, NULL, NULL);
            in_args_1++;
        }
        for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
            if (err_k[compute_unit] != CL_SUCCESS) {
                printf("Error: Failed to write to source Input array %d \n", compute_unit);
                enqueue_error = 1;
            }
        }
        if (enqueue_error) {
            printf("Error: Failed to write to source Input array \n");
            printf("Test failed\n");
            SUCESSFUL_EXIT_CODE = 0;
        } else {
            // Execute the kernel over the entire range of input data set
            // using the maximum number of work group items for this device
            

            enqueue_error = 0;
            for(int local_size = 0; local_size < MAX_ITERATION; local_size++) {
                for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
                    err_k[compute_unit] = clEnqueueTask(commands, kernel[compute_unit], 0, NULL, NULL);
                }
                for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
                    if (err_k[compute_unit] != CL_SUCCESS) {
                        enqueue_error = 1;
                    }
                }
            }
            if (enqueue_error) {
                printf("Error: Failed to execute kernel(s)! \n");
                printf("Test failed\n");
                SUCESSFUL_EXIT_CODE = 0;
            } else {
                // Read back the results from the device to verify the output
                enqueue_error = 0;
                for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
                    err_k[compute_unit] = clEnqueueReadBuffer( commands, outputs[compute_unit][0], CL_TRUE, 0, out_args_size_vec[0], results_out, 0, NULL, &readevent[compute_unit]);
                    results_out++;
                }
                for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
                    if (err_k[compute_unit] != CL_SUCCESS) {
                        enqueue_error = 1;
                    }
                }
                if (enqueue_error) {
                    printf("Error: Failed to read output array! \n");
                    printf("Test failed\n");
                    SUCESSFUL_EXIT_CODE = 0;
                } else {
                    for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
                        clWaitForEvents(1, &readevent[compute_unit]);
                    }
                } // __SRAI clEnqueueReadBuffer (outputs / results array)
            } // __SRAI clEnqueueTask (OR openCL clEnqueueNDRangeKernel)
        } // __SRAI clEnqueueWriteBuffer (input Array)
     return SUCESSFUL_EXIT_CODE;
    }

    int clean_fpga_hw_accel(void) {
        int compute_unit;
        int i;
        int err;
        bool SUCESSFUL_EXIT_CODE;
        SUCESSFUL_EXIT_CODE = 1;

        for(compute_unit = 0; compute_unit < NUM_OF_COMPUTE_UNITS; compute_unit++) {
            err = CL_SUCCESS;
            for (i = 0 ; i < num_input_args; i++ ) {
                err |= clReleaseMemObject(inputs[compute_unit][i]);
            }
            if (err != CL_SUCCESS) {
                SUCESSFUL_EXIT_CODE = 0;
            }
            err = CL_SUCCESS;
            for (i = 0 ; i < num_output_args; i++ ) {
                err |= clReleaseMemObject(outputs[compute_unit][i]);
            }
            err = clReleaseKernel(kernel[compute_unit]);
            if (err != CL_SUCCESS) {
                SUCESSFUL_EXIT_CODE = 0;
            }
        }

        if (err != CL_SUCCESS) {
            SUCESSFUL_EXIT_CODE = 0;
        }
        err = clReleaseProgram(program);
        if (err != CL_SUCCESS) {
            SUCESSFUL_EXIT_CODE = 0;
        }
        err = clReleaseCommandQueue(commands);
        if (err != CL_SUCCESS) {
            SUCESSFUL_EXIT_CODE = 0;
        }
        err = clReleaseContext(context);
        if (err != CL_SUCCESS) {
            SUCESSFUL_EXIT_CODE = 0;
        }
        
        if (!SUCESSFUL_EXIT_CODE) {
            return 0;
        } else { 
            return 1;
        }
    }



};

#endif //SDACCEL_UTILITIES_H_
