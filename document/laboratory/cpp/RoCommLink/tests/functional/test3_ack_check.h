#include "transport/udp_transport.h"
#include "common/task_queue.h"
#include "service/rocommlink.h"
#include "protocol/packet_serializer.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

namespace CommandBlockingTest {

#pragma pack(push, 1)
struct TakeoffCommand {
    float target_altitude_meters;
    float climb_rate_mps;
};
#pragma pack(pop)

void execute_task_1_operator_node() {
    std::cout << "[Operator (Parent)] System booting up...\n";

    auto       dummy_transport = std::make_unique<UdpTransport>();
    RoCommLink comm(std::move(dummy_transport), 2);

    // Operator starts up, binds local port to 14551 and sets outbound target to 14550
    if (!comm.start("127.0.0.1", /*local_port=*/14551, /*remote_port=*/14550)) {
        std::cerr << "[Operator (Parent)] Failed to bind UDP infrastructure node\n";
        return;
    }

    TakeoffCommand cmd{15.5f, 1.5f};
    Packet         cmd_packet  = PacketSerializer::pack_struct(1, 0x0001, cmd);
    cmd_packet.header.sequence = 777;

    std::cout << "[Operator (Parent)] ➔ Initiating blocking takeoff sequence...\n";

    // This call will safely capture the timeline: blocks waiting for ACK,
    // while the internal check_timeouts background thread triggers retries if drone is lagging!
    comm.send_command_blocking(cmd_packet);

    comm.stop();
    std::cout << "[Operator (Parent)] Mission script execution complete.\n";
}

void execute_task_2_drone_node() {
    std::cout << "[Drone (Child)] Embedded systems online. Activating network sockets...\n";

    auto       dummy_transport = std::make_unique<UdpTransport>();
    RoCommLink comm(std::move(dummy_transport), 2);

    // Register transactional handling rules for Message ID 0x0001
    comm.dispatcher().subscribe(0x0001, [&comm](const Packet& packet) {
        std::cout << "[Drone (Child)] ➔ Takeoff command frame intercepted over UDP socket line!\n";

        TakeoffCommand incoming_cmd;
        std::memcpy(&incoming_cmd, packet.payload.data(), sizeof(TakeoffCommand));
        std::cout << "  [Executing Action] Climbing to " << incoming_cmd.target_altitude_meters << "m\n";

        // Manually construct and transmit the feedback ACK frame back to the parent
        Packet ack_pkt{};
        ack_pkt.header.magic       = 0xAA;
        ack_pkt.header.system_id   = packet.header.system_id;
        ack_pkt.header.msg_id      = 0x00FF;                  // Command ACK Message ID
        ack_pkt.header.sequence    = packet.header.sequence;  // Match transaction sequence
        ack_pkt.header.payload_len = sizeof(CommandAckPayload);

        CommandAckPayload ack_payload{};
        ack_payload.command_msg_id  = packet.header.msg_id;
        ack_payload.target_sequence = packet.header.sequence;
        ack_payload.result          = static_cast<uint8_t>(CommandResult::ACCEPTED);

        ack_pkt.payload.resize(sizeof(CommandAckPayload));
        std::memcpy(ack_pkt.payload.data(), &ack_payload, sizeof(CommandAckPayload));

        std::cout << "  [Feedback Loop] Broadcasting outbound ACK back to operator...\n";
        comm.send_packet(ack_pkt);

        return true;
    });
    // Drone starts up, binds local port to 14550 and sets outbound target to 14551
    if (!comm.start("127.0.0.1", /*local_port=*/14550, /*remote_port=*/14551)) {
        std::cerr << "[Drone (Child)] Socket initialization aborted\n";
        return;
    }

    // Keep child process operational loop active to receive message and process network I/O
    std::this_thread::sleep_for(std::chrono::seconds(6));
    comm.stop();
    std::cout << "[Drone (Child)] Shutting down embedded interface node.\n";
}

inline int main() {
    pid_t process_id = fork();
    if (process_id < 0) return 1;

    if (process_id == 0) {
        // Enforce a 5-second initial delay inside the Child process lifecycle
        // This causes the Operator node to trigger its exact 3-cycle retry/timeout tracking loops!
        // std::this_thread::sleep_for(std::chrono::seconds(5));
        execute_task_2_drone_node();
        return 0;
    } else {
        // Give a short 200ms delay to clear terminal racing logs before launching the parent controller
        // std::this_thread::sleep_for(std::chrono::milliseconds(200));
        execute_task_1_operator_node();

        int status;
        waitpid(process_id, &status, 0);
    }
    return 0;
}

}  // namespace CommandBlockingTest