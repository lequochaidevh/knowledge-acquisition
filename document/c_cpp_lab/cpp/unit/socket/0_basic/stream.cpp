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
int               streamRecvSocket = -1;
int               streamSendSocket = -1;

// Thread function to handle stream-like continuous data reception
void ReceiveStreamData() {
    char buffer[1024];
    std::cout << "[Connected-Receiver] Waiting for continuous data stream...\n";

    while (isRunning) {
        // Reads data arriving sequentially from the connected peer only (no address tracking required)
        ssize_t bytesReceived = recv(streamRecvSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived < 0) {
            std::cout << "[Connected-Receiver] Stream disconnected or socket closed.\n";
            break;
        }

        buffer[bytesReceived] = '\0';
        std::cout << "[Connected-Receiver] Stream fragment: " << buffer << "\n";
    }
}

// Thread function to handle stream-like continuous data transmission
void SendStreamData() {
    std::cout << "[Connected-Sender] Stream transmission started.\n";
    int count = 1;

    while (isRunning) {
        std::string message = "Continuous data fragment " + std::to_string(count++) + " | ";

        // Streams data fragments continuously without providing structural destination parameters
        ssize_t bytesSent = send(streamSendSocket, message.c_str(), message.length(), 0);

        if (bytesSent < 0) {
            std::cout << "[Connected-Sender] Stream write operation failed.\n";
            break;
        }

        std::cout << "[Connected-Sender] Streamed: " << message << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    // === INIT STATE ===
    // Step 1: Initialize the local receiver subsystem
    streamRecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in recvAddr;
    std::memset(&recvAddr, 0, sizeof(recvAddr));
    recvAddr.sin_family      = AF_INET;
    recvAddr.sin_port        = htons(6000);  // Listen on interface port 6000
    recvAddr.sin_addr.s_addr = INADDR_ANY;
    bind(streamRecvSocket, (sockaddr*)&recvAddr, sizeof(recvAddr));

    // Step 2: Initialize the sender subsystem
    streamSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in targetAddr;
    std::memset(&targetAddr, 0, sizeof(targetAddr));
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port   = htons(6000);
    inet_pton(AF_INET, "127.0.0.1", &targetAddr.sin_addr);

    // Step 3: Establish a virtual association on the UDP socket to shift it to "Connected mode"
    // This defines a permanent default destination path for all subsequent send() and recv() calls
    connect(streamSendSocket, (sockaddr*)&targetAddr, sizeof(targetAddr));

    // Step 4: Run the data flow handlers across independent tasks
    std::thread tReceive(ReceiveStreamData);
    std::thread tSend(SendStreamData);

    std::cout << "Press ENTER to stop the stream program...\n";
    std::cin.get();

    // === CLOSE STATE ===
    isRunning = false;

    // Step 1: Revoke network interface permissions to unlock hanging read/write operations
    shutdown(streamRecvSocket, SHUT_RDWR);
    shutdown(streamSendSocket, SHUT_RDWR);

    // Step 2: Destroy file descriptors to release system resources fully
    close(streamRecvSocket);
    close(streamSendSocket);

    // Step 3: Block execution path until background worker jobs have formally exited
    if (tReceive.joinable()) tReceive.join();
    if (tSend.joinable()) tSend.join();

    std::cout << "Stream system closed cleanly.\n";
    return 0;
}