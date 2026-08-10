#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <cstring>
#include <libexif/exif-data.h>

/**
 * @brief Creates a default EXIF data structure with Intel byte order
 *        and initializes mandatory system tags to prevent empty/corrupt readouts.
 * @return Pointer to the allocated ExifData, or nullptr on failure.
 */
ExifData* create_default_exif_data() {
    ExifData* exif_data = exif_data_new();
    if (!exif_data) return nullptr;

    // Set byte order to Intel (little-endian)
    exif_data_set_byte_order(exif_data, EXIF_BYTE_ORDER_INTEL);

    // FIX HERE: Automatically generate all mandatory EXIF specification fields
    // This ensures third-party image viewers and libexif recognize the block as valid.
    exif_data_fix(exif_data);

    return exif_data;
}

/**
 * @brief Sets or updates an ASCII string value for a specific EXIF tag in IFD0.
 * @param exif_data Pointer to the ExifData structure.
 * @param tag The specific EXIF tag to write (e.g., EXIF_TAG_MAKE).
 * @param value The string content to write into the tag.
 */
void set_exif_tag_value(ExifData* exif_data, ExifTag tag, const std::string& value) {
    if (!exif_data) return;

    ExifEntry* entry = exif_data_get_entry(exif_data, tag);
    if (!entry) {
        entry = exif_entry_new();
        if (!entry) return;

        entry->tag = tag;
        // Make and Model belong to IFD_0 (Main Image directory)
        exif_content_add_entry(exif_data->ifd[EXIF_IFD_0], entry);
        exif_entry_initialize(entry, tag);
    }

    entry->format     = EXIF_FORMAT_ASCII;
    entry->components = value.length() + 1;  // Include null terminator '\0'

    if (entry->data) free(entry->data);
    entry->data = (unsigned char*)malloc(entry->components);
    if (entry->data) {
        std::memcpy(entry->data, value.c_str(), entry->components);
        entry->size = entry->components;
    }
}

/**
 * @brief Safely reads and formats an EXIF tag value as a C++ string.
 * @param exif_data Pointer to the ExifData structure.
 * @param tag The EXIF tag to read.
 * @return The formatted string value, or "N/A" if not found.
 */
std::string get_exif_tag_value(ExifData* exif_data, ExifTag tag) {
    if (!exif_data) return "N/A";

    ExifEntry* entry = exif_data_get_entry(exif_data, tag);
    if (!entry) return "N/A";

    char buf[1024] = {0};
    exif_entry_get_value(entry, buf, sizeof(buf));
    return std::string(buf);
}

/**
 * @brief Safely inserts EXIF metadata into a JPEG file without corrupting pixel data.
 * @param file_path Path to the target JPEG file.
 * @param exif_data Pointer to the ExifData structure to serialize and write.
 * @return true if successful, false otherwise.
 */
bool write_exif_to_jpeg_file(const std::string& file_path, ExifData* exif_data) {
    // 1. Serialize ExifData into a raw memory byte array via libexif
    unsigned char* exif_data_ptr = nullptr;
    unsigned int   exif_data_len = 0;
    exif_data_save_data(exif_data, &exif_data_ptr, &exif_data_len);
    if (!exif_data_ptr || exif_data_len == 0) return false;

    // 2. Read the entire original JPEG file into memory
    std::ifstream in_file(file_path, std::ios::binary);
    if (!in_file) {
        free(exif_data_ptr);
        return false;
    }
    std::vector<unsigned char> original_content((std::istreambuf_iterator<char>(in_file)),
                                                std::istreambuf_iterator<char>());
    in_file.close();

    // Validate JPEG magic header: must start with SOI (0xFF 0xD8)
    if (original_content.size() < 2 || original_content[0] != 0xFF || original_content[1] != 0xD8) {
        std::cerr << "Error: Target file is not a valid JPEG format.\n";
        free(exif_data_ptr);
        return false;
    }

    // 3. Open the same file to overwrite and reconstruct the binary safely
    std::ofstream out_file(file_path, std::ios::binary);
    if (!out_file) {
        free(exif_data_ptr);
        return false;
    }

    // Write original SOI Marker (0xFFD8)
    out_file.put(0xFF);
    out_file.put(0xD8);

    // Write APP1 Marker (0xFFE1) for the new EXIF block
    out_file.put(0xFF);
    out_file.put(0xE1);

    // Calculate and write APP1 Segment Length: 2 (length bytes) + 6 ("Exif\0\0") + payload length
    unsigned int app1_len = 2 + 6 + exif_data_len;
    out_file.put((app1_len >> 8) & 0xFF);
    out_file.put(app1_len & 0xFF);

    // Write the standard EXIF header signature string
    out_file.write("Exif\0\0", 6);

    // Write the raw EXIF payload data
    out_file.write(reinterpret_cast<char*>(exif_data_ptr), exif_data_len);

    // Free the libexif allocated C pointer immediately
    free(exif_data_ptr);

    // 4. Safely parse and skip any existing old APP1 segment from original content, then append the rest
    size_t offset = 2;  // Start scanning right after original SOI (0xFFD8)
    if (offset + 3 < original_content.size() && original_content[offset] == 0xFF &&
        original_content[offset + 1] == 0xE1) {
        // If an old APP1 segment is found right at the start, extract its length and step over it entirely
        unsigned int old_app1_len = (original_content[offset + 2] << 8) | original_content[offset + 3];
        offset += 2 + old_app1_len;
    }

    // Append all the remaining raw image data bytes exactly as they were (fixes image corruption)
    if (offset < original_content.size()) {
        out_file.write(reinterpret_cast<char*>(&original_content[offset]), original_content.size() - offset);
    }

    out_file.close();
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::clog << "Usage: " << argv << " <image_path.jpg>\n";
        return 1;
    }

    std::string image_path = argv[1];
    std::cout << "Reading target file: " << image_path << "\n";

    // Attempt to load existing EXIF
    ExifData* exif_data = exif_data_new_from_file(image_path.c_str());

    if (!exif_data) {
        std::cout << "-> EXIF structure empty. Initializing custom metadata block...\n";
        exif_data = create_default_exif_data();
        if (!exif_data) {
            std::cerr << "Error: Failed to allocate EXIF memory structure.\n";
            return 1;
        }
    }

    // --- WRITE AREA ---
    set_exif_tag_value(exif_data, EXIF_TAG_MAKE, "Hethong_Noibo_Camera");
    set_exif_tag_value(exif_data, EXIF_TAG_MODEL, "Sandbox_v1.0");
    set_exif_tag_value(exif_data, EXIF_TAG_SOFTWARE, "Internal C++ EXIF Writer v1.2");

    // Commit and save data back into the file
    if (write_exif_to_jpeg_file(image_path, exif_data)) {
        std::cout << "-> Successfully synchronized and wrote Metadata to file!\n";
    } else {
        std::cerr << "Error: Failed to patch EXIF into the JPEG binary stream.\n";
        exif_data_unref(exif_data);
        return 1;
    }

    // --- READ AREA (Verification) ---
    std::string camera_make   = get_exif_tag_value(exif_data, EXIF_TAG_MAKE);
    std::string camera_model  = get_exif_tag_value(exif_data, EXIF_TAG_MODEL);
    std::string software_used = get_exif_tag_value(exif_data, EXIF_TAG_SOFTWARE);

    std::cout << "====================================\n";
    std::cout << "Camera Make   : " << camera_make << "\n";
    std::cout << "Camera Model  : " << camera_model << "\n";
    std::cout << "Software Used : " << software_used << "\n";
    std::cout << "====================================\n";

    exif_data_unref(exif_data);
    return 0;
}
