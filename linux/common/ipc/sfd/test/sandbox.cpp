#include "smart_file_descriptor.h"
#include <gtest/gtest.h>

// ============================================================================
// MULTI-THREADED TESTING FUNCTIONS
// ============================================================================

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <string>
#include <fstream>

// ============================================================================
// GOOGLE TEST FIXTURE SETUP
// ============================================================================

using namespace HarisLinux;

// Test fixture to manage directory setup and teardown for isolation
class FileDescriptorTest : public ::testing::Test {
 protected:
    void SetUp() override {
        // Create build directory if it doesn't exist using POSIX API
        ::mkdir("output", 0755);
    }

    void TearDown() override {
        // Optional clean up of generated test files can be put here
    }
};

// ============================================================================
// CONCURRENCY TEST WORKERS FOR GOOGLE TEST
// ============================================================================

// Worker function for SharedFileDescription concurrency testing
void shared_concurrency_worker(SharedFileDescription<FilePolicy> shared_desc, int thread_id) {
    std::string thread_identity = "[Thread_" + std::to_string(thread_id) + "]";

    for (int i = 0; i < 100; ++i) {
        // Acquire the fine-grained session lock
        auto session = shared_desc.lock();

        // Prepare two disconnected data blocks to write
        std::string part1 = thread_identity + " iter " + std::to_string(i);

        // Clean text string block without any special characters
        std::string part2 = " : " + thread_identity + " : Executing critical section uninterrupted.\n";

        // Write the first block
        session.write(part1.c_str(), part1.length());

        // Write the second block
        session.write(part2.c_str(), part2.length());
    }
}

// ============================================================================
// GOOGLE TEST CASES
// ============================================================================

