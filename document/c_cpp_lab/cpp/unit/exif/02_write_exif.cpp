#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>

// Safely fetches the current system clock time formatted for EXIF spec
std::string get_current_timestamp_string() {
    auto        now          = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm     local_tm;

    localtime_r(&current_time, &local_tm);

    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y:%m:%d %H:%M:%S");
    return ss.str();
}

/**
 * @brief Helper to write 16-bit integer into a byte vector using Little-Endian order
 */
void push_uint16_le(std::vector<unsigned char>& vec, uint16_t val) {
    vec.push_back(val & 0xFF);
    vec.push_back((val >> 8) & 0xFF);
}

/**
 * @brief Helper to write 32-bit integer into a byte vector using Little-Endian order
 */
void push_uint32_le(std::vector<unsigned char>& vec, uint32_t val) {
    vec.push_back(val & 0xFF);
    vec.push_back((val >> 8) & 0xFF);
    vec.push_back((val >> 16) & 0xFF);
    vec.push_back((val >> 24) & 0xFF);
}

/**
 * @brief Constructs a 100% compliant TIFF/EXIF binary payload from scratch.
 *        Includes Make, Model, Software, Copyright, and DateTime tags with exact byte alignment.
 */
std::vector<unsigned char> build_pure_exif_payload(const std::string& make, const std::string& model,
                                                   const std::string& software, const std::string& copyright,
                                                   const std::string& date_time) {
    std::vector<unsigned char> payload;

    // 1. Write Mandatory 8-byte TIFF Header (Intel Little Endian)
    payload.push_back(0x49);
    payload.push_back(0x49);  // "II" signature
    payload.push_back(0x2A);
    payload.push_back(0x00);        // Magic number 42
    push_uint32_le(payload, 0x08);  // Offset to first IFD

    // 2. Build IFD0 (Main Image Directory) - 5 registered tags
    uint16_t num_fields_ifd0 = 5;
    push_uint16_le(payload, num_fields_ifd0);

    // Calculate memory positions for the actual string allocations
    // TIFF Header(8) + NumFields(2) + 5 Tags * 12 bytes(60) + NextIFDPointer(4) = 74 bytes.
    uint32_t string_data_offset = 74;

    // Tag 1: Camera Make (0x010F)
    push_uint16_le(payload, 0x010F);              // Tag ID
    push_uint16_le(payload, 2);                   // Format: ASCII (2)
    push_uint32_le(payload, make.length() + 1);   // Component count
    push_uint32_le(payload, string_data_offset);  // Address offset pointer
    string_data_offset += make.length() + 1;

    // Tag 2: Camera Model (0x0110)
    push_uint16_le(payload, 0x0110);              // Tag ID
    push_uint16_le(payload, 2);                   // Format: ASCII
    push_uint32_le(payload, model.length() + 1);  // Component count
    push_uint32_le(payload, string_data_offset);  // Address offset pointer
    string_data_offset += model.length() + 1;

    // Tag 3: Software (0x0131)
    push_uint16_le(payload, 0x0131);                 // Tag ID
    push_uint16_le(payload, 2);                      // Format: ASCII
    push_uint32_le(payload, software.length() + 1);  // Component count
    push_uint32_le(payload, string_data_offset);     // Address offset pointer
    string_data_offset += software.length() + 1;

    // Tag 4: Copyright (0x8298)
    push_uint16_le(payload, 0x8298);                  // Tag ID
    push_uint16_le(payload, 2);                       // Format: ASCII
    push_uint32_le(payload, copyright.length() + 1);  // Component count
    push_uint32_le(payload, string_data_offset);      // Address offset pointer
    string_data_offset += copyright.length() + 1;

    // Tag 5: Date Time (0x0132)
    push_uint16_le(payload, 0x0132);                  // Tag ID
    push_uint16_le(payload, 2);                       // Format: ASCII
    push_uint32_le(payload, date_time.length() + 1);  // Component count
    push_uint32_le(payload, string_data_offset);      // Address offset pointer

    // Next IFD Offset Pointer (0 means end of directories)
    push_uint32_le(payload, 0);

    // 3. Append the raw actual string payloads byte-by-byte into the vector (with '\0')
    payload.insert(payload.end(), make.begin(), make.end());
    payload.push_back('\0');
    payload.insert(payload.end(), model.begin(), model.end());
    payload.push_back('\0');
    payload.insert(payload.end(), software.begin(), software.end());
    payload.push_back('\0');
    payload.insert(payload.end(), copyright.begin(), copyright.end());
    payload.push_back('\0');
    payload.insert(payload.end(), date_time.begin(), date_time.end());
    payload.push_back('\0');

    return payload;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::clog << "Usage: " << argv << " <image.jpg>\n";
        return 1;
    }
    std::string path = argv[1];

    // Load original binary file from FFmpeg into active RAM memory
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Error: File not found.\n";
        return 1;
    }
    std::vector<unsigned char> src((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    if (src.size() < 2 || src[0] != 0xFF || src[1] != 0xD8) {
        std::cerr << "Error: Source file is not a valid JPEG stream.\n";
        return 1;
    }

    // Generate dynamic textual metadata values
    std::string make      = "Internal_Camera_Performance";
    std::string model     = "Pure_CPP_Parser_v2.0";
    std::string software  = "Pure_CPP_Performance_Engine";
    std::string copyright = "Project_Internal_2026";
    std::string timestamp = get_current_timestamp_string();

    // Construct the standard aligned EXIF payload natively
    std::vector<unsigned char> exif_payload = build_pure_exif_payload(make, model, software, copyright, timestamp);

    // Reconstruct output image target stream instantly in memory
    std::ofstream out(path, std::ios::binary);
    out.put(0xFF);
    out.put(0xD8);  // Write SOI

    // Inject the pristine APP1 (EXIF Container Segment Marker)
    out.put(0xFF);
    out.put(0xE1);

    // Compute exact chunk boundary dimensions: 2 (length bytes) + 6 ("Exif\0\0") + PayloadSize
    uint16_t app1_length = 2 + 6 + exif_payload.size();
    out.put((app1_length >> 8) & 0xFF);
    out.put(app1_length & 0xFF);
    out.write("Exif\0\0", 6);                                                      // Magic identifier string
    out.write(reinterpret_cast<char*>(exif_payload.data()), exif_payload.size());  // Bulk stream dump

    // --- FIX FOR FFmpeg yuvj444p ---
    // If the original image already has a header segment right at byte 2, step over it safely.
    // Otherwise, copy the file data immediately from offset byte 2 to prevent corrupting tables.
    // size_t src_read_offset = 2;
    // if (src_read_offset + 3 < src.size() && src[src_read_offset] == 0xFF &&
    //     (src[src_read_offset + 1] == 0xE0 || src[src_read_offset + 1] == 0xE1)) {
    //     uint16_t old_segment_len = (src[src_read_offset + 2] << 8) | src[src_read_offset + 3];
    //     src_read_offset += 2 + old_segment_len;  // Safely skip the collision boundary
    // }

    size_t src_read_offset = 2;
    while (src_read_offset < src.size() - 4) {
        if (src[src_read_offset] == 0xFF) {
            unsigned char marker = src[src_read_offset + 1];

            //  marker APP0 to APP15 (0xE0 - 0xEF) or comment block COM (0xFE)
            if ((marker >= 0xE0 && marker <= 0xEF) || marker == 0xFE) {
                // Đọc độ dài khối (2 byte tiếp theo) để nhảy qua
                uint16_t segment_len = (src[src_read_offset + 2] << 8) | src[src_read_offset + 3];
                src_read_offset += 2 + segment_len;
            } else {
                // meet data (DQT, DHT, SOF, SOS)
                break;
            }
        } else {
            src_read_offset++;
        }
    }

    // Append all original remaining compressed pixel matrices down to the file
    out.write(reinterpret_cast<char*>(&src[src_read_offset]), src.size() - src_read_offset);
    out.close();

    std::cout << "-> Performance Engine: Successfully injected metadata directly in memory!\n";
    return 0;
}
