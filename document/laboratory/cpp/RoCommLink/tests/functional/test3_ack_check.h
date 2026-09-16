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

void execute_task_1_operator_node(int read_fd, int write_fd) {
    std::cout << "[Operator (Parent)] System booting up...\n";

    auto       dummy_transport = std::make_unique<UdpTransport>();
    RoCommLink comm(std::move(dummy_transport), 2);

    TakeoffCommand cmd;
    cmd.target_altitude_meters = 15.5f;
    cmd.climb_rate_mps         = 1.5f;

    Packet cmd_packet          = PacketSerializer::pack_struct(1, 0x0001, cmd);
    cmd_packet.header.sequence = 777;

    std::cout << "[Operator (Parent)] ➔ Initiating blocking takeoff sequence...\n";

    std::thread command_thread([&comm, cmd_packet]() mutable { comm.send_command_blocking(cmd_packet); });

    std::vector<uint8_t> serialized_cmd = PacketSerializer::serialize(cmd_packet);
    bool                 write_res      = write(write_fd, serialized_cmd.data(), serialized_cmd.size());
    (void)write_res;

    // Inside your test/transport layer (No comments inside code)
    char    buffer[1024];
    ssize_t bytes_read;

    while ((bytes_read = read(read_fd, buffer, sizeof(buffer))) > 0) {
        // Collect a chunk of bytes, e.g., 64 or 128 bytes at once
        std::string chunk(buffer, static_cast<size_t>(bytes_read));

        // If testing via a mock injector, pass the whole chunk to an exposed injection API
        comm.inject_mock_serial_data(std::move(chunk));
    }

    close(read_fd);
    close(write_fd);

    if (command_thread.joinable()) {
        command_thread.join();
    }

    std::cout << "[Operator (Parent)] Mission script execution complete.\n";
}

void execute_task_2_drone_node(int read_fd, int write_fd) {
    std::cout << "[Drone (Child)] Embedded systems online. Awaiting controller instructions...\n";

    RoCommLinkParser parser;
    uint8_t          stream_byte;

    while (read(read_fd, &stream_byte, 1) > 0) {
        if (auto pkt_opt = parser.parse_byte(stream_byte); pkt_opt.has_value()) {
            Packet packet = std::move(pkt_opt.value());

            if (packet.header.msg_id == 0x0001) {
                std::cout << "[Drone (Child)] ➔ Takeoff command frame intercepted!\n";

                TakeoffCommand incoming_cmd;
                std::memcpy(&incoming_cmd, packet.payload.data(), sizeof(TakeoffCommand));

                std::cout << "  [Executing Action] Climbing to " << incoming_cmd.target_altitude_meters
                          << " meters...\n";

                Packet ack_pkt{};
                ack_pkt.header.magic       = 0xAA;
                ack_pkt.header.system_id   = packet.header.system_id;
                ack_pkt.header.msg_id      = 0x00FF;
                ack_pkt.header.sequence    = 999;
                ack_pkt.header.payload_len = sizeof(CommandAckPayload);

                CommandAckPayload ack_payload{};
                ack_payload.command_msg_id  = packet.header.msg_id;
                ack_payload.target_sequence = packet.header.sequence;
                ack_payload.result          = static_cast<uint8_t>(CommandResult::ACCEPTED);

                ack_pkt.payload.resize(sizeof(CommandAckPayload));
                std::memcpy(ack_pkt.payload.data(), &ack_payload, sizeof(CommandAckPayload));

                std::vector<uint8_t> serialized_ack = PacketSerializer::serialize(ack_pkt);
                bool                 write_res      = write(write_fd, serialized_ack.data(), serialized_ack.size());
                (void)write_res;
                break;
            }
        }
    }

    close(read_fd);
    close(write_fd);
}

int main() {
    int pipe_parent_to_child[2];
    int pipe_child_to_parent[2];

    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        return 1;
    }

    pid_t process_id = fork();
    if (process_id < 0) return 1;

    if (process_id == 0) {
        close(pipe_parent_to_child[1]);
        close(pipe_child_to_parent[0]);

        std::this_thread::sleep_for(std::chrono::seconds(5));
        execute_task_2_drone_node(pipe_parent_to_child[0], pipe_child_to_parent[1]);
        return 0;
    } else {
        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);

        execute_task_1_operator_node(pipe_child_to_parent[0], pipe_parent_to_child[1]);

        int status;
        waitpid(process_id, &status, 0);
    }
    return 0;
}

}  // namespace CommandBlockingTest