#include <iostream>
#include <string>
#include <libexif/exif-data.h>

// Safely extracts text data entries from the parsed image container mapping structures
std::string read_text_tag(ExifData* exif, ExifTag tag) {
    if (!exif) return "N/A";

    ExifEntry* entry = exif_data_get_entry(exif, tag);
    if (!entry || !entry->data || entry->size == 0) return "N/A";

    // Allocate buffer size based on entry formatting to dump clean text sequences
    char buf[1024] = {0};
    exif_entry_get_value(entry, buf, sizeof(buf));
    return std::string(buf);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::clog << "Usage: " << argv[0] << " <image.jpg>\n";
        return 1;
    }
    std::string path = argv[1];
    std::cout << "Opening image to read metadata: " << path << "\n";

    // Instantiates an active reader parse block mapped to the physical image file path
    ExifData* exif = exif_data_new_from_file(path.c_str());
    if (!exif) {
        std::cerr << "Error: Could not read EXIF data or file is invalid.\n";
        return 1;
    }

    std::string make      = read_text_tag(exif, EXIF_TAG_MAKE);
    std::string model     = read_text_tag(exif, EXIF_TAG_MODEL);
    std::string software  = read_text_tag(exif, EXIF_TAG_SOFTWARE);
    std::string copyright = read_text_tag(exif, EXIF_TAG_COPYRIGHT);
    std::string datetime  = read_text_tag(exif, EXIF_TAG_DATE_TIME);

    std::cout << "====================================\n";
    std::cout << "  VERIFIED EXIF METADATA FROM FILE  \n";
    std::cout << "====================================\n";
    std::cout << "Camera Make   : " << make << "\n";
    std::cout << "Camera Model  : " << model << "\n";
    std::cout << "Software Used : " << software << "\n";
    std::cout << "Copyright     : " << copyright << "\n";
    std::cout << "Date & Time   : " << datetime << "\n";
    std::cout << "====================================\n";

    exif_data_unref(exif);
    return 0;
}
