#include <thread>
#include <chrono>
#include <iostream>

#include <unistd.h>    // Required for fork()
#include <sys/wait.h>  // Required for waitpid()

void runModuleA();
void runModuleB();

int main() {
    runModuleA();
    runModuleB();

    // 2. Spawn a new worker thread to run Module B continuously (will show [tid: ...])
    std::thread worker_thread([]() {
        while (true) {
            runModuleB();

            // Sleep for exactly 700 milliseconds using C++ chrono literals
            std::this_thread::sleep_for(std::chrono::milliseconds(1400));
        }
    });

    // 2. Fork the process to create a completely new child process
    pid_t child_pid = ::fork();

    if (child_pid < 0) {
        std::cerr << "Failed to fork process!\n";
        return 1;
    }

    if (child_pid == 0) {
        // === CHILD PROCESS CODE BLOCK ===
        // This block runs only inside the newly created child process
        // It will print log with its own unique [pid: ...]
        while (true) {
            runModuleA();  // Let child process execute Module A logic
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
        }
        return 0;  // Child process exits here (unreachable due to loop)
    }

    // 3. Keep the Main Thread alive so the background thread can continue running
    // In a real application, this could be your main event loop (e.g., epoll, IPC select)
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Detach or join the thread (unreachable here due to infinite loop, but good practice)
    if (worker_thread.joinable()) {
        worker_thread.join();
    }

    return 0;
}
