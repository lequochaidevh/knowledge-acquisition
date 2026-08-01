#include <asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

// Class managing an asynchronous HTTP read session
class WebReader : public std::enable_shared_from_this<WebReader> {
 public:
    // Constructor initializes the socket and sets the dynamic buffer size
    WebReader(asio::io_context& io, const std::string& host, size_t buffer_size)
        : resolver_(io), socket_(io), host_(host) {
        // Configure buffer size from vector size as requested (e.g., 512, 1024, or more)
        buffer_.resize(buffer_size);
    }

    // Starts the asynchronous chain operation
    void start() {
        // Resolve the website hostname to an IP address asynchronously
        resolver_.async_resolve(
            host_, "80",
            [self = shared_from_this()](const asio::error_code& ec, asio::ip::tcp::resolver::results_type results) {
                if (!ec) {
                    self->connect(results);
                } else {
                    std::cerr << "Resolve failed: " << ec.message() << "\n";
                }
            });
    }

 private:
    // Step 2: Connect to the resolved endpoints asynchronously
    void connect(const asio::ip::tcp::resolver::results_type& results) {
        asio::async_connect(
            socket_, results,
            [self = shared_from_this()](const asio::error_code& ec, const asio::ip::tcp::endpoint& /*endpoint*/) {
                if (!ec) {
                    std::cout << "Connected successfully to " << self->host_ << "\n";
                    self->send_request();
                } else {
                    std::cerr << "Connect failed: " << ec.message() << "\n";
                }
            });
    }

    // Step 3: Send a standard HTTP GET request
    void send_request() {
        // Construct a raw HTTP request payload
        request_ =
            "GET / HTTP/1.1\r\n"
            "Host: " +
            host_ +
            "\r\n"
            "User-Agent: AsioWebReader/1.0\r\n"
            "Connection: close\r\n\r\n";

        asio::async_write(socket_, asio::buffer(request_),
                          [self = shared_from_this()](const asio::error_code& ec, size_t /*bytes_transferred*/) {
                              if (!ec) {
                                  std::cout << "Request sent. Starting asynchronous read loop...\n";
                                  std::cout << "--------------------------------------------------\n";
                                  self->read_loop();
                              } else {
                                  std::cerr << "Write failed: " << ec.message() << "\n";
                              }
                          });
    }

    // Step 4: The Core Read Loop (Demonstrating Asio's power)
    void read_loop() {
        // Read data directly into the pre-allocated std::vector memory area
        // Max chunk size is governed perfectly by buffer_.size() (e.g., 512 or 1024)
        socket_.async_read_some(
            asio::buffer(buffer_), [self = shared_from_this()](const asio::error_code& ec, size_t bytes_transferred) {
                if (!ec) {
                    // Process the chunk from the vector buffer
                    std::string chunk(self->buffer_.data(), bytes_transferred);
                    std::cout << "\n\n\n[Read " << bytes_transferred << " bytes]:\n\n\n" << chunk << "\n";

                    // CRITICAL: Call itself recursively to fetch the next data chunk
                    self->read_loop();
                }
                // asio::error::eof means the server closed the connection normally after sending all data
                else if (ec == asio::error::eof) {
                    std::cout << "--------------------------------------------------\n";
                    std::cout << "Connection closed by server. All data received successfully.\n";
                } else {
                    std::cerr << "Read error: " << ec.message() << "\n";
                }
            });
    }

    asio::ip::tcp::resolver resolver_;
    asio::ip::tcp::socket   socket_;
    std::string             host_;
    std::string             request_;
    std::vector<char>       buffer_;  // Dynamic buffer configured by the user
};

int main() {
    try {
        asio::io_context io;

        // Configuration: Target host
        std::string target_host = "google.com";

        // Configuration: Set chunk buffer size here (e.g., 512, 1024, 4096)
        size_t config_buffer_size = 512;

        std::cout << "Initializing WebReader for " << target_host << " with chunk size: " << config_buffer_size
                  << " bytes\n";

        // Create the session instance using shared_ptr for safe async lifecycle control
        auto session = std::make_shared<WebReader>(io, target_host, config_buffer_size);

        // Fire up the asynchronous chain
        session->start();

        // The main thread hands over control to the OS network kernel (epoll/kqueue)
        io.run();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
