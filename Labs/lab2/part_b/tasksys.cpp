#include "tasksys.h"


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

void TaskSystemParallelThreadPoolSleeping::finishTaskNode(TaskID bulk_task_id) {
    int i, childNum;
    TaskID childId;

    // 遍历任务图，删除所完成任务节点的子节点的入度
    m_haveDone_mutex.lock();
    childNum = m_taskGraph[bulk_task_id].size();
    //printf("finishTaskNode bulk task %d have childNum %d\n", bulk_task_id, childNum);
    for (i = 0; i < childNum; i++) {
        childId = m_taskGraph[bulk_task_id][i];
        m_taskId2Node[childId].num_total_indegree--;
        //printf("finishTaskNode %d and %d indegree--\n", bulk_task_id, childId);
        if (m_taskId2Node[childId].num_total_indegree == 0) { // 若子节点的入度为0则可加入任务队列
            m_taskQueue_mutex.lock();
            //printf("finishTaskNode %d indegree == 0 push in queue\n", bulk_task_id);
            m_taskQueue.push(m_taskId2Node[childId]);
            m_taskQueue_condition.notify_all(); // 任务队列可能非空了，进行提醒
            m_taskQueue_mutex.unlock();
        }
    }
    m_have_done_tasks[bulk_task_id] = true;
    //printf("finishTaskNode %d finish\n", bulk_task_id);
    m_haveDone_mutex.unlock();

    // 更新总完成的任务节点数，若完成了全部的任务节点数则告知调用sync()的线程
    m_num_bulk_tasks_mutex.lock();
    m_num_done_bulk_tasks++;
    //printf("finishTaskNode: finish bulk task %d, m_num_done_bulk_tasks:%d, m_num_total_bulk_tasks:%d\n", 
    //    bulk_task_id, m_num_done_bulk_tasks, m_num_total_bulk_tasks);
    if (m_num_done_bulk_tasks == m_num_total_bulk_tasks) m_num_bulk_tasks_condition.notify_all();
    m_num_bulk_tasks_mutex.unlock();
}

