#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <functional>
#include <chrono>
#include <algorithm>

class TaskQueue {
 public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using TaskFunc  = std::function<void()>;

 private:
    struct TaskItem {
        TaskFunc  func;
        TimePoint execute_at;

        // Custom comparator for min-heap execution order
        bool operator>(const TaskItem& other) const { return execute_at > other.execute_at; }
    };

    std::priority_queue<TaskItem, std::vector<TaskItem>, std::greater<TaskItem>> _tasks;
    std::vector<std::thread>                                                     _workers;
    std::mutex                                                                   _mutex;
    std::condition_variable                                                      _cv;
    bool                                                                         _is_shutdown = false;

    // Main event loop runner executed by worker threads
    void worker_loop() {
        while (true) {
            TaskFunc task;
            {
                std::unique_lock<std::mutex> lock(_mutex);

                while (true) {
                    if (_is_shutdown && _tasks.empty()) {
                        return;
                    }

                    if (_tasks.empty()) {
                        _cv.wait(lock);
                    } else {
                        auto  now      = Clock::now();
                        auto& top_task = _tasks.top();

                        if (now >= top_task.execute_at) {
                            // Extract task data bypassing priority queue const restrictions safely
                            task = std::move(const_cast<TaskItem&>(top_task).func);
                            _tasks.pop();
                            break;
                        } else {
                            // Sleep until the exact deadline arrives
                            auto status = _cv.wait_until(lock, top_task.execute_at);

                            // If it wakes up because time expired, loop again to execute it instantly
                            if (status == std::cv_status::timeout) {
                                continue;
                            }
                        }
                    }
                }
            }

            // Fire task callback outside the locked area to avoid contention
            if (task) {
                task();
            }
        }
    }

 public:
    explicit TaskQueue(size_t threads = std::thread::hardware_concurrency()) {
        _workers.reserve(threads);
        for (size_t i = 0; i < threads; ++i) {
            _workers.emplace_back(&TaskQueue::worker_loop, this);
        }
    }

    ~TaskQueue() { shutdown(); }

    // Schedules a task for immediate execution
    void push(TaskFunc&& f) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_is_shutdown) return;
            _tasks.push(TaskItem{std::move(f), Clock::now()});
        }
        _cv.notify_one();
    }

    // Schedules a task with a forced execution offset
    void push_delayed(TaskFunc&& f, std::chrono::milliseconds delay) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_is_shutdown) return;
            _tasks.push(TaskItem{std::move(f), Clock::now() + delay});
        }
        _cv.notify_all();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_is_shutdown) return;
            _is_shutdown = true;
        }
        _cv.notify_all();

        for (std::thread& worker : _workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};