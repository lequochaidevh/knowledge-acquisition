#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <atomic>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../UdpTransport.h"
#include "../TaskQueue.h"
#include "../ComLink.h"
#include "../UdpTransport.h"

/**
 * [Task 1 (Parent Process)]
       │ (Sends bytes via pipe)
       ▼
[Task 2 (Child Process)]
   ┌───┴────────────────────────────────────────┐
   │ 1. Read Loop Thread: Reads raw bytes       │
   │                      from Pipe             │
   └───┬────────────────────────────────────────┘
       │ (Pushes lambda task)
       ▼
   ┌────────────────────────────────────────────┐
   │ 2. TaskQueue (Worker Threads Pool)         │
   │    ⚡ Thread 1: Parses byte ➔ Yields Packet│
   │    ⚡ Thread 2: Dispatches to subscribers  │
   └────────────────────────────────────────────┘
 */

// Utility function to simulate Task 1 writing a packet into the pipe channel
// Sends data in the frame format: [0xAA] [SystemID] [MsgID_H] [MsgID_L] [Length] [Payload] [Checksum]
void execute_task_1_producer(int write_file_descriptor) {
    std::cout << "[Task 1 (Parent)] Starting transmission engine...\n";

    // Craft a dummy custom telemetry packet
    Packet packet;
    packet.system_id = 5;
    packet.msg_id    = 1001;
    packet.payload   = {0xDE, 0xAD, 0xBE, 0xEF};  // 4 bytes of data
    packet.checksum  = 0xFF;

    // Serialize packet into a raw byte stream matching ComlinkParser rules
    std::vector<uint8_t> frame;
    frame.reserve(6 + packet.payload.size());
    frame.push_back(0xAA);
    frame.push_back(packet.system_id);
    frame.push_back(static_cast<uint8_t>((packet.msg_id >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(packet.msg_id & 0xFF));
    frame.push_back(static_cast<uint8_t>(packet.payload.size()));
    frame.insert(frame.end(), packet.payload.begin(), packet.payload.end());
    frame.push_back(static_cast<uint8_t>(packet.checksum & 0xFF));

    // Simulate network/latency delay before dispatching bytes
    usleep(9000000);

    std::cout << "[Task 1 (Parent)] Sending serialized frame over IPC pipe stream...\n";

    // Write raw bytes directly into the IPC pipe file descriptor
    ssize_t bytes_written = write(write_file_descriptor, frame.data(), frame.size());
    if (bytes_written < 0) {
        std::cerr << "[Task 1 (Parent)] Write failure to IPC channel.\n";
    }

    // Always close the descriptor when finished to signal End-Of-File (EOF) to the reader
    close(write_file_descriptor);
    std::cout << "[Task 1 (Parent)] Pipeline closed. Exiting process scope.\n";
}

using Clock = std::chrono::steady_clock;

// 1. Change declaration to a lock-free primitive integer type
// Unify to primitive atomic integer type to fix compiler error
std::atomic<uint64_t> _last_received_time_ms{0};
std::atomic<bool>     _is_timeout_triggered{false};

// Helper helper utility to convert current time to raw milliseconds
// Hàm helper chuyển đổi thời gian hiện tại thành số miligiây thô
uint64_t get_current_time_ms() {
    auto now = Clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

// High-level application subscriber callback
void handle_incoming_custom_packet(const Packet& packet) {
    std::cout << "[Subscriber] Valid frame intercepted. Resetting watchdog timer clock.\n";
}

// Asynchronous Watchdog Task that monitors system health periodically
// Asynchronous Watchdog Task monitoring system health periodically
void schedule_watchdog_monitor(TaskQueue& pool) {
    uint64_t now_ms                             = get_current_time_ms();
    uint64_t last_ms                            = _last_received_time_ms.load();
    uint64_t duration_since_last_packet_seconds = (now_ms - last_ms) / 1000;

    if (duration_since_last_packet_seconds >= 3) {
        // Prevent spamming the console with redundant alerts
        std::cerr << "[WATCHDOG ALERT] FAILSAFE TRIGGERED! No data from Parent for "
                  << duration_since_last_packet_seconds << " seconds!\n";
        _is_timeout_triggered.store(true);
    } else {
        if (_is_timeout_triggered.load()) {
            std::cout << "[WATCHDOG] Connection restored cleanly with Parent.\n";
            _is_timeout_triggered.store(false);
        }
    }

    // Reschedule the watchdog task in 1000 milliseconds
    pool.push_delayed([&pool]() { schedule_watchdog_monitor(pool); }, std::chrono::milliseconds(2000));
}

void execute_task_2_consumer(int read_file_descriptor) {
    std::cout << "[Task 2 (Child)] Initializing Parallel Engine...\n";

    TaskQueue         worker_pool(3);
    ComlinkDispatcher dispatcher;

    dispatcher.subscribe(1001, handle_incoming_custom_packet);
    auto parser = std::make_shared<ComlinkParser>();

    // Seed initial startup baseline clock time
    _last_received_time_ms.store(get_current_time_ms());

    // Deploy asynchronous 3-second Failsafe Watchdog loop
    std::cout << "[Task 2 (Child)] Deploying asynchronous 3-second Failsafe Watchdog...\n";
    worker_pool.push_delayed([&worker_pool]() { schedule_watchdog_monitor(worker_pool); },
                             std::chrono::milliseconds(1000));

    uint8_t buffer;
    ssize_t bytes_read = 0;

    std::cout << "[Task 2 (Child)] Listening to IPC channel stream...\n";

    // Main data polling loop reading from the POSIX pipe descriptor
    while ((bytes_read = read(read_file_descriptor, &buffer, sizeof(buffer))) > 0) {
        std::vector<uint8_t> memory_chunk(&buffer, &buffer + bytes_read);

        worker_pool.push([bytes = std::move(memory_chunk), parser, &dispatcher]() {
            for (uint8_t byte : bytes) {
                if (auto packet_optional = parser->parse_byte(byte); packet_optional.has_value()) {
                    // Reset the unified atomic ms timestamp upon package arrival
                    _last_received_time_ms.store(get_current_time_ms());

                    dispatcher.dispatch(packet_optional.value());
                }
            }
        });
    }

    close(read_file_descriptor);
    std::cout << "[Task 2 (Child)] Pipeline EOF hit. Shutting down worker pool...\n";
    worker_pool.shutdown();
    std::cout << "[Task 2 (Child)] Core engine exited cleanly.\n";
}

int test_3_2() {
    // Array to hold the pipe file descriptors: indices [0] = Read, [1] = Write
    int pipe_file_descriptors[2];

    // Create the POSIX pipe system resource before forking
    if (pipe(pipe_file_descriptors) == -1) {
        std::cerr << "[Main] Pipeline initialization failed.\n";
        return 1;
    }

    // Fork the process into Parent and Child
    pid_t process_identifier = fork();

    if (process_identifier < 0) {
        std::cerr << "[Main] Fork operation failed.\n";
        return 1;
    }

    if (process_identifier == 0) {
        // ---------------------------------------------------------------------
        // CHILD PROCESS SCOPE (Task 2)
        // ---------------------------------------------------------------------
        // Child only reads, so close the unused write end of the pipe immediately
        close(pipe_file_descriptors[1]);

        execute_task_2_consumer(pipe_file_descriptors[0]);
        return 0;  // Exit child cleanly
    } else {
        // ---------------------------------------------------------------------
        // PARENT PROCESS SCOPE (Task 1)
        // ---------------------------------------------------------------------
        // Parent only writes, so close the unused read end of the pipe immediately
        close(pipe_file_descriptors[0]);

        execute_task_1_producer(pipe_file_descriptors[1]);

        // Wait for the child process to complete execution to prevent zombie processes
        int status;
        waitpid(process_identifier, &status, 0);
        std::cout << "[Main] Child process reaped successfully. Main process terminating.\n";
    }

    return 0;
}