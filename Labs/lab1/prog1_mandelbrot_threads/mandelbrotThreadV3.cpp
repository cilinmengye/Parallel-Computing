#include <stdio.h>
#include <thread>
#include <stdlib.h>

#include "CycleTimer.h"

typedef struct {
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int* output;
    int threadId;
    int numThreads;
} WorkerArgs;


extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[]);

static inline int mandel(float c_re, float c_im, int count)
{
        float z_re = c_re, z_im = c_im;
        int i;
      
        for (i = 0; i < count; ++i) {
    
            if (z_re * z_re + z_im * z_im > 4.f)
                break;
    
            float new_re = z_re*z_re - z_im*z_im;
            float new_im = 2.f * z_re * z_im;
            z_re = c_re + new_re;
            z_im = c_im + new_im;
        }
        return i;
}

//
// workerThreadStart --
//
// Thread entrypoint.
void workerThreadStart(WorkerArgs * const args) {

    // TODO FOR CS149 STUDENTS: Implement the body of the worker
    // thread here. Each thread should make a call to mandelbrotSerial()
    // to compute a part of the output image.  For example, in a
    // program that uses two threads, thread 0 could compute the top
    // half of the image and thread 1 could compute the bottom half.
    double startTime = CycleTimer::currentSeconds();

    float x0 = args->x0, y0 = args->y0;
    float x1 = args->x1, y1 = args->y1;
    int height = args->height, width = args->width;
    int numThreads = args->numThreads, threadId = args->threadId;

    float dy = (y1 - y0) / height;
    float dx = (x1 - x0) / width;
    int dh = height / numThreads;
    int pos = threadId;
    printf("Hello world from thread %d\n", threadId);

    for (int j = 0; j < dh; j++) {
        pos = (pos - 1 + numThreads) % numThreads;
        for (int i = 0; i < width; i++) {
            float x = x0 + i * dx;
            float y = y0 + (pos + numThreads * j) * dy;
            int index = (pos + numThreads * j) * width + i;

            args->output[index] = mandel(x, y, args->maxIterations);
        }
    }
    if (height % numThreads != 0) {
        pos = (pos - 1 + numThreads) % numThreads;
        if (pos < (height - numThreads * dh)) {
            for (int i = 0; i < width; i++) {
                float x = x0 + i * dx;
                float y = y0 + (pos + numThreads * dh) * dy;
                int index = (pos + numThreads * dh) * width + i;

                args->output[index] = mandel(x, y, args->maxIterations);
            }
        }
    }

    double endTime = CycleTimer::currentSeconds();
    printf("[mandelbrot thread %d]:\t\t[%.3f] ms\n", threadId, (endTime - startTime) * 1000);
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations, int output[])
{
    static constexpr int MAX_THREADS = 32;

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i=0; i<numThreads; i++) {
      
        // TODO FOR CS149 STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        
        args[i].x0 = x0; args[i].y0 = y0;
        args[i].x1 = x1; args[i].y1 = y1;
        args[i].width = width; args[i].height = height;
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output;
        args[i].threadId = i;
    }

    // Spawn the worker threads.  Note that only numThreads-1 std::threads
    // are created and the main application thread is used as a worker
    // as well.
    for (int i=1; i<numThreads; i++) {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }
    
    workerThreadStart(&args[0]);

    // join worker threads
    for (int i=1; i<numThreads; i++) {
        workers[i].join();
    }
}

