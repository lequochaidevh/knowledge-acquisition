#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::atomic<bool> isRunning(true);
int               recvSocket = -1;
int               sendSocket = -1;

// Thread function to handle receiving data (DGRAM mode)
void ReceiveData() {
    char        buffer[1024];
    sockaddr_in remoteAddr;
    socklen_t   remoteAddrLen = sizeof(remoteAddr);

    std::cout << "[Receiver] Thread started. Listening on port 5000...\n";

    while (isRunning) {
        // Blocks the thread until a discrete datagram packet arrives from any sender
        ssize_t bytesReceived =
            recvfrom(recvSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&remoteAddr, &remoteAddrLen);

        if (bytesReceived < 0) {
            // Triggered when shutdown() is called from the main thread to unblock recvfrom()
            std::cout << "[Receiver] Socket closed or error occurred. Exiting thread.\n";
            break;
        }

        buffer[bytesReceived] = '\0';  // Append null-terminator to safely treat data as a string
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &remoteAddr.sin_addr, ipStr, INET_ADDRSTRLEN);

        std::cout << "[Receiver] Got from " << ipStr << ":" << ntohs(remoteAddr.sin_port) << " -> " << buffer << "\n";
    }
}

// Thread function to handle sending data (DGRAM mode)
void SendData() {
    sockaddr_in targetAddr;
    std::memset(&targetAddr, 0, sizeof(targetAddr));
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port   = htons(5000);  // Destination port is 5000
    // inet_pton(AF_INET, "127.0.0.1", &targetAddr.sin_addr);  // Target address is localhost
    targetAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    std::cout << "[Sender] Thread started.\n";
    int count = 1;

    while (isRunning) {
        std::string message = "Linux Datagram packet number " + std::to_string(count++);

        // Sends an independent, individual packet to the explicitly specified target address
        ssize_t bytesSent =
            sendto(sendSocket, message.c_str(), message.length(), 0, (sockaddr*)&targetAddr, sizeof(targetAddr));

        if (bytesSent < 0) {
            std::cout << "[Sender] Send operation failed or socket was closed.\n";
            break;
        }

        std::cout << "[Sender] Sent packet: " << message << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Throttle transmission loop
    }
}

int main() {
    // === INIT STATE ===
    // Step 1: Create and initialize the receiving socket (SOCK_DGRAM specifies UDP)
    recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in recvAddr;
    std::memset(&recvAddr, 0, sizeof(recvAddr));
    recvAddr.sin_family      = AF_INET;
    recvAddr.sin_port        = htons(5000);  // Bind the socket to port 5000
    recvAddr.sin_addr.s_addr = INADDR_ANY;   // Accept packets sent to any local interface
    bind(recvSocket, (sockaddr*)&recvAddr, sizeof(recvAddr));

    // Step 2: Create the sending socket (OS automatically assigns a random ephemeral port)
    sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    int broadcastEnable = 1;
    int ret             = setsockopt(sendSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    if (ret < 0) {
        std::cerr << "[Initialization Error] Failed to enable broadcast permission.\n";
        close(sendSocket);
        close(recvSocket);
        return 1;
    }

    // Step 3: Spawn independent concurrent threads for transmission and reception
    std::thread tReceive(ReceiveData);
    std::thread tSend(SendData);

    std::cout << "Press ENTER to stop the program...\n";
    std::cin.get();

    // === CLOSE STATE ===
    isRunning = false;

    // Step 1: Forcefully interrupt blocking network operations (like recvfrom) immediately
    shutdown(recvSocket, SHUT_RDWR);
    shutdown(sendSocket, SHUT_RDWR);

    // Step 2: Close file descriptors to return system resource handles back to Linux OS
    close(recvSocket);
    close(sendSocket);

    // Step 3: Wait for threads to complete execution gracefully and synchronize with main thread
    if (tReceive.joinable()) tReceive.join();
    if (tSend.joinable()) tSend.join();

    std::cout << "System closed cleanly.\n";
    return 0;
}
