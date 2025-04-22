#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"

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
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
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
    public:
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
struct TaskNode {
    TaskID bulk_task_id; // 任务节点ID
    int num_total_tasks; // 任务节点总任务数
    int num_total_indegree; // 任务节点总入度数
    IRunnable *runnable;
};

class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    private:
        std::mutex m_taskQueue_mutex;
        std::condition_variable m_taskQueue_condition;
        std::queue<TaskNode> m_taskQueue;

        std::unordered_map<TaskID, std::vector<TaskID>> m_taskGraph;
        std::unordered_map<TaskID, TaskNode> m_taskId2Node;

        std::unordered_map<TaskID, int> m_num_done_tasks;
        std::mutex m_done_mutex;

        std::unordered_map<TaskID, int> m_next_id_tasks;
        std::mutex m_id_mutex;

        std::unordered_map<TaskID, bool> m_have_done_tasks;
        std::mutex m_haveDone_mutex;
        
        std::mutex m_num_bulk_tasks_mutex; // m_num_total_bulk_tasks和m_num_done_bulk_tasks
        std::condition_variable m_num_bulk_tasks_condition; // 共用一把锁和条件变量
        int m_num_total_bulk_tasks;
        int m_num_done_bulk_tasks;

        bool m_stop;

        std::vector<std::thread> m_threads;
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        void threadWorker(); // 执行单个task任务的实例
        void finishTaskNode(TaskID bulk_task_id); // 完成ID为bulk_task_id的任务节点 
};

#endif
