#pragma once
#include "std17pch.h"

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
    void worker_loop();

 public:
    explicit TaskQueue(size_t threads = std::thread::hardware_concurrency());

    ~TaskQueue();

    // Schedules a task for immediate execution
    void push(TaskFunc&& f);

    // Schedules a task with a forced execution offset
    void push_delayed(TaskFunc&& f, std::chrono::milliseconds delay);

    void shutdown();
};