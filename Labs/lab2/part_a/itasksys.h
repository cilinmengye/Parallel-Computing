#ifndef _ITASKSYS_H
#define _ITASKSYS_H
#include <assert.h>
#include <vector>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
typedef int TaskID;

class IRunnable {
    public:
        virtual ~IRunnable();

        /*
          Executes an instance of the task as part of a bulk task launch.
          
           - task_id: the current task identifier. This value will be
              between 0 and num_total_tasks-1.
              
           - num_total_tasks: the total number of tasks in the bulk
             task launch.
         */
        virtual void runTask(int task_id, int num_total_tasks) = 0;
};

// 包含了并行处理一大批任务的API，包括执行的线程数，同步执行，异步执行，同步屏障
class ITaskSystem {
    public:
        /*
          Instantiates a task system.

           - num_threads: the maximum number of threads that the task system
             can use.
         */
        ITaskSystem(int num_threads);
        virtual ~ITaskSystem();
        virtual const char* name() = 0;

        /*
          Executes a bulk task launch of num_total_tasks.  Task
          execution is synchronous with the calling thread, so run()
          will return only when the execution of all tasks is
          complete.
          处理一大批任务，仅当全部任务完成run才返回
        */
        virtual void run(IRunnable* runnable, int num_total_tasks) = 0;

        /*
          Executes an asynchronous bulk task launch of
          num_total_tasks, but with a dependency on prior launched
          tasks.


          The task runtime must complete execution of the tasks
          associated with all bulk task launches referenced in the
          array `deps` before beginning execution of *any* task in
          this bulk task launch.

          The caller must invoke sync() to guarantee completion of the
          tasks in this bulk task launch.
 
          Returns an identifer that can be used in subsequent calls to
          runAsnycWithDeps() to specify a dependency of some future
          bulk task launch on this bulk task launch.
         */
        virtual TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                        const std::vector<TaskID>& deps) = 0;

        /*
          Blocks until all tasks created as a result of **any prior**
          runXXX calls are done.
          即相当于pthread_join()的效果，同步屏障
         */
        virtual void sync() = 0;
};
#endif
