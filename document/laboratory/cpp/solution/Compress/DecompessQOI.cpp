#include <iostream>
#include <array>
#include <cstdint>

// ====== SIMULATING MOCK EMBEDDED ENVIRONMENT ======
// In your real MCU project, you just #include "image_data.h"
#include "build/image_data.h"

// ==================================================

// Standard QOI Tags definitions for decoder matching logic
#define QOI_MASK_2   0xC0
#define QOI_OP_INDEX 0x00
#define QOI_OP_DIFF  0x40
#define QOI_OP_LUMA  0x80
#define QOI_OP_RUN   0xC0
#define QOI_OP_RGB   0xFE
#define QOI_OP_RGBA  0xFF

struct QoiPixel {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    uint8_t hash() const { return (r * 3 + g * 5 + b * 7 + a * 11) % 64; }
};

// HARDWARE-FRIENDLY RUNTIME STREAM DECODER
// Reads the flash array byte-by-byte and pushes pixels straight to the display
void decompressQoiToDisplay() {
    // Skip the 14-byte header since we already have width/height as compile-time constants
    size_t byteIdx     = 14;
    size_t totalPixels = static_cast<size_t>(IMAGE_WIDTH) * IMAGE_HEIGHT;
    size_t pixelCount  = 0;

    // Allocate minimal internal lookup cache states on RAM (Only 256 bytes!)
    std::array<QoiPixel, 64> colorHistory = {};
    QoiPixel                 px           = {0, 0, 0, 255};  // Current active color register

    std::cout << "[+] MCU Booting: Extracting graphics stream...\n";

    while (pixelCount < totalPixels && byteIdx < QOI_COMPRESSED_BITMAP.size()) {
        uint8_t b1 = QOI_COMPRESSED_BITMAP[byteIdx++];

        if (b1 == QOI_OP_RGB) {
            px.r = QOI_COMPRESSED_BITMAP[byteIdx++];
            px.g = QOI_COMPRESSED_BITMAP[byteIdx++];
            px.b = QOI_COMPRESSED_BITMAP[byteIdx++];
        } else if (b1 == QOI_OP_RGBA) {
            px.r = QOI_COMPRESSED_BITMAP[byteIdx++];
            px.g = QOI_COMPRESSED_BITMAP[byteIdx++];
            px.b = QOI_COMPRESSED_BITMAP[byteIdx++];
            px.a = QOI_COMPRESSED_BITMAP[byteIdx++];
        } else if ((b1 & QOI_MASK_2) == QOI_OP_INDEX) {
            px = colorHistory[b1];
        } else if ((b1 & QOI_MASK_2) == QOI_OP_DIFF) {
            px.r += ((b1 >> 4) & 0x03) - 2;
            px.g += ((b1 >> 2) & 0x03) - 2;
            px.b += (b1 & 0x03) - 2;
        } else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA) {
            uint8_t b2 = QOI_COMPRESSED_BITMAP[byteIdx++];
            int8_t  dg = (b1 & 0x3F) - 32;
            px.r += dg - 8 + ((b2 >> 4) & 0x0F);
            px.g += dg;
            px.b += dg - 8 + (b2 & 0x0F);
        } else if ((b1 & QOI_MASK_2) == QOI_OP_RUN) {
            uint8_t run = (b1 & 0x3F) + 1;
            for (uint8_t r = 0; r < run; ++r) {
                // ------------------------------------------------------------
                // HARDWARE INTEGRATION POINT:
                // Instead of std::cout, you call your LCD driver here, e.g.:
                // myLcdDriver.drawPixel(px.r, px.g, px.b);
                // ------------------------------------------------------------
                pixelCount++;
            }
            colorHistory[px.hash()] = px;
            continue;  // Run branch handles its own insertion block
        }

        // Save current color state into circular cache memory
        colorHistory[px.hash()] = px;

        // Push single decoded pixel straight to hardware pipeline
        pixelCount++;
    }

    std::cout << "[+] SUCCESS: Stream decoding finished. Total pixels sent: " << pixelCount << "\n";
}

int main() {
    // Run the runtime embedded decoder simulation
    decompressQoiToDisplay();
    return 0;
}