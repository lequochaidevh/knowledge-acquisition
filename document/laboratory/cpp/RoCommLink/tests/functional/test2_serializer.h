#include "transport/udp_transport.h"
#include "common/task_queue.h"
#include "service/rocommlink.h"
#include "protocol/packet_serializer.h"

namespace SerializerTest {
// Define a realistic packed struct representing drone flight states
struct __attribute__((packed)) TelemetryData {
    float   latitude                  = 10.76262f;
    float   longitude                 = 106.66017f;
    float   altitude_meters           = 120.5f;
    uint8_t battery_remaining_percent = 85;
};

void execute_task_1_producer(int write_file_descriptor) {
    std::cout << "[Task 1 (Parent)] Gathering system flight stats...\n";

    // 1. Populate custom functional struct container
    TelemetryData drone_stats;
    drone_stats.latitude                  = 10.7756f;
    drone_stats.longitude                 = 106.7019f;
    drone_stats.altitude_meters           = 45.2f;
    drone_stats.battery_remaining_percent = 99;

    // 2. Convert raw struct layout automatically into a clean universal Packet layout
    Packet packed_node = PacketSerializer::pack_struct(1, 2002, drone_stats);

    // 3. Flatten the full tracking envelope into standard binary network strings
    std::vector<uint8_t> complete_wire_frame = PacketSerializer::serialize(packed_node);

    std::cout << "[Task 1 (Parent)] Wire frame packed totalizing: " << complete_wire_frame.size()
              << " bytes. Sending over IPC pipe...\n";

    // 4. Inject into the communication pipe channel
    bool result = write(write_file_descriptor, complete_wire_frame.data(), complete_wire_frame.size());
    (void)result;
    close(write_file_descriptor);
}

void execute_task_2_consumer(int read_file_descriptor) {
    std::cout << "[Task 2 (Child)] Listener online.\n";

    RoCommLinkParser parser;
    uint8_t          stream_byte;

    while (read(read_file_descriptor, &stream_byte, 1) > 0) {
        if (auto packet_optional = parser.parse_byte(stream_byte); packet_optional.has_value()) {
            const auto& packet = packet_optional.value();

            std::cout << "[Task 2 (Child)] ➔ Frame received correctly!\n"
                      << "  Message Identifier Target: " << packet.msg_id << "\n";

            if (packet.msg_id == 2002) {
                // Deserialize payload bytes backward straight into our structured type cleanly
                TelemetryData incoming_stats;
                std::memcpy(&incoming_stats, packet.payload.data(), sizeof(TelemetryData));

                std::cout << "  ➔ [Decoded Telemetry]:\n"
                          << "    Latitude:  " << incoming_stats.latitude << "\n"
                          << "    Longitude: " << incoming_stats.longitude << "\n"
                          << "    Altitude:  " << incoming_stats.altitude_meters << " m\n"
                          << "    Battery:   " << static_cast<int>(incoming_stats.battery_remaining_percent) << "%\n";
            }
        }
    }
    close(read_file_descriptor);
}

int main() {
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) return 1;

    pid_t process_id = fork();
    if (process_id < 0) return 1;

    if (process_id == 0) {
        close(pipe_fds[1]);  // Close unused write end
        execute_task_2_consumer(pipe_fds[0]);
        return 0;
    } else {
        close(pipe_fds[0]);  // Close unused read end
        execute_task_1_producer(pipe_fds[1]);

        int status;
        waitpid(process_id, &status, 0);
    }
    return 0;
}

}  // namespace SerializerTest
