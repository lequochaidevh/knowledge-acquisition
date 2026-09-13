
#include "common/task_queue.h"

void TaskQueue::worker_loop() {
    // Standard execution limit threshold (e.g., 100 milliseconds)
    // Any task taking longer than this is considered an execution overflow
    const auto execution_threshold = std::chrono::milliseconds(100);
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
        // Execute task outside the lock area and detect execution overflow
        if (task) {
            auto start_time = Clock::now();

            // Fire the actual callback
            task();

            auto end_time           = Clock::now();
            auto execution_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            // Trigger alert if the callback overshot the latency threshold
            if (execution_duration > execution_threshold) {
                std::cerr << "[TASK OVERFLOW DETECTED] A callback stalled worker thread [" << std::this_thread::get_id()
                          << "] for " << execution_duration.count()
                          << " ms! (Threshold: " << execution_threshold.count() << " ms)\n";
            }
        }
    }
}

TaskQueue::TaskQueue(size_t threads) {
    _workers.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        _workers.emplace_back(&TaskQueue::worker_loop, this);
    }
}

TaskQueue::~TaskQueue() { shutdown(); }

void TaskQueue::push(TaskFunc&& f) {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_is_shutdown) return;
        _tasks.push(TaskItem{std::move(f), Clock::now()});
    }
    _cv.notify_one();
}

void TaskQueue::push_delayed(TaskFunc&& f, std::chrono::milliseconds delay) {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_is_shutdown) return;
        _tasks.push(TaskItem{std::move(f), Clock::now() + delay});
    }
    _cv.notify_all();
}

void TaskQueue::shutdown() {
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
