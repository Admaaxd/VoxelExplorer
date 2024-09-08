#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <exception>
#include <chrono>

class ThreadPool {
public:
    // Initializes the thread pool with a specified number of threads and optional delay.
    ThreadPool(size_t threads, std::chrono::milliseconds delay = std::chrono::milliseconds(0))
        : stop(false), delay(delay) {

        // Reserve space for worker threads.
        workers.reserve(threads);

        // Create and start the specified number of worker threads.
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;  // A task to be executed by the thread.

                    // Acquire lock to safely access the task queue.
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);

                        // Wait until there is a task to execute or the pool is stopping.
                        this->condition.wait(lock, [this] {
                            return this->stop.load() || !this->tasks.empty();
                        });

                        // If stop is true and there are no remaining tasks, exit the loop.
                        if (this->stop.load() && this->tasks.empty())
                            return;

                        // Get the next task from the queue.
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    // If a delay is specified, the thread sleeps for the given duration before executing the task.
                    if (this->delay.count() > 0) {
                        std::this_thread::sleep_for(this->delay);
                    }

                    // Execute the task.
                    task();
                }
            });
        }
    }

    // Enqueue a new task to be executed by the thread pool.
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        // Wrap the task into a packaged_task to handle the return value.
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        // Retrieve the future from the packaged_task so we can access the result later.
        std::future<return_type> res = task->get_future();

        {
            std::lock_guard<std::mutex> lock(queue_mutex);

            // If the pool is stopping, don't allow new tasks to be enqueued.
            if (stop.load())
                throw std::runtime_error("enqueue on stopped ThreadPool");

            // Add the task to the queue as a lambda function.
            tasks.emplace([task]() { (*task)(); });
        }

        // Notify one worker thread that a new task is available.
        condition.notify_one();
        return res;
    }

    // Waits for all worker threads to finish and cleans up resources.
    ~ThreadPool() {
        stop.store(true);
        condition.notify_all();

        // Join all worker threads (wait for them to finish).
        for (std::thread& worker : workers) {
            if (worker.joinable())
                worker.join();
        }
    }

private:
    // Vector of worker threads.
    std::vector<std::thread> workers;

    // Queue of tasks to be executed by the workers.
    std::queue<std::function<void()>> tasks;

    // Mutex to protect access to the task queue.
    std::mutex queue_mutex;

    // Condition variable to notify worker threads when new tasks are available.
    std::condition_variable condition;

    // Flag to indicate when the pool is stopping.
    std::atomic<bool> stop;

    // Delay between task executions.
    std::chrono::milliseconds delay;
};
