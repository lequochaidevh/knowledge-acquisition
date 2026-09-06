#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <string>

// Include system headers to check terminal size (Linux / macOS / WSL)
#include <sys/ioctl.h>
#include <unistd.h>

struct Color {
    uint8_t r, g, b;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <path_to_image>\n";
        return 1;
    }

    // 1. Get the current terminal window dimensions
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width  = w.ws_col;  // Available character columns
    int term_height = w.ws_row;  // Available character rows

    // Fallback defaults if terminal size detection fails
    if (term_width == 0) term_width = 80;

    // 2. Load the original image file
    int            orig_width, orig_height, channels;
    unsigned char* img_data = stbi_load(argv[1], &orig_width, &orig_height, &channels, 3);

    if (!img_data) {
        std::cerr << "Error: Failed to load image '" << argv[1] << "'\n";
        return 1;
    }

    // 3. Calculate target display dimensions to maintain aspect ratio
    int display_width  = orig_width;
    int display_height = orig_height;

    // If the image is wider than the terminal, scale it down to fit
    if (display_width > term_width) {
        double scale   = (double)term_width / orig_width;
        display_width  = term_width;
        display_height = (int)(orig_height * scale);
    }

    // 4. Loop over the target/output coordinates (Nearest-Neighbor Downscaling)
    // We jump y by 2 because 1 terminal block holds 2 vertical pixels
    for (int y = 0; y < display_height; y += 2) {
        for (int x = 0; x < display_width; ++x) {
            // Map the target terminal (x, y) coordinates back to the original image coordinates
            int orig_x     = (int)((double)x / display_width * orig_width);
            int orig_y_top = (int)((double)y / display_height * orig_height);
            int orig_y_bot = (int)((double)(y + 1) / display_height * orig_height);

            // Bounds protection (Ensure mapping stays inside original image matrix)
            if (orig_x >= orig_width) orig_x = orig_width - 1;
            if (orig_y_top >= orig_height) orig_y_top = orig_height - 1;
            if (orig_y_bot >= orig_height) orig_y_bot = orig_height - 1;

            // Fetch top pixel color data
            int   top_idx = (orig_y_top * orig_width + orig_x) * 3;
            Color top     = {img_data[top_idx], img_data[top_idx + 1], img_data[top_idx + 2]};

            // Fetch bottom pixel color data
            Color bottom = {0, 0, 0};
            if (y + 1 < display_height) {
                int bot_idx = (orig_y_bot * orig_width + orig_x) * 3;
                bottom      = {img_data[bot_idx], img_data[bot_idx + 1], img_data[bot_idx + 2]};
            }

            // Print colors out onto terminal using \u2584 (▄)
            std::cout << "\033[48;2;" << (int)top.r << ";" << (int)top.g << ";" << (int)top.b << "m"
                      << "\033[38;2;" << (int)bottom.r << ";" << (int)bottom.b << ";" << (int)bottom.b << "m"
                      << "\u2584";
        }
        std::cout << "\033[0m\n";  // End of line reset
    }

    stbi_image_free(img_data);
    return 0;
}