#include "pipe_server.h"

using namespace HarisLinux;

int main() {
    std::string request_pipe_path = "/tmp/main_request_pipe";

    std::cout << "[Main] Init thread ...\n" << std::endl;

    // 1. Trigger SERVER mode (Feedback)
    std::thread server_thread_template([&request_pipe_path]() {
        // Init Server
        Ipc::Generic server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;
        PipeServer   server(request_pipe_path, server_flags);
        server.dispatch_events();
        std::cout << "[Server] Done task. Close Server thread. \n" << std::endl;
    });
    // Wait 20ms to Server have created FIFO file before client connecting.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Wait 1 thread done
    server_thread_template.join();

    std::cout << "[Main] Test Successfully !" << std::endl;
    return 0;
}