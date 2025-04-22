#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"

template <typename T>
class TaskQueue {
    private:
        std::queue<T> taskQueue;
        std::mutex mutex;
    public:
        TaskQueue() {}
        ~TaskQueue() {}
        // 进队
        void enqueue(T& t) {
            std::unique_lock<std::mutex> lock(mutex);
            taskQueue.push(t);
        }
        // 出队
        bool dequeue(T &t) {
            std::unique_lock<std::mutex> lock(mutex);
            if (taskQueue.empty()) return false;
            t = std::move(taskQueue.front());
            taskQueue.pop();
            return true;
        }
        // 查空
        bool empty() {
            std::unique_lock<std::mutex> lock(mutex);
            return taskQueue.empty();
        }
        // 任务个数
        int size() {
            std::unique_lock<std::mutex> lock(mutex);
            return taskQueue.size();
        }
};

enum ThreadStatus{
    T_FREE,
    T_SPINING,
    T_WORKING
};

struct TaskParameter {
    IRunnable* runnable;
    int taskId;
    int num_total_tasks;
};

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        int num_threads;
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        void threadRun(IRunnable* runnable, int threadId, int num_threads, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    private:
        /*
         * creates all worker threads up front call to run()
         * spinning
         * How might a worker thread determine there is work to do?
         * How run() to determine that all tasks in the bulk task launch have completed?
         */
        class ThreadPool {
            private:
                class ThreadWorker {
                    private:
                        int threadId;
                        ThreadPool* pool;
                    public:
                        ThreadWorker(ThreadPool* pool, int threadId): threadId(threadId), pool(pool) {}
                        void operator()() {
                            bool dequeue;
                            TaskParameter taskParameter;
                            while(!pool->finish) {
                                dequeue = pool->taskQueue.dequeue(taskParameter);
                                if (dequeue) {
                                    {
                                        std::unique_lock<std::mutex> lock(pool->threadStatusMutexs[threadId]);
                                        pool->threadStatus[threadId] = T_WORKING;
                                    }
                                    taskParameter.runnable->runTask(taskParameter.taskId, taskParameter.num_total_tasks);
                                } else {
                                    {
                                        std::unique_lock<std::mutex> lock(pool->threadStatusMutexs[threadId]);
                                        pool->threadStatus[threadId] = T_SPINING;
                                    }
                                    while (!pool->finish && pool->taskQueue.empty());
                                }
                            }
                        }
                };
                bool finish;
                TaskQueue<TaskParameter> taskQueue;
                std::vector<std::thread> threads;
            public:
                std::vector<ThreadStatus> threadStatus;
                std::vector<std::mutex> threadStatusMutexs;
                ThreadPool(int num_threads): finish(false), threads(std::vector<std::thread>(num_threads)),
                                             threadStatus(std::vector<ThreadStatus>(num_threads)),
                                             threadStatusMutexs(std::vector<std::mutex>(num_threads)) {}
                ~ThreadPool() {}
                void initThreadPool() {
                    finish = false;
                    int i;
                    int num_threads = threads.size();
                    for (i = 0; i < num_threads; i++) threadStatus[i] = T_FREE;
                    for (i = 0; i < num_threads; i++) threads[i] = std::thread(ThreadWorker(this, i));
                }
                void shutdownThreadPool() {
                    finish = true;
                    int i;
                    int num_threads = threads.size();
                    for (i = 0; i < num_threads; i++) 
                        if (threads[i].joinable()) threads[i].join();
                }
                void submitTask(TaskParameter &t) {
                    taskQueue.enqueue(t);
                }
                bool emptyTask() {
                    return taskQueue.empty();
                }
        };
    public:
        int num_threads;
        ThreadPool threadPool;
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    private:
        class ThreadPool {
            private:
                class ThreadWorker {
                    private:
                        int threadId;
                        ThreadPool* threadPool;
                    public:
                        ThreadWorker(int threadId, ThreadPool* threadPool): threadId(threadId), threadPool(threadPool) {}
                        ~ThreadWorker() {}
                        void operator()() {
                            bool dequeue;
                            TaskParameter taskParameter;
                            while (!threadPool->stop) {
                                std::unique_lock<std::mutex> lt_lock(threadPool->left_mutex);
                                threadPool->left_lock.wait(lt_lock, [this]() { 
                                    return threadPool->stop || !threadPool->taskQueue.empty(); });
                                dequeue = threadPool->taskQueue.dequeue(taskParameter);
                                lt_lock.unlock();
                                
                                if (dequeue) taskParameter.runnable->runTask(taskParameter.taskId, taskParameter.num_total_tasks);
                                else continue;

                                std::unique_lock<std::mutex> de_lock(threadPool->done_mutex);
                                threadPool->num_done_tasks++;
                                if (threadPool->num_done_tasks == threadPool->num_total_tasks) 
                                    threadPool->done_lock.notify_all();
                                de_lock.unlock();
                            }
                        }
                };
                bool stop; // 强制结束这一切
                TaskQueue<TaskParameter> taskQueue; // 任务队列
                std::vector<std::thread> threads; 
            public:
                int num_total_tasks;
                int num_done_tasks;
                std::mutex done_mutex; // 解决ITaskSystem的run函数为等待全部线程完成任务而spining的问题
                std::condition_variable done_lock;// 若全部线程并非完成了全部任务的条件成立则wait, 否则唤醒
                std::mutex left_mutex; // 解决线程池中线程spining的问题
                std::condition_variable left_lock; // 若任务队列为空的条件成立则wait, 否则唤醒

                ThreadPool(int num_threads): stop(false), threads(std::vector<std::thread>(num_threads)),
                                             num_total_tasks(0), num_done_tasks(0) {}
                ~ThreadPool() {}
                void initThreadPool() {
                    int i, num_threads = threads.size();
                    for (i = 0; i < num_threads; i++) threads[i] = std::thread(ThreadWorker(i, this));
                }
                void shutdownPool() {
                    stop = true;
                    left_lock.notify_all();

                    int i, num_threads = threads.size();
                    for (i = 0; i < num_threads; i++) if (threads[i].joinable()) threads[i].join();
                }
                void submitTask(TaskParameter& t) {
                    std::unique_lock<std::mutex> lock(left_mutex);
                    taskQueue.enqueue(t);
                    left_lock.notify_one();
                }
        };
    public:
        ThreadPool threadPool;
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

#endif
