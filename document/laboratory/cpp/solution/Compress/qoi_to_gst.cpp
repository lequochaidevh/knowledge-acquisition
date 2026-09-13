#include <iostream>
#include <vector>
#include <array>
#include <cstdint>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include "build/image_data.h"

// Standard QOI specifications definitions
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

// RUNTIME DECODER ENGINE: Unpacks QOI stream into raw flat RGB memory blocks
std::vector<uint8_t> decompressQoiToBuffer(const std::array<uint8_t, 118160>& compressedData) {
    size_t totalPixels = static_cast<size_t>(IMAGE_WIDTH) * IMAGE_HEIGHT;

    // Allocate 4 bytes per pixel (RGBx) instead of 3 bytes to guarantee 4-byte memory alignment
    std::vector<uint8_t> rawPixels(totalPixels * 4);

    size_t byteIdx    = 14;  // Skip QOI header descriptor
    size_t pixelCount = 0;
    size_t outIdx     = 0;

    std::array<QoiPixel, 64> colorHistory = {};
    QoiPixel                 px           = {0, 0, 0, 255};

    while (pixelCount < totalPixels && byteIdx < compressedData.size()) {
        uint8_t b1 = compressedData[byteIdx++];

        if (b1 == QOI_OP_RGB) {
            px.r = compressedData[byteIdx++];
            px.g = compressedData[byteIdx++];
            px.b = compressedData[byteIdx++];
        } else if (b1 == QOI_OP_RGBA) {
            px.r = compressedData[byteIdx++];
            px.g = compressedData[byteIdx++];
            px.b = compressedData[byteIdx++];
            px.a = compressedData[byteIdx++];
        } else if ((b1 & QOI_MASK_2) == QOI_OP_INDEX) {
            px = colorHistory[b1];
        } else if ((b1 & QOI_MASK_2) == QOI_OP_DIFF) {
            px.r += ((b1 >> 4) & 0x03) - 2;
            px.g += ((b1 >> 2) & 0x03) - 2;
            px.b += (b1 & 0x03) - 2;
        } else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA) {
            uint8_t b2 = compressedData[byteIdx++];
            int8_t  dg = (b1 & 0x3F) - 32;
            px.r += dg - 8 + ((b2 >> 4) & 0x0F);
            px.g += dg;
            px.b += dg - 8 + (b2 & 0x0F);
        } else if ((b1 & QOI_MASK_2) == QOI_OP_RUN) {
            uint8_t run = (b1 & 0x3F) + 1;
            for (uint8_t r = 0; r < run; ++r) {
                rawPixels[outIdx++] = px.r;
                rawPixels[outIdx++] = px.g;
                rawPixels[outIdx++] = px.b;
                rawPixels[outIdx++] = 0xFF;  // Padding dummy 'x' byte to fix stride alignment
                pixelCount++;
            }
            colorHistory[px.hash()] = px;
            continue;
        }

        colorHistory[px.hash()] = px;
        rawPixels[outIdx++]     = px.r;
        rawPixels[outIdx++]     = px.g;
        rawPixels[outIdx++]     = px.b;
        rawPixels[outIdx++]     = 0xFF;  // Padding dummy 'x' byte
        pixelCount++;
    }

    return rawPixels;
}

int main(int argc, char* argv[]) {
    // Initialize GStreamer Core framework architecture
    gst_init(&argc, &argv);

    // 1. EXECUTE CODES: Decompress QOI to raw RGB array buffer on RAM
    // (Assuming you pass your previously saved binary vector payload here)
    // std::vector<uint8_t> decompressQoiToBuffer(const std::vector<uint8_t>& compressedData) {
    std::vector<uint8_t> rawBuffer = decompressQoiToBuffer(QOI_COMPRESSED_BITMAP);

    std::cout << "[+] Graphics data uncompressed. Initializing GStreamer Window sink pipeline...\n";

    // 2. DESIGN HARDWARE PIPELINE: Define the system media routing layout string
    // appsrc receives raw memory -> videoconvert structures pixels -> autovideosink pops visual desktop display
    // windows
    std::string pipelineStr =
        "appsrc name=mysource ! "
        "video/x-raw,format=RGBx,width=225,height=225,framerate=0/1 ! "
        "videoconvert ! "
        "autovideosink";

    GError*     error    = nullptr;
    GstElement* pipeline = gst_parse_launch(pipelineStr.c_str(), &error);
    if (!pipeline) {
        std::cerr << "[-] GStreamer error: " << error->message << "\n";
        g_error_free(error);
        return -1;
    }

    // Locate the dynamic appsrc element handle inside the running pipeline structure
    GstElement* appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "mysource");

    // 3. START PIPELINE STATE: Transition core loop into operational execution profile
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // 4. MEMORY INJECTION: Wrap the raw native vector buffer into a safe GstBuffer packet wrapper
    gpointer   dataCopy = g_memdup(rawBuffer.data(), rawBuffer.size());
    GstBuffer* buffer   = gst_buffer_new_wrapped(dataCopy, rawBuffer.size());

    // Inject the allocated buffer instance directly down the video pipeline path
    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        std::cerr << "[-] Error feeding data packet downstream into appsrc element configuration.\n";
    } else {
        std::cout << "[+] SUCCESS: Pixels successfully pushed to GStreamer. Rendering popup display window...\n";
    }

    // 5. WINDOW MAIN LOOP EXECUTION MANAGEMENT
    // Create an artificial window capture frame hold (Keep active for 5 seconds before cleanup termination)
    GstBus*     bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // 6. SAFE RELEASES HARDWARE TEARDOWN CLEANUP CONTEXT
    if (msg != nullptr) gst_message_unref(msg);
    gst_object_unref(bus);
    gst_object_unref(appsrc);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    std::cout << "[+] Pipeline successfully closed down. Safe memory environment released.\n";
    return 0;
}