// TEST 1: UniqueFileDescription Move Semantics and RAII Verification
TEST_F(FileDescriptorTest, UniqueLifecycleAndMoveSemantics) {
    UniqueFileDescription<FilePolicy> u1(LinuxArgs{}, "output/unique_test.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    EXPECT_GE(u1.get(), 0);

    // Transfer ownership via move constructor
    UniqueFileDescription<FilePolicy> u2(std::move(u1));
    EXPECT_EQ(u1.get(), -1);
    EXPECT_GE(u2.get(), 0);
}

// TEST 2: SharedFileDescription Multi-threaded I/O Lock Isolation Test
TEST_F(FileDescriptorTest, SharedMultiThreadedIOLockIsolation) {
    SharedFileDescription<FilePolicy> shared_log(LinuxArgs{}, "output/io_isolation_test.log",
                                                 O_CREAT | O_WRONLY | O_TRUNC, 0644);
    ASSERT_TRUE(shared_log);

    const int                num_threads = 8;
    std::vector<std::thread> thread_pool;

    for (int i = 1; i <= num_threads; ++i) {
        thread_pool.emplace_back(shared_concurrency_worker, shared_log, i);
    }

    for (auto& t : thread_pool) {
        t.join();
    }

    // Read the log file back to verify no data lines are torn or corrupted
    std::ifstream infile("output/io_isolation_test.log");
    std::string   line;
    while (std::getline(infile, line)) {
        if (!line.empty()) {
            // Verify that if a line starts with a Thread ID, it must execute cleanly
            EXPECT_NE(line.find("Executing critical section uninterrupted"), std::string::npos);
        }
    }
}

void registry_stress_worker(SharedFileDescription<FilePolicy>& shared_desc, int thread_id) {
    std::string thread_identity = "[Thread_" + std::to_string(thread_id) + "]";
    for (int i = 0; i < 500; ++i) {
        // Rapidly create and destroy copies to trigger heavy CAS loop contention in the registry
        SharedFileDescription<FilePolicy> temporary_copy = shared_desc;

        auto        session = shared_desc.lock();
        std::string context = thread_identity + " : New shared fd access this file "  //
                              + "- with i = "                                         //
                              + std::to_string(i) + " with use_count = "              //
                              + std::to_string(temporary_copy.use_count()) + "\n";

        // Write the first block
        session.write(context.c_str(), context.length());

        assert(temporary_copy.use_count() > 1);
    }
}

// TEST 3: Static Registry Reference Counting High Contention CAS Test
TEST_F(FileDescriptorTest, StaticRegistryLockFreeHighContentionCAS) {
    SharedFileDescription<FilePolicy> shared_stress(LinuxArgs{}, "output/registry_stress.log",
                                                    O_CREAT | O_WRONLY | O_TRUNC, 0644);
    size_t                            initial_count = shared_stress.use_count();
    EXPECT_EQ(initial_count, 1);

    const int                num_threads = 12;
    std::vector<std::thread> thread_pool;

    for (int i = 0; i < num_threads; ++i) {
        thread_pool.emplace_back(registry_stress_worker, std::ref(shared_stress), i);
    }

    for (auto& t : thread_pool) {
        t.join();
    }

    // Verify that after all threads exit, the reference count converges perfectly back to 1
    size_t final_count = shared_stress.use_count();
    EXPECT_EQ(final_count, 1);
}

// TEST 4: Unique-to-Shared Conversion Verification
TEST_F(FileDescriptorTest, UniqueToSharedConversionLifecycle) {
    UniqueFileDescription<FilePolicy> unique_source(LinuxArgs{}, "output/conversion_test.txt",
                                                    O_CREAT | O_WRONLY | O_TRUNC, 0644);
    int                               native_fd = unique_source.get();

    // Convert by moving Unique into Shared constructor
    SharedFileDescription<FilePolicy> shared_dest(std::move(unique_source));

    EXPECT_EQ(unique_source.get(), -1);
    EXPECT_EQ(shared_dest.get(), native_fd);
    EXPECT_EQ(shared_dest.use_count(), 1);
}

// TEST 1: Unix Domain Socket Client-Server Echo Data Verification
TEST_F(FileDescriptorTest, UnixDomainSocketTransmission) {
    UnixDomainStreamContext ctx{"output/test_unix.sock"};

    SharedFileDescription<UnixDomainStreamPolicy> server_sock(LinuxArgs{}, ctx, AF_UNIX, SOCK_STREAM, 0);
    ASSERT_TRUE(server_sock);

    struct sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, ctx.path.c_str(), sizeof(addr.sun_path) - 1);

    int bind_res = ::bind(server_sock.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    ASSERT_GE(bind_res, 0);
    ASSERT_GE(::listen(server_sock.get(), 5), 0);

    // Spawn Client thread: Use open_client to connect without corrupting server path node
    std::thread client_thread([&ctx]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        int client_raw_fd = UnixDomainStreamPolicy::open_transmitter(AF_UNIX, SOCK_STREAM, 0);
        if (client_raw_fd < 0) return;

        SharedFileDescription<UnixDomainStreamPolicy> client_sock(client_raw_fd);
        struct sockaddr_un                            client_addr {};
        client_addr.sun_family = AF_UNIX;
        std::strncpy(client_addr.sun_path, ctx.path.c_str(), sizeof(client_addr.sun_path) - 1);

        int connect_res =
            ::connect(client_sock.get(), reinterpret_cast<struct sockaddr*>(&client_addr), sizeof(client_addr));
        if (connect_res >= 0) {
            auto        session = client_sock.lock();
            std::string payload = "PING";
            session.write(payload.c_str(), payload.length());
        }
    });

    // Server Accept block: Will now instantly wake up as client connection succeeds
    int client_fd = ::accept(server_sock.get(), nullptr, nullptr);
    ASSERT_GE(client_fd, 0);

    SharedFileDescription<UnixDomainStreamPolicy> accepted_session(client_fd);
    {
        auto    session         = accepted_session.lock();
        char    read_buffer[10] = {0};  // Allocating buffer cleanly on stack
        ssize_t bytes_received  = session.read(read_buffer, sizeof(read_buffer) - 1);

        EXPECT_GT(bytes_received, 0);
        EXPECT_STREQ(read_buffer, "PING");
    }

    client_thread.join();

    // Trigger RAII destruction to clean up files completely
    server_sock.reset();
    struct stat st {};
    EXPECT_NE(::stat(ctx.path.c_str(), &st), 0);  // File node must be unlinked from filesystem
}

// TEST 2: Named Pipe (FIFO) Unidirectional Stream Verification
TEST_F(FileDescriptorTest, NamedPipeFifoTransmission) {
    PipeContext ctx;
    std::string file_path{"output/test_fifo.fifo"};
    uint16_t    flag = 0;

    // Step 1: Initialize the FIFO file layout safely on the disk
    ASSERT_TRUE(PipePolicy::open_and_declare_ctx(ctx, file_path, flag));

    // Step 2: Spawn the writer thread FIRST. It opens the write channel in blocking mode.
    // It will safely pause inside kernel space until the reader side joins the connection.
    std::thread writer_thread([&ctx]() {
        int write_fd = PipePolicy::open_channel(ctx, O_WRONLY);
        if (write_fd < 0) return;

        SharedFileDescription<PipePolicy> write_desc(write_fd);
        auto                              session = write_desc.lock();

        std::string message = "HELLO_FIFO";
        session.write(message.c_str(), message.length());
    });

    // Step 3: Open the read channel on the main thread in standard blocking mode.
    // This instantly unblocks both the writer's open and this reader's open simultaneously.
    int read_fd = PipePolicy::open_channel(ctx, O_RDONLY);
    ASSERT_GE(read_fd, 0);

    SharedFileDescription<PipePolicy> read_desc(read_fd);
    ASSERT_TRUE(read_desc);

    // Step 4: Read the data cleanly via the isolated read descriptor guard
    char read_buffer[32] = {0};  // Allocating array buffer cleanly on stack
    bool data_verified   = false;

    auto    session    = read_desc.lock();
    ssize_t bytes_read = session.read(read_buffer, sizeof(read_buffer) - 1);

    if (bytes_read > 0) {
        EXPECT_STREQ(read_buffer, "HELLO_FIFO");
        data_verified = true;
    }

    EXPECT_TRUE(data_verified);
    writer_thread.join();
}

