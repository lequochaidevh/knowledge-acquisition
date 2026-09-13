#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <string>

// Include the stb_image library implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Standard QOI Specification Opcode Definitions
#define QOI_OP_INDEX 0x00
#define QOI_OP_DIFF  0x40
#define QOI_OP_LUMA  0x80
#define QOI_OP_RUN   0xC0
#define QOI_OP_RGB   0xFE
#define QOI_OP_RGBA  0xFF

#define QOI_MAGIC (((uint32_t)'q') << 24 | ((uint32_t)'o') << 16 | ((uint32_t)'i') << 8 | 'f')

struct QoiPixel {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    // Spec-defined hash function to map a color to a 0-63 index slot
    constexpr uint8_t hash() const { return (r * 3 + g * 5 + b * 7 + a * 11) % 64; }
};

// HIGH-PERFORMANCE RUNTIME QOI COMPRESSOR
std::vector<uint8_t> compressToQoiRuntime(const unsigned char* rawPixels, int w, int h, int channels) {
    std::vector<uint8_t> out;
    size_t               totalPixels = static_cast<size_t>(w) * h;

    // OPTIMIZATION: Pre-allocate capacity buffer limits to eliminate runtime dynamic resizing overhead
    out.reserve(14 + totalPixels * channels + 8);

    // 1. Write the 14-byte QOI Header Metadata
    out.push_back((QOI_MAGIC >> 24) & 0xFF);
    out.push_back((QOI_MAGIC >> 16) & 0xFF);
    out.push_back((QOI_MAGIC >> 8) & 0xFF);
    out.push_back(QOI_MAGIC & 0xFF);

    out.push_back((w >> 24) & 0xFF);
    out.push_back((w >> 16) & 0xFF);
    out.push_back((w >> 8) & 0xFF);
    out.push_back(w & 0xFF);
    out.push_back((h >> 24) & 0xFF);
    out.push_back((h >> 16) & 0xFF);
    out.push_back((h >> 8) & 0xFF);
    out.push_back(h & 0xFF);
    out.push_back(static_cast<uint8_t>(channels));
    out.push_back(0);  // 0 = sRGB color space profile default

    // 2. Initialize fast local state register variables
    QoiPixel colorHistory[64] = {};  // Flattened layout arrays maximize CPU cache friendliness
    QoiPixel prevPixel        = {0, 0, 0, 255};
    QoiPixel currPixel        = {0, 0, 0, 255};
    uint8_t  runLength        = 0;

    for (size_t px = 0; px < totalPixels; ++px) {
        size_t offset = px * channels;
        currPixel.r   = rawPixels[offset + 0];
        currPixel.g   = rawPixels[offset + 1];
        currPixel.b   = rawPixels[offset + 2];
        if (channels == 4) currPixel.a = rawPixels[offset + 3];

        // OPTIMIZATION LAYER 1: Run-Length Matching String Match
        if (currPixel.r == prevPixel.r && currPixel.g == prevPixel.g && currPixel.b == prevPixel.b &&
            currPixel.a == prevPixel.a) {
            runLength++;
            if (runLength == 62) {  // Maximum permitted bound per specification
                out.push_back(QOI_OP_RUN | (runLength - 1));
                runLength = 0;
            }
        } else {
            if (runLength > 0) {
                out.push_back(QOI_OP_RUN | (runLength - 1));
                runLength = 0;
            }

            // OPTIMIZATION LAYER 2: 64-Byte Internal Hash Table Lookup Cache
            uint8_t indexHash = currPixel.hash();
            if (colorHistory[indexHash].r == currPixel.r && colorHistory[indexHash].g == currPixel.g &&
                colorHistory[indexHash].b == currPixel.b && colorHistory[indexHash].a == currPixel.a) {
                out.push_back(QOI_OP_INDEX | indexHash);
            } else {
                colorHistory[indexHash] = currPixel;  // Update cache matrix index

                if (currPixel.a == prevPixel.a) {
                    int8_t dr = currPixel.r - prevPixel.r;
                    int8_t dg = currPixel.g - prevPixel.g;
                    int8_t db = currPixel.b - prevPixel.b;

                    int8_t dr_dg = dr - dg;
                    int8_t db_dg = db - dg;

                    // OPTIMIZATION LAYER 3: Small Delta Differential Compression (QOI_OP_DIFF)
                    if (dr >= -2 && dr <= 1 && dg >= -2 && dg <= 1 && db >= -2 && db <= 1) {
                        out.push_back(QOI_OP_DIFF | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2));
                    }
                    // OPTIMIZATION LAYER 4: Middle Green Channel Prediction Bias (QOI_OP_LUMA)
                    else if (dg >= -32 && dg <= 31 && dr_dg >= -8 && dr_dg <= 7 && db_dg >= -8 && db_dg <= 7) {
                        out.push_back(QOI_OP_LUMA | (dg + 32));
                        out.push_back(((dr_dg + 8) << 4) | (db_dg + 8));
                    } else {
                        // Uncompressed color branch escape tag
                        out.push_back(QOI_OP_RGB);
                        out.push_back(currPixel.r);
                        out.push_back(currPixel.g);
                        out.push_back(currPixel.b);
                    }
                } else {
                    // Alpha-state change escape tag
                    out.push_back(QOI_OP_RGBA);
                    out.push_back(currPixel.r);
                    out.push_back(currPixel.g);
                    out.push_back(currPixel.b);
                    out.push_back(currPixel.a);
                }
            }
        }
        prevPixel = currPixel;
    }

    if (runLength > 0) {
        out.push_back(QOI_OP_RUN | (runLength - 1));
    }

    // Standard QOI End Marker Trailer padding sequence
    for (int m = 0; m < 7; ++m) out.push_back(0);
    out.push_back(1);

    return out;
}

