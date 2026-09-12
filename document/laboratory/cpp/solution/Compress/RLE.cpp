// NOTE GOOD

#include <iostream>
#include <vector>
#include <cstdint>

// 1. REQUIREMENT: Define the implementation macro EXACTLY ONCE before including the header
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Define a compact asset package structure for the compressed output stream
struct RuntimeCompressedAsset {
    std::vector<uint8_t> packedPayload;
    int                  width    = 0;
    int                  height   = 0;
    int                  channels = 0;
};

// RUNTIME COMPRESSOR ENGINE: Takes raw pixel buffers from stb_image and applies RLE
RuntimeCompressedAsset compressStbImageRuntime(const unsigned char* rawPixels, int w, int h, int channels) {
    RuntimeCompressedAsset asset;
    asset.width    = w;
    asset.height   = h;
    asset.channels = channels;

    size_t totalPixels = static_cast<size_t>(w) * h;
    if (totalPixels == 0 || !rawPixels) return asset;

    size_t i = 0;
    while (i < totalPixels) {
        size_t  currentPixelOffset = i * channels;
        uint8_t runLength          = 1;

        // Compare color channel blocks across adjacent window bounds
        while (i + runLength < totalPixels && runLength < 255) {
            size_t nextPixelOffset = (i + runLength) * channels;
            bool   colorsMatch     = true;

            for (int c = 0; c < channels; ++c) {
                if (rawPixels[currentPixelOffset + c] != rawPixels[nextPixelOffset + c]) {
                    colorsMatch = false;
                    break;
                }
            }

            if (!colorsMatch) break;
            runLength++;
        }

        // Push packet data: [Count Byte] + [R, G, B channels sequence]
        asset.packedPayload.push_back(runLength);
        for (int c = 0; c < channels; ++c) {
            asset.packedPayload.push_back(rawPixels[currentPixelOffset + c]);
        }

        i += runLength;
    }

    return asset;
}

int main() {
    // --- CHANGE TO YOUR ACTUAL IMAGE FILE NAME (PNG, JPG, BMP) ---
    std::string imagePath = "test.jpeg";

    int width = 0, height = 0, channels = 0;

    std::cout << "--- [STAGE 1] Loading Real Image Asset via stb_image ---\n";
    // stbi_load automatically processes file metadata and returns a flat pixel array
    unsigned char* rawPixels = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);

    if (rawPixels == nullptr) {
        std::cerr << "[-] ERROR: Failed to load target image: " << imagePath << "\n";
        std::cerr << "[!] Ensure the image file exists in the executable directory.\n";
        return -1;
    }

    size_t originalSizeBytes = static_cast<size_t>(width) * height * channels;
    std::cout << "[+] SUCCESS: Loaded image metadata:\n";
    std::cout << "    -> Dimensions: " << width << " x " << height << " pixels\n";
    std::cout << "    -> Channels  : " << channels << " (3=RGB, 4=RGBA)\n";
    std::cout << "    -> Uncompressed Raw Buffer Size: " << originalSizeBytes << " bytes\n\n";

    std::cout << "--- [STAGE 2] Executing Dynamic Runtime Compression ---\n";
    // 2. Compress the decoded native data payload array
    RuntimeCompressedAsset packedImage = compressStbImageRuntime(rawPixels, width, height, channels);

    std::cout << "[+] SUCCESS: Finished runtime compression matrix:\n";
    std::cout << "    -> Compressed Codec Footprint: " << packedImage.packedPayload.size() << " bytes\n";

    double savingRatio = (1.0 - (double)packedImage.packedPayload.size() / originalSizeBytes) * 100.0;
    std::cout << "    -> Flash Memory Saving Ratio : " << savingRatio << " %\n";

    // 3. MANDATORY CRITICAL CLEANUP: Free memory allocated by stb_image immediately
    stbi_image_free(rawPixels);
    std::cout << "\n[+] Heap memory safely released. System ready.\n";

    return 0;
}