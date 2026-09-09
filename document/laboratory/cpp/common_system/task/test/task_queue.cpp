#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "../TaskQueue.h"

using namespace std::chrono_literals;

// Helper to print logs with timestamp to verify timing precision
void log_message(const std::string& msg) {
    auto now        = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()) %
        1000;

    // Format: [SS:ms] Message
    std::cout << "[" << (time_t_now % 60) << ":" << ms.count() << "] " << msg << std::endl;
}

int main() {
    log_message("[Main] Starting intense TaskQueue Testing...");

    // Initialize TaskQueue with 3 parallel worker threads
    TaskQueue event_loop(3);

    // =========================================================================
    // TEST CASE 1: Chronological Ordering
    // Push tasks out of order, verify if workers execute them chronologically.
    // =========================================================================
    log_message("[Test 1] Enqueueing tasks with different delays out of order...");

    event_loop.push_delayed([]() { log_message("  -> [Task C] Should run last (at +1500ms)"); }, 1500ms);
    event_loop.push_delayed([]() { log_message("  -> [Task A] Should run first (at +200ms)"); }, 200ms);
    event_loop.push_delayed([]() { log_message("  -> [Task B] Should run second (at +800ms)"); }, 800ms);

    // =========================================================================
    // TEST CASE 2: Thread Interruption / Preemption
    // =========================================================================
    std::this_thread::sleep_for(400ms);
    log_message("[Test 2] Pushing an EMERGENCY task while workers are sleeping for Task B/C...");

    event_loop.push(
        []() { log_message("  ⚡ [EMERGENCY TASK] Executed immediately! Interrupted the sleep loop successfully."); });

    // =========================================================================
    // TEST CASE 3: Stress Test & Concurrency
    // Create 5 external threads simultaneously throwing 20 tasks to check for race conditions.
    // =========================================================================
    std::this_thread::sleep_for(1500ms);  // Chờ các test trước xả hết dữ liệu
    log_message("[Test 3] Starting Stress Test with multiple producer threads...");

    std::vector<std::thread> producers;
    std::atomic<int>         completed_tasks{0};

    for (int i = 0; i < 5; ++i) {
        producers.emplace_back([&event_loop, i, &completed_tasks]() {
            for (int j = 0; j < 4; ++j) {
                event_loop.push([i, j, &completed_tasks]() {
                    std::this_thread::sleep_for(50ms);
                    completed_tasks++;
                });
            }
        });
    }

    for (auto& p : producers) {
        p.join();
    }
    log_message("[Test 3] All 20 heavy tasks enqueued. Waiting for completion...");

    // (~ 50ms * 20 / 3 thread = ~350ms)
    std::this_thread::sleep_for(500ms);
    log_message("[Test 3] Stress test finished. Total tasks completed: " + std::to_string(completed_tasks.load()));

    log_message("[Main] Shutting down TaskQueue.");
    event_loop.shutdown();

    return 0;
}