// TEST 3: EventFd Signal Counter Notification Verification
TEST_F(FileDescriptorTest, EventFdSignalingAndNotification) {
    // Instantiate lightweight signaling system using compile-time flags
    SharedFileDescription<EventFdPolicy> notifier(LinuxArgs{}, 0, EFD_CLOEXEC | EFD_NONBLOCK);
    ASSERT_TRUE(notifier);

    // Write a trigger event value of 5 to the system layer counter
    {
        auto     session        = notifier.lock();
        uint64_t trigger_signal = 5;
        ssize_t  bytes_written  = EventFdPolicy::notify(session.get_fd(), trigger_signal);
        EXPECT_EQ(bytes_written, sizeof(uint64_t));
    }

    // Read the event state back to verify precise data signaling integration
    {
        auto     session         = notifier.lock();
        uint64_t captured_signal = 0;
        ssize_t  bytes_read      = EventFdPolicy::fetch_signal(session.get_fd(), captured_signal);

        EXPECT_EQ(bytes_read, sizeof(uint64_t));
        EXPECT_EQ(captured_signal, 5);  // Kernel counter aggregation check
    }
}

// TEST 4: Epoll Interface Multiplexing Event Listening Verification
TEST_F(FileDescriptorTest, EpollEventMultiplexing) {
    // Create high-performance asynchronous tracking instance via EpollPolicy
    SharedFileDescription<EpollPolicy> epoll_manager(LinuxArgs{}, EPOLL_CLOEXEC);
    ASSERT_TRUE(epoll_manager);

    // Build an EventFd device to register into our asynchronous epoll matrix
    SharedFileDescription<EventFdPolicy> target_device(LinuxArgs{}, 0, EFD_CLOEXEC | EFD_NONBLOCK);
    ASSERT_TRUE(target_device);

    // Configure tracking matrix to listen exclusively for inbound read triggers
    struct epoll_event ev {};
    ev.events  = EPOLLIN;
    ev.data.fd = target_device.get();

    int ctl_res = EpollPolicy::control(epoll_manager.get(), EPOLL_CTL_ADD, target_device.get(), &ev);
    ASSERT_EQ(ctl_res, 0);

    // Trigger target signaling data frame
    {
        auto     session   = target_device.lock();
        uint64_t write_val = 1;
        EventFdPolicy::notify(session.get_fd(), write_val);
    }

    // Await system notification multiplexer updates using wait API
    struct epoll_event active_events[1];
    int                events_fired = EpollPolicy::wait(epoll_manager.get(), active_events, 1, 100);

    // Verify that the event engine picked up exactly 1 registered IO notification
    ASSERT_EQ(events_fired, 1);
    EXPECT_EQ(active_events[0].data.fd, target_device.get());
    EXPECT_TRUE(active_events[0].events & EPOLLIN);
}

// TEST 5: UDP Localhost Client-Server Datagram Verification
TEST_F(FileDescriptorTest, UdpLocalhostDatagramTransmission) {
    // Step 1: Initialize Server-side UDP Smart FileDescription

    std::string ip_address{"127.0.0.1"};
    uint16_t    port{9999};

    // Bind the server socket to port 9999 on Localhost
    struct sockaddr_in server_addr {};
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip_address.c_str());

    SharedFileDescription<UdpLocalhostPolicy> server_udp(LinuxArgs{}, ip_address, port);
    ASSERT_TRUE(server_udp);

    int bind_res = ::bind(server_udp.get(), reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr));
    ASSERT_GE(bind_res, 0);

    // Step 2: Spawn a Client thread to fire a UDP datagram packet
    std::thread client_thread([&ip_address, port]() {
        // Sleep briefly to make sure server socket is active in kernel space
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Client Side initialization
        SharedFileDescription<UdpLocalhostPolicy> client_udp(LinuxArgs{}, ip_address, port);
        if (client_udp) {
            auto        session  = client_udp.lock();
            std::string datagram = "UDP_PING";
            // Policy automatically routes this write call via sendto() to 127.0.0.1:9999
            session.write(datagram.c_str(), datagram.length());
        }
    });

    // Step 3: Server Side awaits packet arrival via blocking read
    char read_buffer[64] = {0};  // Safe stack array buffer allocation
    bool data_verified   = false;

    auto    session        = server_udp.lock();
    ssize_t bytes_received = session.read(read_buffer, sizeof(read_buffer) - 1);

    if (bytes_received > 0) {
        EXPECT_STREQ(read_buffer, "UDP_PING");
        data_verified = true;
    }

    EXPECT_TRUE(data_verified);
    client_thread.join();
}

// ============================================================================
// GOOGLE TEST MAIN ENTRY POINT
// ============================================================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
