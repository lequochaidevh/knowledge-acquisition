#include "transport/udp_transport.h"
#include "common/task_queue.h"
#include "service/rocommlink.h"
#include "protocol/packet_serializer.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

namespace RoutingTest {

#pragma pack(push, 1)
struct GimbalControlCommand {
    float pan_angle_deg;
    float tilt_angle_deg;
};
#pragma pack(pop)

constexpr uint16_t MSG_ID_GIMBAL_CMD  = 0x0002;
constexpr uint16_t MSG_ID_COMMAND_ACK = 0x00FF;

class TEST_ROUTING {
 public:
    // Node 1: Operator initiating requests targeting system 5 (Gimbal)
    void execute_operator_node() {
        std::cout << "[Operator (System 1)] Station booting up...\n";

        // System 1, Component 1

        RoCommLink comm(2 /*thread*/);

        comm.add_transport(std::make_unique<UdpTransport>(), 0);

        // Connects directly to the Gateway (Port 14550) via local port 14551
        if (!comm.start("127.0.0.1", /*local_port=*/14551, /*remote_port=*/14550, 0)) {
            std::cerr << "[Operator] Infrastructure binding failure\n";
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Allow architecture settling time

        GimbalControlCommand cmd{45.0f, -10.5f};
        // Pack the structure targeted directly to System 5, Component 1 (External Gimbal)
        Packet cmd_packet              = PacketSerializer::pack_struct(5, MSG_ID_GIMBAL_CMD, cmd);
        cmd_packet.header.component_id = 1;
        cmd_packet.header.sequence     = 888;

        std::cout << "[Operator (System 1)] ➔ Dispatching Gimbal Command across Gateway...\n";

        // Blocks natively waiting for structural verification acknowledgement tracking indexes
        comm.send_command_blocking(cmd_packet);

        comm.stop();
        std::cout << "[Operator (System 1)] Script shutdown sequence complete.\n";
    };

    // Node 2: The Core Routing Machine bypassing payload copy loops
    void execute_gateway_node() {
        std::cout << "[Gateway (System 2)] Inter-network routing hub online...\n";

        // Gateway setup as System 2, Component 1
        RoCommLink comm(2);

        // Dynamic dual-interface topology configuration
        comm.add_transport(std::make_unique<UdpTransport>(), 0);  // Line 0: Front-facing Operator
        comm.add_transport(std::make_unique<UdpTransport>(), 1);  // Line 1: Back-facing Gimbal Hardware

        // Explicit network address space boundary routing policies
        comm.add_routing_rule(/*target_sys=*/5, /*target_comp=*/1, /*out_interface=*/1);  // Forward to Gimbal
        comm.add_routing_rule(/*target_sys=*/1, /*target_comp=*/1, /*out_interface=*/0);  // Forward to Operator

        // ➔ FIX: Pre-allocate the parsers inside the map to prevent nullptr crash before data arrives
        {
            // std::unique_lock<std::shared_mutex> lock(comm._parsers_mutex);
            comm._parsers[0] = std::make_unique<RoCommLinkParser>();
            comm._parsers[1] = std::make_unique<RoCommLinkParser>();
        }

        // Register Early screening filter callback loops to force immediate bypass optimizations
        comm._parsers[0]->register_early_filter([&comm](Packet& packet, RoCommLinkParser::State& state) -> bool {
            // If arriving target identification details do not align with gateway identity, pivot state instantly!
            if (packet.header.system_id != 2) {
                state = RoCommLinkParser::State::ForwardActive;
                return true;
            }
            return false;
        });

        // Mirror early filtering conditions cleanly onto the backplane line 1 interface parser
        comm._parsers[1]->register_early_filter([&comm](Packet& packet, RoCommLinkParser::State& state) -> bool {
            if (packet.header.system_id != 2) {
                state = RoCommLinkParser::State::ForwardActive;
                return true;
            }
            return false;
        });

        // 1.  0 facing Operator
        bool gateway_op_ok = comm.start("127.0.0.1", 14550, 14551, 0);

        // 2. 1 facing Gimbal
        bool gateway_gmb_ok = comm.start("127.0.0.1", 14560, 14561, 1);

        if (!gateway_op_ok || !gateway_gmb_ok) {
            return;
        } else {
            std::cout << "[Gateway (System 2)] Good communications infrastructure channels\n";
        }

        // Keep active background event pumps humming smoothly across the validation lifecycle
        std::this_thread::sleep_for(std::chrono::seconds(5));
        comm.stop();
        std::cout << "[Gateway (System 2)] Operational routing framework offline.\n";
    };

    // Node 3: The ultimate destination target machine
    void execute_gimbal_hardware_node() {
        std::cout << "[Gimbal (System 5)] Stabilizer target hardware activating...\n";

        // System 5, Component 1
        RoCommLink comm(2);
        comm.add_transport(std::make_unique<UdpTransport>(), 0);

        // Connects back to the Gateway interface channel (Port 14560) via port 14561
        comm.dispatcher().subscribe(MSG_ID_GIMBAL_CMD, [&comm](const Packet& packet) {
            std::cout << "[Gimbal (System 5)] ➔ Direct match intercepted over network wire lines!\n";

            GimbalControlCommand incoming_cmd;
            std::memcpy(&incoming_cmd, packet.payload.data(), sizeof(GimbalControlCommand));
            std::cout << "  [Action Execution] Adjusting Orientation: Pan=" << incoming_cmd.pan_angle_deg
                      << "°, Tilt=" << incoming_cmd.tilt_angle_deg << "°\n";

            // Generate the matching transactional validation frame targeting the originating Operator node
            Packet ack_pkt{};
            ack_pkt.header.magic        = 0x5A;
            ack_pkt.header.system_id    = 1;  // Explicit route to System 1 (Operator)
            ack_pkt.header.component_id = 1;
            ack_pkt.header.msg_id       = MSG_ID_COMMAND_ACK;
            ack_pkt.header.sequence     = packet.header.sequence;
            ack_pkt.header.payload_len  = sizeof(CommandAckPayload);

            CommandAckPayload ack_payload{};
            ack_payload.command_msg_id  = packet.header.msg_id;
            ack_payload.target_sequence = packet.header.sequence;
            ack_payload.result          = static_cast<uint8_t>(CommandResult::ACCEPTED);

            ack_pkt.payload.resize(sizeof(CommandAckPayload));
            std::memcpy(ack_pkt.payload.data(), &ack_payload, sizeof(CommandAckPayload));

            std::cout << "  [Gimbal (System 5)] Routing feedback validation frame back to Gateway line...\n";
            comm.send_packet(ack_pkt);
            return true;
        });

        if (!comm.start("127.0.0.1", /*local_port=*/14561, /*remote_port=*/14560, 0)) {
            std::cerr << "[Gimbal] Failed to attach communication pipeline sockets\n";
            return;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
        comm.stop();
        std::cout << "[Gimbal (System 5)] Target interface engine shutdown complete.\n";
    };
};

int main() {
    std::cout << "=========================================================\n";
    std::cout << "Launching Multi-Node Stream Forwarding Infrastructure Test\n";
    std::cout << "=========================================================\n";

    TEST_ROUTING testing;
    pid_t        gateway_pid = fork();
    if (gateway_pid < 0) return 1;

    if (gateway_pid == 0) {
        testing.execute_gateway_node();
        return 0;
    }

    pid_t gimbal_pid = fork();
    if (gimbal_pid < 0) return 1;

    if (gimbal_pid == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Prevent thread log collisions
        testing.execute_gimbal_hardware_node();
        return 0;
    }

    // Launch the primary controlling operator node in the parent execution thread context
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    testing.execute_operator_node();

    // Re-collect dangling process environments cleanly from the Linux kernel architecture tables
    int status;
    waitpid(gateway_pid, &status, 0);
    waitpid(gimbal_pid, &status, 0);

    std::cout << "=========================================================\n";
    std::cout << "Routing Test Suite Finalized Successfully\n";
    std::cout << "=========================================================\n";
    return 0;
}

}  // namespace RoutingTest

/*
 [ Operator (Sys 1) ]             [ Gateway (Sys 2) ]            [ Gimbal (Sys 5) ]
   (Interface 0)                    (Intf 0)   (Intf 1)                (Interface 0)
         │                             │          │                          │
         │ ─── [1. Send Packet] ────➔ │          │                          │
         │     (Gimbal Command)        │          │                          │
         │                             │          │                          │
         │   ======================= GATEWAY INGESTION =======================
         │                             │          │                          │
         │                             │── [2. parse_byte()]                 │
         │                             │   (Byte 0-3 Resolved)               │
         │                             │                                     │
         │                             │── [3. execute_early_filter()]       │
         │                             │   (Target System ID != 2)           │
         │                             │                                     │
         │                             │── [4. Switch State]                 │
         │                             │   (State ➔ ForwardActive)           │
         │                             │          │                          │
         │   ====================== STREAMING ROADWAY ========================
         │                             │          │                          │
         │                             │ ── [5. Stream Raw Bytes (1 by 1)] ➔ │
         │                             │    (Zero-Copy / Bypass Payload)     │
         │                             │          │                          │
         │                             │          │ ── [6. Intercept Match] ➔│
         │                             │          │    (Execute Action)      │
         │                             │          │                          │
         │   ======================== FEEDBACK LOOP ==========================
         │                             │          │                          │
         │                             │          │ ── [7. Broadcast ACK] ── │
         │                             │          │    (CommandAckPayload)   │
         │                             │          │                          │
         │                             │ ── [8. parse_byte() on Intf 1] ──── │
         │                             │    (Target System ID != 2)          │
         │                             │                                     │
         │                             │ ── [9. Switch State]                │
         │                             │    (State ➔ ForwardActive)          │
         │                             │          │                          │
         │ ─── [10. Stream ACK Byte] ──│          │                          │
         │     (Back to Operator)      │          │                          │
         │                             │          │                          │
         │   ======================= RESOLVE TRANSACTION =====================
         │                             │          │                          │
         │── [11. Parse Complete]      │          │                          │
         │   (Validates CRC locally)   │          │                          │
         │                             │          │                          │
         │── [12. _command_tracker]    │          │                          │
         │   (resolve transaction)     │          │                          │
         │                             │          │                          │
         │── [13. Unblock Stream]      │          │                          │
         │   (Command accepted!)       │          │                          │
         ▼                             ▼          ▼                          ▼

*/