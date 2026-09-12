#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>

// Require the stb_image implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// QOI spec definitions
#define QOI_OP_INDEX 0x00
#define QOI_OP_DIFF  0x40
#define QOI_OP_LUMA  0x80
#define QOI_OP_RUN   0xC0
#define QOI_OP_RGB   0xFE
#define QOI_OP_RGBA  0xFF

#define QOI_MAGIC (((uint32_t)'q') << 24 | ((uint32_t)'o') << 16 | ((uint32_t)'i') << 8 | 'f')

struct QoiHeader {
    uint32_t magic;
    uint32_t width;
    uint32_t height;
    uint8_t  channels;
    uint8_t  colorspace;
};

struct QoiPixel {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    // Hash function defined by QOI specification to map color into a 64-buffer index
    constexpr uint8_t hash() const { return (r * 3 + g * 5 + b * 7 + a * 11) % 64; }
};

// RUNTIME QOI COMPRESSOR ENGINE
std::vector<uint8_t> compressToQoiRuntime(const unsigned char* rawPixels, int w, int h, int channels) {
    std::vector<uint8_t> out;
    size_t               totalPixels = static_cast<size_t>(w) * h;

    // Reserve an approximate size boundary to optimize vector push limits
    out.reserve(sizeof(QoiHeader) + totalPixels * channels + 8);

    // 1. Write standard QOI Header
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
    out.push_back(channels);
    out.push_back(0);  // 0 = sRGB color space default metadata

    // 2. Initialize tracking state variables
    QoiPixel colorHistory[64] = {};  // Internal 64-color circular loop cache
    QoiPixel prevPixel        = {0, 0, 0, 255};
    QoiPixel currPixel        = {0, 0, 0, 255};
    uint8_t  runLength        = 0;

    for (size_t px = 0; px < totalPixels; ++px) {
        size_t offset = px * channels;
        currPixel.r   = rawPixels[offset + 0];
        currPixel.g   = rawPixels[offset + 1];
        currPixel.b   = rawPixels[offset + 2];
        if (channels == 4) currPixel.a = rawPixels[offset + 3];

        // CHECK 1: Run-Length Match
        if (currPixel.r == prevPixel.r && currPixel.g == prevPixel.g && currPixel.b == prevPixel.b &&
            currPixel.a == prevPixel.a) {
            runLength++;
            if (runLength == 62) {  // Max run size constraint per token block
                out.push_back(QOI_OP_RUN | (runLength - 1));
                runLength = 0;
            }
        } else {
            // Flush any active run sequences before processing tag boundaries
            if (runLength > 0) {
                out.push_back(QOI_OP_RUN | (runLength - 1));
                runLength = 0;
            }

            // CHECK 2: Cache Index Buffer Match
            uint8_t indexHash = currPixel.hash();
            if (colorHistory[indexHash].r == currPixel.r && colorHistory[indexHash].g == currPixel.g &&
                colorHistory[indexHash].b == currPixel.b && colorHistory[indexHash].a == currPixel.a) {
                out.push_back(QOI_OP_INDEX | indexHash);
            } else {
                // Update history lookup cache slots
                colorHistory[indexHash] = currPixel;

                if (currPixel.a == prevPixel.a) {
                    int8_t dr = currPixel.r - prevPixel.r;
                    int8_t dg = currPixel.g - prevPixel.g;
                    int8_t db = currPixel.b - prevPixel.b;

                    int8_t dr_dg = dr - dg;
                    int8_t db_dg = db - dg;

                    // CHECK 3: Small Delta Difference (QOI_OP_DIFF) - 2 bit signed ranges
                    if (dr >= -2 && dr <= 1 && dg >= -2 && dg <= 1 && db >= -2 && db <= 1) {
                        out.push_back(QOI_OP_DIFF | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2));
                    }
                    // CHECK 4: Luma Channel Delta Difference (QOI_OP_LUMA)
                    else if (dg >= -32 && dg <= 31 && dr_dg >= -8 && dr_dg <= 7 && db_dg >= -8 && db_dg <= 7) {
                        out.push_back(QOI_OP_LUMA | (dg + 32));
                        out.push_back(((dr_dg + 8) << 4) | (db_dg + 8));
                    } else {
                        // CHECK 5: Full fallback write (QOI_OP_RGB)
                        out.push_back(QOI_OP_RGB);
                        out.push_back(currPixel.r);
                        out.push_back(currPixel.g);
                        out.push_back(currPixel.b);
                    }
                } else {
                    // CHECK 6: Alpha change fallback (QOI_OP_RGBA)
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

    // Standard QOI End Marker trailer sequence
    for (int m = 0; m < 7; ++m) out.push_back(0);
    out.push_back(1);

    return out;
}

int main() {
    // Uses your exact 225x225 real image layout file target
    std::string imagePath = "test.jpeg";
    int         width = 0, height = 0, channels = 0;

    std::cout << "--- [STAGE 1] Loading Real Image Asset via stb_image ---\n";
    unsigned char* rawPixels = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);

    if (!rawPixels) {
        std::cerr << "[-] ERROR: Failed to load target image: " << imagePath << "\n";
        return -1;
    }

    size_t originalSizeBytes = static_cast<size_t>(width) * height * channels;
    std::cout << "[+] SUCCESS: Loaded " << width << "x" << height << " (" << channels << " channels)\n";
    std::cout << "    Raw Uncompressed Buffer: " << originalSizeBytes << " bytes\n\n";

    std::cout << "--- [STAGE 2] Executing Dynamic Runtime QOI Compression ---\n";
    std::vector<uint8_t> qoiData = compressToQoiRuntime(rawPixels, width, height, channels);

    std::cout << "[+] SUCCESS: Finished runtime QOI encoder pipeline:\n";
    std::cout << "    -> Compressed Codec Footprint: " << qoiData.size() << " bytes\n";

    double savingRatio = (1.0 - (double)qoiData.size() / originalSizeBytes) * 100.0;
    std::cout << "    -> Flash Memory Saving Ratio : " << savingRatio << " %\n";

    stbi_image_free(rawPixels);
    return 0;
}