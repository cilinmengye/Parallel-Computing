#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>

#include <driver_functions.h>

#include <thrust/scan.h>
#include <thrust/device_ptr.h>
#include <thrust/device_malloc.h>
#include <thrust/device_free.h>

#include "CycleTimer.h"

#define THREADS_PER_BLOCK 256

/*
 * In the starter code, the reference solution scan implementation above assumes that 
 * the input array's length (N) is a power of 2. 
 * In the cudaScan function, we solve this problem by 
 * rounding the input array length to the next power of 2 when 
 * allocating the corresponding buffers on the GPU. 
 */
// helper function to round an integer up to the next power of 2
// (n == 3 return 4), (n == 5 return 8)
static inline int nextPow2(int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

static inline std::pair<int, int> calGridBlock(int N, int two_d) {
    // 注意到threadPerBlock == 512 == 2^9，totalThread一定是2的倍数（因为我们保证N是2的倍数）
    // 所以除了totalThread <= threadPerBlock外，blockNum = totalThread / threadPerBlock一定是整数
    const int threadPerBlock = 512;
    int two_dplus1 = 2 * two_d;
    int totalThread = N / two_dplus1;
    int blockNum;
    if (totalThread <= threadPerBlock) return {1, totalThread};
    blockNum = totalThread / threadPerBlock;
    return {blockNum, threadPerBlock};
}

// exclusive_scan --
//
// Implementation of an exclusive scan on global memory array `input`,
// with results placed in global memory `result`.
//
// N is the logical size of the input and output arrays, however
// students can assume that both the start and result arrays we
// allocated with next power-of-two sizes as described by the comments
// in cudaScan().  This is helpful, since your parallel scan
// will likely write to memory locations beyond N, but of course not
// greater than N rounded up to the next power of 2.
//
// Also, as per the comments in cudaScan(), you can implement an
// "in-place" scan, since the timing harness makes a copy of input and
// places it in result
__global__ void Parallel_Upsweep(int two_d, int *output) {
    int two_dplus1 = 2 * two_d;
    int i = two_dplus1 * (blockIdx.x * blockDim.x + threadIdx.x);
    output[i + two_dplus1 - 1] += output[i + two_d - 1];
}

__global__ void Set_Zero(int N, int *output) {
    output[N - 1] = 0;
}

__global__ void Parallel_Downsweep(int two_d, int *output) {
    int two_dplus1 = 2 * two_d;
    int i = two_dplus1 * (blockIdx.x * blockDim.x + threadIdx.x);
    int t = output[i + two_d - 1];
    output[i + two_d - 1] = output[i + two_dplus1 - 1];
    output[i + two_dplus1 - 1] += t;
}

void exclusive_scan(int* input, int N, int* result)
{

    // CS149 TODO:
    //
    // Implement your exclusive scan implementation here.  Keep in
    // mind that although the arguments to this function are device
    // allocated arrays, this is a function that is running in a thread
    // on the CPU.  Your implementation will need to make multiple calls
    // to CUDA kernel functions (that you must write) to implement the
    // scan.
    /*
     * a naive implementation of scan might launch N CUDA threads for each iteration of 
     * the parallel loops in the pseudocode, and using conditional execution in the kernel 
     * to determine which threads actually need to do work. 
     * Such a solution will not be performant! (Consider the last outmost loop iteration of 
     * the upsweep phase, where only two threads would do work!). 
     * A full credit solution will only launch one CUDA thread for each iteration of 
     * the innermost parallel loops.
     */
     N = nextPow2(N);

     for (int two_d = 1; two_d <= N / 2; two_d *= 2) {
        auto [blockNum, threadPerBlock] = calGridBlock(N, two_d);
        Parallel_Upsweep<<<blockNum, threadPerBlock>>>(two_d, result);
        cudaDeviceSynchronize();
    }

    Set_Zero<<<1, 1>>>(N, result);
    cudaDeviceSynchronize();

    for (int two_d = N / 2; two_d >= 1; two_d /= 2) {
        auto [blockNum, threadPerBlock] = calGridBlock(N, two_d);
        Parallel_Downsweep<<<blockNum, threadPerBlock>>>(two_d, result);
        cudaDeviceSynchronize();
    }
}


//
// cudaScan --
//
// This function is a timing wrapper around the student's
// implementation of scan - it copies the input to the GPU
// and times the invocation of the exclusive_scan() function
// above. Students should not modify it.
double cudaScan(int* inarray, int* end, int* resultarray)
{
    int* device_result;
    int* device_input;
    int N = end - inarray;  

    // This code rounds the arrays provided to exclusive_scan up
    // to a power of 2, but elements after the end of the original
    // input are left uninitialized and not checked for correctness.
    //
    // Student implementations of exclusive_scan may assume an array's
    // allocated length is a power of 2 for simplicity. This will
    // result in extra work on non-power-of-2 inputs, but it's worth
    // the simplicity of a power of two only solution.

    int rounded_length = nextPow2(end - inarray);
    
    cudaMalloc((void **)&device_result, sizeof(int) * rounded_length);
    cudaMalloc((void **)&device_input, sizeof(int) * rounded_length);

    // For convenience, both the input and output vectors on the
    // device are initialized to the input values. This means that
    // students are free to implement an in-place scan on the result
    // vector if desired.  If you do this, you will need to keep this
    // in mind when calling exclusive_scan from find_repeats.
    cudaMemcpy(device_input, inarray, (end - inarray) * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(device_result, inarray, (end - inarray) * sizeof(int), cudaMemcpyHostToDevice);

    double startTime = CycleTimer::currentSeconds();

    /*
     * When calling your exclusive_scan implementation, 
     * remember that the contents of the start array are copied over to the output array. 
     * Also, the arrays passed to exclusive_scan are assumed to be in device memory. 
     */
    exclusive_scan(device_input, N, device_result);

    // Wait for completion
    cudaDeviceSynchronize();
    double endTime = CycleTimer::currentSeconds();
       
    cudaMemcpy(resultarray, device_result, (end - inarray) * sizeof(int), cudaMemcpyDeviceToHost);

    double overallDuration = endTime - startTime;
    return overallDuration; 
}


// cudaScanThrust --
//
// Wrapper around the Thrust library's exclusive scan function
// As above in cudaScan(), this function copies the input to the GPU
// and times only the execution of the scan itself.
//
// Students are not expected to produce implementations that achieve
// performance that is competition to the Thrust version, but it is fun to try.
double cudaScanThrust(int* inarray, int* end, int* resultarray) {

    int length = end - inarray;
    thrust::device_ptr<int> d_input = thrust::device_malloc<int>(length);
    thrust::device_ptr<int> d_output = thrust::device_malloc<int>(length);
    
    cudaMemcpy(d_input.get(), inarray, length * sizeof(int), cudaMemcpyHostToDevice);

    double startTime = CycleTimer::currentSeconds();

    thrust::exclusive_scan(d_input, d_input + length, d_output);

    cudaDeviceSynchronize();
    double endTime = CycleTimer::currentSeconds();
   
    cudaMemcpy(resultarray, d_output.get(), length * sizeof(int), cudaMemcpyDeviceToHost);

    thrust::device_free(d_input);
    thrust::device_free(d_output);

    double overallDuration = endTime - startTime;
    return overallDuration; 
}

__global__ void findRepeat_Flag(int *input, int N, int *output) {
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    
    if (i < N - 1) output[i] = (input[i] == input[i + 1]);
}

__global__ void findRepeat_Set(int *flags, int *idxs, int N, int *output) {
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i < N - 1 && flags[i] == 1) output[idxs[i]] = i;
}
// find_repeats --
//
// Given an array of integers `device_input`, returns an array of all
// indices `i` for which `device_input[i] == device_input[i+1]`.
//
// Returns the total number of pairs found
int find_repeats(int* device_input, int length, int* device_output) {

    // CS149 TODO:
    //
    // Implement this function. You will probably want to
    // make use of one or more calls to exclusive_scan(), as well as
    // additional CUDA kernel launches.
    //    
    // Note: As in the scan code, the calling code ensures that
    // allocated arrays are a power of 2 in size, so you can use your
    // exclusive_scan function with them. However, your implementation
    // must ensure that the results of find_repeats are correct given
    // the actual array length.
    int res;
    int *device_flag, *device_idx;
    int rounded_length = nextPow2(length);
    cudaMalloc((void **)&device_flag, sizeof(int) * rounded_length);
    cudaMalloc((void **)&device_idx, sizeof(int) * rounded_length);

    const int threadPerBlock = 512;
    int blockNum = (length + threadPerBlock - 1) / threadPerBlock;
    // GPU并行计算数组元素是否满足device_input[i] == device_input[i + 1]
    findRepeat_Flag<<<blockNum, threadPerBlock>>>(device_input, length, device_flag);
    cudaDeviceSynchronize();

    cudaMemcpy(device_idx, device_flag, sizeof(int) * length, cudaMemcpyDeviceToDevice);
    // 对device_flag求扩展前缀和
    // 最大问题是我们如何将符合device_input[i] == device_input[i + 1]的i并行地放入device_input中？
    // 通过对device_flag求扩展前缀和得到的device_idx就可以知道放入device_input下标了
    // 即满足device_flag[i] == 1时的i，在device_idx[i]中的值即是device_input的下标
    // 即if (device_flag[i] == 1) device_input[device_idx[i]] = i;
    exclusive_scan(device_flag, length, device_idx);
    cudaDeviceSynchronize();

    findRepeat_Set<<<blockNum, threadPerBlock>>>(device_flag, device_idx, length, device_output);

    cudaMemcpy(&res, device_idx + length - 1, sizeof(int), cudaMemcpyDeviceToHost);

    cudaFree(device_flag);
    cudaFree(device_idx);
    return res; 
}


/*
 * Grading: We will test your code for correctness and performance on random input arrays.
 */
//
// cudaFindRepeats --
//
// Timing wrapper around find_repeats. You should not modify this function.
double cudaFindRepeats(int *input, int length, int *output, int *output_length) {

    int *device_input;
    int *device_output;
    int rounded_length = nextPow2(length);
    
    cudaMalloc((void **)&device_input, rounded_length * sizeof(int));
    cudaMalloc((void **)&device_output, rounded_length * sizeof(int));
    cudaMemcpy(device_input, input, length * sizeof(int), cudaMemcpyHostToDevice);

    cudaDeviceSynchronize();
    double startTime = CycleTimer::currentSeconds();
    
    int result = find_repeats(device_input, length, device_output);

    cudaDeviceSynchronize();
    double endTime = CycleTimer::currentSeconds();

    // set output count and results array
    *output_length = result;
    cudaMemcpy(output, device_output, length * sizeof(int), cudaMemcpyDeviceToHost);

    cudaFree(device_input);
    cudaFree(device_output);

    float duration = endTime - startTime; 
    return duration;
}



void printCudaInfo()
{
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    printf("---------------------------------------------------------\n");
    printf("Found %d CUDA devices\n", deviceCount);

    for (int i=0; i<deviceCount; i++)
    {
        cudaDeviceProp deviceProps;
        cudaGetDeviceProperties(&deviceProps, i);
        printf("Device %d: %s\n", i, deviceProps.name);
        printf("   SMs:        %d\n", deviceProps.multiProcessorCount);
        printf("   Global mem: %.0f MB\n",
               static_cast<float>(deviceProps.totalGlobalMem) / (1024 * 1024));
        printf("   CUDA Cap:   %d.%d\n", deviceProps.major, deviceProps.minor);
    }
    printf("---------------------------------------------------------\n"); 
}
