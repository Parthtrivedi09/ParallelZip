#include "thread_pool.h"

#include <stdexcept>
#include <utility>


ThreadPool::ThreadPool(std::size_t threadCount) {

    if (threadCount == 0) {
        throw std::invalid_argument(
            "Thread pool must contain at least one thread."
        );
    }

    workers.reserve(threadCount);

    // Create persistent worker threads.
    for (std::size_t i = 0; i < threadCount; i++) {

        workers.emplace_back([this]() {

            while (true) {

                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(
                        queueMutex
                    );

                    // Sleep until either:
                    // 1. a new task becomes available, or
                    // 2. the pool is shutting down.
                    taskAvailable.wait(
                        lock,
                        [this]() {
                            return stopping || !tasks.empty();
                        }
                    );

                    // Exit only when shutdown has been requested
                    // and there is no remaining work.
                    if (stopping && tasks.empty()) {
                        return;
                    }

                    // Take one task from the queue.
                    task = std::move(tasks.front());
                    tasks.pop();

                    activeTasks++;
                }

                // Execute outside the mutex.
                //
                // This is important because other workers should
                // still be able to access the queue concurrently.
                task();

                {
                    std::lock_guard<std::mutex> lock(
                        queueMutex
                    );

                    activeTasks--;

                    // If nothing is queued or running,
                    // wake anyone waiting inside wait().
                    if (tasks.empty() && activeTasks == 0) {
                        allFinished.notify_all();
                    }
                }
            }
        });
    }
}


void ThreadPool::enqueue(
    std::function<void()> task
) {

    {
        std::lock_guard<std::mutex> lock(
            queueMutex
        );

        if (stopping) {
            throw std::runtime_error(
                "Cannot enqueue task after thread pool shutdown."
            );
        }

        tasks.push(std::move(task));
    }

    // Wake one worker because new work is available.
    taskAvailable.notify_one();
}


void ThreadPool::wait() {

    std::unique_lock<std::mutex> lock(
        queueMutex
    );

    // Sleep until there are no waiting tasks
    // and no workers currently executing tasks.
    allFinished.wait(
        lock,
        [this]() {
            return tasks.empty() &&
                   activeTasks == 0;
        }
    );
}


ThreadPool::~ThreadPool() {

    {
        std::lock_guard<std::mutex> lock(
            queueMutex
        );

        stopping = true;
    }

    // Wake all sleeping workers so they can see
    // stopping == true and terminate.
    taskAvailable.notify_all();

    for (std::thread& worker : workers) {

        if (worker.joinable()) {
            worker.join();
        }
    }
}