void TaskSystemParallelThreadPoolSleeping::threadWorker() {
    int task_id;
    TaskNode tasknode;

    while (!m_stop) {
        // 从任务队列中取出任务节点
        std::unique_lock<std::mutex> taskQueue_lock(m_taskQueue_mutex);
        //printf("know m_taskQueue size:%d\n", int(m_taskQueue.size()));
        m_taskQueue_condition.wait(taskQueue_lock, [this] { return m_stop || !m_taskQueue.empty(); });
        if (m_stop) {
            taskQueue_lock.unlock();
            break;
        }
        tasknode = m_taskQueue.front();
        //printf("bulk task id: %d get tasknode\n", tasknode.bulk_task_id);
        taskQueue_lock.unlock();
        

        // 分配任务ID
        m_id_mutex.lock();
        assert(m_next_id_tasks.count(tasknode.bulk_task_id) > 0);
        task_id = m_next_id_tasks[tasknode.bulk_task_id]++;
        if (task_id == tasknode.num_total_tasks) { // 任务节点中的全部任务被分配完毕，可将任务节点从任务队列删除
            m_id_mutex.unlock();
            m_taskQueue_mutex.lock();
            //printf("bulk task %d pop from queue\n", tasknode.bulk_task_id);
            m_taskQueue.pop();
            m_taskQueue_mutex.unlock();
            continue;
        } else if (task_id > tasknode.num_total_tasks) {
            m_id_mutex.unlock();
            //printf("bulk task %d task id %d not verify\n", tasknode.bulk_task_id, task_id);
            continue;
        }
        m_id_mutex.unlock();

        // 执行任务
        assert(task_id < tasknode.num_total_tasks);
        tasknode.runnable->runTask(task_id, tasknode.num_total_tasks);
        //printf("bulk task id: %d and get task id: %d\n", tasknode.bulk_task_id, task_id);    
        
        // 更新任务节点状态
        m_done_mutex.lock();
        //printf("need bulk task %d have else m_num_done_tasks assert\n", tasknode.bulk_task_id);
        assert(m_num_done_tasks.count(tasknode.bulk_task_id) > 0);
        m_num_done_tasks[tasknode.bulk_task_id]++;
        if (m_num_done_tasks[tasknode.bulk_task_id] == tasknode.num_total_tasks) {
            m_done_mutex.unlock(); 
            finishTaskNode(tasknode.bulk_task_id);
        } else {
            assert(m_num_done_tasks[tasknode.bulk_task_id] <= tasknode.num_total_tasks);
            m_done_mutex.unlock();
        }
    }
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    int i;

    m_stop = false;
    m_num_total_bulk_tasks = 0;
    m_num_done_bulk_tasks = 0;
    for (i = 0; i < num_threads; i++) m_threads.push_back(std::thread([this]() { threadWorker(); }));
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    int i, num_threads;

    m_taskQueue_mutex.lock();
    m_stop = true;
    m_taskQueue_condition.notify_all(); // 要结束了，进行提醒
    m_taskQueue_mutex.unlock();
    
    num_threads = m_threads.size();
    for (i = 0; i < num_threads; i++)
        if (m_threads[i].joinable())
            m_threads[i].join();
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    /*
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
    */
    std::vector<TaskID> deps;
    runAsyncWithDeps(runnable, num_total_tasks, deps);
    sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //
    /*
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
    */
    int i, depsSize;

    // 初始化任务节点
    TaskNode tasknode;
    tasknode.num_total_tasks = num_total_tasks;
    tasknode.runnable = runnable;

    m_num_bulk_tasks_mutex.lock();
    tasknode.bulk_task_id = m_num_total_bulk_tasks++;
    m_num_bulk_tasks_mutex.unlock();

    depsSize = deps.size();
    tasknode.num_total_indegree = depsSize;
    
    m_done_mutex.lock();
    m_num_done_tasks[tasknode.bulk_task_id] = 0;
    m_done_mutex.unlock();

    m_id_mutex.lock();
    m_next_id_tasks[tasknode.bulk_task_id] = 0;
    m_id_mutex.unlock();
    
    m_haveDone_mutex.lock();
    m_have_done_tasks[tasknode.bulk_task_id] = false;
    m_haveDone_mutex.unlock();
    //printf("bulk task %d have init\n", tasknode.bulk_task_id);

    // 更新任务图信息
    m_haveDone_mutex.lock();
    if (depsSize != 0) {
        //printf("runAsyncWithDeps bulk task %d have deps %d\n", tasknode.bulk_task_id, depsSize);
        for (i = 0; i < depsSize; i++) {
            m_taskGraph[deps[i]].push_back(tasknode.bulk_task_id);
            //printf("runAsyncWithDeps %d -> %d\n", deps[i], tasknode.bulk_task_id);
            if (m_have_done_tasks[deps[i]]) {
                //printf("runAsyncWithDeps bulk task id %d have finish and bulk task id %d indegree--\n", deps[i], tasknode.bulk_task_id);
                tasknode.num_total_indegree--;
            }
        }
    }
    m_taskId2Node[tasknode.bulk_task_id] = tasknode;
    m_haveDone_mutex.unlock();
    
    if (tasknode.num_total_indegree == 0) {
        m_taskQueue_mutex.lock();
        //printf("runAsyncWithDeps bulk task id %d indegree == 0 push in queue\n", tasknode.bulk_task_id);
        m_taskQueue.push(tasknode);
        m_taskQueue_condition.notify_all(); // 任务队列可能非空了，进行提醒
        m_taskQueue_mutex.unlock();
    }

    return tasknode.bulk_task_id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //
    std::unique_lock<std::mutex> lock(m_num_bulk_tasks_mutex);
    //printf("sync: m_num_done_bulk_tasks:%d m_num_total_bulk_tasks:%d\n", m_num_done_bulk_tasks, m_num_total_bulk_tasks);
    m_num_bulk_tasks_condition.wait(lock, [this] { return m_num_done_bulk_tasks == m_num_total_bulk_tasks; });
    lock.unlock();
    //printf("sync: end\n");
    return;
}
