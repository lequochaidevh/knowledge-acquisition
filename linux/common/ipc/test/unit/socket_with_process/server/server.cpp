#include "socket_server.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Instantiate Server in Feedback mode to return ACKs for loss evaluation
    SocketServer server(ServerMode::Feedback);
    int          port = 8080;

    std::cout << "[Server] Starting UDP Server on port " << port << "...\n";
    if (!server.start_server(port)) {
        std::cerr << "[Server] Failed to start server. Port might be in use.\n";
        return 1;
    }
    std::cout << "[Server] Server is running. Ready to receive data...\n";

    PacketHeader         header;
    std::vector<uint8_t> payload;

    // Endless monitoring event loop
    while (true) {
        if (server.receive_packet(header, payload)) {
            // Divert processing stream using data type classification headers
            switch (header.type) {
                case DataType::Number: {
                    if (payload.size() >= sizeof(double)) {
                        double num;
                        std::memcpy(&num, payload.data(), sizeof(double));
                        std::cout << ">> Recv NUMBER: " << num << "\n";
                    }
                    break;
                }
                case DataType::Text: {
                    std::string text(reinterpret_cast<char*>(payload.data()), payload.size());
                    std::cout << ">> Recv TEXT: " << text << "\n";
                    break;
                }
                case DataType::Command: {
                    if (payload.size() >= sizeof(SocketRequestPayload)) {
                        SocketRequestPayload cmd;
                        std::memcpy(&cmd, payload.data(), sizeof(SocketRequestPayload));
                        std::cout << ">> Recv COMMAND Code: " << cmd.command << "\n";
                    }
                    break;
                }
                case DataType::Media: {
                    std::cout << ">> Recv MEDIA: " << payload.size() << " bytes (Image/Video Frame)\n";
                    break;
                }
                case DataType::Heartbeat: {
                    std::cout << ">> Recv HEARTBEAT\n";
                    break;
                }
            }
            std::cout << "-------------------------------------------\n";
        }

        // Prevent aggressive CPU pinning when socket pipelines empty
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    return 0;
}