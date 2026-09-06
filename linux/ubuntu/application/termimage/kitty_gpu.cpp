#include "termimage_common.h"

// Optimized Base64 encoder that avoids unnecessary memory allocations
std::string base64_encode(const unsigned char* data, size_t len) {
    static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string       out;
    out.reserve(((len + 2) / 3) * 4);
    int val = 0, valb = -6;
    for (size_t i = 0; i < len; ++i) {
        val = (val << 8) + data[i];
        valb += 8;
        while (valb >= 0) {
            out.push_back(lookup[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

int kitty_terminal(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <path_to_image>\n";
        return 1;
    }

    // 1. Fetch available text cell columns and rows
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

    int term_cols = ws.ws_col ? ws.ws_col : 80;
    int term_rows = ws.ws_row ? ws.ws_row : 24;

    // Safety padding margins to prevent accidental scrolling loops
    term_cols -= 4;
    term_rows -= 4;

    // 2. Load the image directly as raw 32-bit RGBA (Forcing 4 channels)
    int            img_w, img_h, channels;
    unsigned char* img_data = stbi_load(argv[1], &img_w, &img_h, &channels, 4);
    if (!img_data) {
        std::cerr << "Error: Could not load image file.\n";
        return 1;
    }

    // 3. Determine the aspect ratio and choose limiting boundary axis
    double img_aspect  = (double)img_w / img_h;
    double term_aspect = (double)term_cols / (term_rows * 2.0);

    std::string scale_param;
    if (img_aspect > term_aspect) {
        scale_param = "c=" + std::to_string(term_cols);
    } else {
        scale_param = "r=" + std::to_string(term_rows);
    }

    // 4. Encode the raw, uncompressed RGBA pixel bytes directly into Base64
    std::string b64 = base64_encode(img_data, img_w * img_h * 4);
    stbi_image_free(img_data);  // Free the memory immediately

    // 5. Construct Protocol Commands
    // a=T    -> Draw immediately
    // f=32   -> CRUCIAL CHANGE: Tells Kitty this data is raw, uncompressed 32-bit RGBA bytes
    // s=, v= -> Tells Kitty the original pixel width (s) and height (v) so it can decode the raw stream
    std::string start_cmd =
        "\033_Ga=T,f=32,s=" + std::to_string(img_w) + ",v=" + std::to_string(img_h) + "," + scale_param + ",m=";

    // Speed up standard out by optimizing terminal string dumping
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    size_t pos        = 0;
    size_t chunk_size = 4096;
    bool   first      = true;

    while (pos < b64.length()) {
        std::string chunk = b64.substr(pos, chunk_size);
        pos += chunk_size;
        bool has_more = (pos < b64.length());

        if (first) {
            std::cout << start_cmd << (has_more ? "1" : "0") << ";" << chunk << "\033\\";
            first = false;
        } else {
            std::cout << "\033_Gm=" << (has_more ? "1" : "0") << ";" << chunk << "\033\\";
        }
    }
    std::cout << "\n";

    return 0;
}