// METHOD 2: EXPORT TO STANDARDIZED BINARY .QOI FILE FOR SD CARD STREAMING
bool exportToQoiBinary(const std::vector<uint8_t>& qoiData, const std::string& filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) return false;
    outFile.write(reinterpret_cast<const char*>(qoiData.data()), qoiData.size());
    return true;
}

// METHOD 1: EXPORT TO EMBEDDED C++ FLASH STORAGE HEADER FILE (.H)
bool exportToEmbeddedHeader(const std::vector<uint8_t>& qoiData, const std::string& filename, int width, int height,
                            int channels) {
    std::ofstream hFile(filename);
    if (!hFile.is_open()) return false;

    hFile << "// ====== AUTO GENERATED QOI EMBEDDED HARDWARE COMPRESSED DATA ======\n";
    hFile << "#ifndef IMAGE_DATA_H\n#define IMAGE_DATA_H\n\n";
    hFile << "#include <cstdint>\n#include <array>\n\n";

    hFile << "constexpr uint16_t IMAGE_WIDTH    = " << width << ";\n";
    hFile << "constexpr uint16_t IMAGE_HEIGHT   = " << height << ";\n";
    hFile << "constexpr uint8_t  IMAGE_CHANNELS = " << channels << ";\n\n";

    hFile << "// Total flash byte payload count: " << qoiData.size() << " bytes consumed inside Flash\n";
    hFile << "constexpr std::array<uint8_t, " << qoiData.size() << "> QOI_COMPRESSED_BITMAP = {{\n    ";

    for (size_t i = 0; i < qoiData.size(); ++i) {
        hFile << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(qoiData[i]);
        if (i != qoiData.size() - 1) hFile << ", ";
        if ((i + 1) % 12 == 0) hFile << "\n    ";
    }

    hFile << "\n}};\n\n#endif // IMAGE_DATA_H\n";
    return true;
}

int main(int argc, char* argv[]) {
    // RUNTIME ARGUMENT PARSING
    if (argc < 2) {
        std::cout << "Usage syntax error! Correct style:\n";
        std::cout << "  " << argv[0] << " <path_to_image_file.png/jpg/bmp>\n";
        return -1;
    }

    std::string imagePath = argv[1];
    int         width = 0, height = 0, channels = 0;

    std::cout << "--- [STAGE 1] Decoding Live Dynamic Asset via stb_image ---\n";
    unsigned char* rawPixels = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
    if (!rawPixels) {
        std::cerr << "[-] ERROR: Process terminated. Failed to load file location: " << imagePath << "\n";
        return -1;
    }

    size_t originalSizeBytes = static_cast<size_t>(width) * height * channels;
    std::cout << "[+] SUCCESS: Decoded raw graphics context:\n";
    std::cout << "    -> Boundary Size: " << width << "x" << height << " pixels [" << channels << " channels]\n";
    std::cout << "    -> Uncompressed Buffer Dimension: " << originalSizeBytes << " bytes\n";

    std::cout << "\n--- [STAGE 2] Running High Performance QOI Serialization Loop ---\n";
    std::vector<uint8_t> qoiData = compressToQoiRuntime(rawPixels, width, height, channels);

    std::cout << "\n--- [STAGE 3] Automated Multi-Target File Out Serialization ---\n";

    // Execute Method 2: Dump standard external streaming container
    if (exportToQoiBinary(qoiData, "output_image.qoi")) {
        std::cout << "    [+] Method 2 Stream Target dumped successfully: output_image.qoi\n";
    }

    // Execute Method 1: Dump embedded static flash buffer header module
    if (exportToEmbeddedHeader(qoiData, "image_data.h", width, height, channels)) {
        std::cout << "    [+] Method 1 Flash Header Target dumped successfully: image_data.h\n";
    }

    double savingRatio = (1.0 - (double)qoiData.size() / originalSizeBytes) * 100.0;
    std::cout << "\n====================================================\n";
    std::cout << "          COMPRESSION SYSTEM PERFORMANCE METRICS     \n";
    std::cout << "====================================================\n";
    std::cout << " -> Final Packed Storage Layout Size: " << qoiData.size() << " bytes\n";
    std::cout << " -> Flash Footprint Optimization Gain: " << std::fixed << std::setprecision(2) << savingRatio
              << " %\n";
    std::cout << "====================================================\n";

    stbi_image_free(rawPixels);
    return 0;
}