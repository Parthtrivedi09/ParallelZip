#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>


class ThreadPool {

public:

    // Create a pool containing a fixed number of worker threads.
    explicit ThreadPool(std::size_t threadCount);

    // Wait for workers to finish and shut the pool down safely.
    ~ThreadPool();

    // Add a new task to the shared task queue.
    void enqueue(std::function<void()> task);

    // Block until every queued/running task has completed.
    void wait();


private:

    // Worker threads remain alive and repeatedly pull
    // jobs from the task queue.
    std::vector<std::thread> workers;

    // Shared queue containing work waiting to be executed.
    std::queue<std::function<void()>> tasks;

    // Protects access to the shared task queue and state.
    std::mutex queueMutex;

    // Wakes workers when new work becomes available.
    std::condition_variable taskAvailable;

    // Used by wait() to detect when all work is finished.
    std::condition_variable allFinished;

    // Becomes true when the thread pool is shutting down.
    bool stopping = false;

    // Number of workers currently executing a task.
    std::size_t activeTasks = 0;
};


#endif