#pragma once
#define STB_IMAGE_IMPLEMENTATION

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

#include "Tensor4D.h"

class ImageLoader {
 public:
    // Core function to load an image file directly into an optimized Tensor4D container
    static Tensor4D load_to_tensor(const std::string& filepath, size_t target_channels = 3) {
        int width, height, actual_channels;

        // Load raw pixel data arrays from disk using stb_image decoder
        unsigned char* raw_pixels =
            stbi_load(filepath.c_str(), &width, &height, &actual_channels, static_cast<int>(target_channels));
        if (raw_pixels == nullptr) {
            throw std::runtime_error("CRITICAL CV ERROR: Failed to read image from path: " + filepath);
        }

        // Initialize Tensor4D with format dimension layout: [Batch=1, Channels, Height, Width]
        size_t   H = static_cast<size_t>(height);
        size_t   W = static_cast<size_t>(width);
        Tensor4D image_tensor(1, target_channels, H, W);

        // Standardized dynamic normalization: Scale raw bytes [0-255] down to float bounds [0.0f -> 1.0f]
        // Memory formatting translation from Interleaved (stb layout: RGBRGB) to Planar (DL layout: RRR...GGG...BBB...)
        for (size_t c = 0; c < target_channels; ++c) {
            for (size_t h = 0; h < H; ++h) {
                for (size_t w = 0; w < W; ++w) {
                    size_t raw_pixel_index      = (h * W + w) * target_channels + c;
                    image_tensor.at(0, c, h, w) = static_cast<float>(raw_pixels[raw_pixel_index]) / 255.0f;
                }
            }
        }

        // Free native allocated graphics buffer pointer safely
        stbi_image_free(raw_pixels);
        return image_tensor;
    }

    // Core function to save an optimized Tensor4D back to a physical image file
    static void save_from_tensor(const std::string& filepath, const Tensor4D& tensor) {
        size_t channels = tensor.get_channels();
        size_t H        = tensor.get_height();
        size_t W        = tensor.get_width();

        // Allocate a flattened temporary raw byte buffer array
        std::vector<unsigned char> raw_pixels(H * W * channels);

        for (size_t c = 0; c < channels; ++c) {
            for (size_t h = 0; h < H; ++h) {
                for (size_t w = 0; w < W; ++w) {
                    size_t raw_pixel_index = (h * W + w) * channels + c;

                    // Denormalize float [0.0f -> 1.0f] back to raw byte [0 -> 255] with safety clamping
                    float val = tensor.at(0, c, h, w) * 255.0f;
                    if (val < 0.0f) val = 0.0f;
                    if (val > 255.0f) val = 255.0f;

                    raw_pixels[raw_pixel_index] = static_cast<unsigned char>(val);
                }
            }
        }

        // Export buffer data to disk as a standard PNG format file
        if (stbi_write_png(filepath.c_str(), static_cast<int>(W), static_cast<int>(H), static_cast<int>(channels),
                           raw_pixels.data(), static_cast<int>(W * channels)) == 0) {
            throw std::runtime_error("CRITICAL CV ERROR: Failed to write image to path: " + filepath);
        }
    }
};