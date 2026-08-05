#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <utility>
#include <string>
#include <asio.hpp>

using asio::ip::tcp;

// Represents an isolated active client session.
// Managed via shared_ptr to ensure lifespan across async event boundaries.
class Session : public std::enable_shared_from_this<Session> {
 public:
    // Bind the socket explicitly to an io_context execution strand.
    // The strand guarantees callback handlers for this session execute sequentially.
    explicit Session(tcp::socket socket) : socket_(std::move(socket)), strand_(socket_.get_executor()) {}

    void start() {
        try {
            // Resolve remote client details for tracking and logging
            client_address_ = socket_.remote_endpoint().address().to_string() + ":" +
                              std::to_string(socket_.remote_endpoint().port());

            std::cout << "[Thread " << std::this_thread::get_id() << "] Session started for client: " << client_address_
                      << std::endl;
        } catch (...) {
        }

        do_read();
    }

 private:
    void do_read() {
        auto self(shared_from_this());
        // Read data asynchronously. The call returns instantly.
        // asio::bind_executor ensures the callback executes safely inside the strand.
        socket_.async_read_some(asio::buffer(data_, max_length),
                                asio::bind_executor(strand_, [this, self](std::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        std::cout << "[Thread " << std::this_thread::get_id() << "] Received from "
                                                  << client_address_ << ": " << std::string(data_, length);
                                        if (data_[length - 1] != '\n') std::cout << std::endl;

                                        do_write(length);
                                    } else {
                                        std::cout << "[Thread " << std::this_thread::get_id()
                                                  << "] Client disconnected: " << client_address_ << std::endl;
                                    }
                                }));
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());
        // Echo back data asynchronously.
        // Eliminates data-race conditions on multi-threaded event loops using the strand.
        asio::async_write(socket_, asio::buffer(data_, length),
                          asio::bind_executor(strand_, [this, self](std::error_code ec, std::size_t /*length*/) {
                              if (!ec) {
                                  // Continue waiting for subsequent inputs from this client.
                                  do_read();
                              }
                          }));
    }

    tcp::socket                              socket_;
    asio::strand<tcp::socket::executor_type> strand_;
    static constexpr size_t                  max_length = 4096;  // 4KB optimized block
    char                                     data_[max_length];
    std::string                              client_address_;
};

// Orchestrates high-volume client connection requests.
class EnterpriseServer {
 public:
    EnterpriseServer(asio::io_context& io_context, short port) : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

 private:
    void do_accept() {
        // Non-blocking asynchronous accept loop.
        acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
            if (!ec) {
                // Instantly spin up a session for the client and delegate execution.
                std::make_shared<Session>(std::move(socket))->start();
            }
            // Immediately resume accepting next concurrent connections.
            do_accept();
        });
    }

    tcp::acceptor acceptor_;
};

int main() {
    try {
        // Core management object handling system demultiplexers (epoll/IOCP/kqueue)
        asio::io_context io_context;
        unsigned short   port = 8080;

        std::cout << "Enterprise Echo Server Starting on Port " << port << "\n";
        EnterpriseServer server(io_context, port);

        // Scaling Engine: Build a Work Thread Pool matching hardware capabilities
        unsigned int thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 2;  // Fallback

        std::vector<std::thread> thread_pool;
        for (unsigned int i = 0; i < thread_count; ++i) {
            thread_pool.emplace_back([&io_context]() {
                // Multi-threaded consumers concurrent polling on the same io_context queue
                io_context.run();
            });
        }

        std::cout << "[Server] Active and listening! Open a second terminal to connect.\n";
        std::cout << "[Test Command]: nc localhost " << port << "\n";

        // Join threads back to prevent application termination
        for (auto& t : thread_pool) {
            if (t.joinable()) t.join();
        }
    } catch (const std::exception& e) {
        std::cerr << "[Critical Exception]: " << e.what() << "\n";
    }
    return 0;
}