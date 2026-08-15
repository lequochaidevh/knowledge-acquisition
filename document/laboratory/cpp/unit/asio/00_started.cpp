#include <asio.hpp>
#include <iostream>
#include <chrono>

int main() {
    asio::io_context io;

    // Counter variable
    int count = 0;

    // Create a timer
    asio::steady_timer t(io, std::chrono::seconds(1));

#if 0
// old:
for (int i = 0; i < 5; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Thread freeze
    std::cout << "Ping!\n";
}
#endif

    // Define a recursive lambda function for repeating the timer
    std::function<void(const asio::error_code&)> handler = [&](const asio::error_code& /*e*/) {
        if (count < 5) {
            std::cout << "Count: " << count << "\n";
            count++;

            // Reset the timer expiration time for another 1 second
            t.expires_at(t.expiry() + std::chrono::seconds(1));

            // Re-register the callback
            t.async_wait(handler);
        }
    };

    // Start the first async wait
    t.async_wait(handler);

    io.run();

    std::cout << "Finished! Total counts: " << count << "\n";

    return 0;
}