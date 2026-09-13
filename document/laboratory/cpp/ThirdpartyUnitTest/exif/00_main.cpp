#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <sstream>

/**
 * @brief Safely fetches the current system clock time and formats it
 *        into the strict EXIF standard string pattern: "YYYY:MM:DD HH:MM:SS"
 */
std::string get_current_exif_timestamp() {
    // Get current system time point
    auto        now          = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);

    // Convert to local time structure safely
    std::tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &current_time);
#else
    localtime_r(&current_time, &local_tm);  // Thread-safe on Linux/Ubuntu
#endif

    // Format strictly using the EXIF delimiter specification colon ':'
    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y:%m:%d %H:%M:%S");
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::clog << "Usage: " << argv[0] << " <image.jpg>\n";
        return 1;
    }
    std::string path = argv[1];
    std::cout << "Writing metadata via ExifTool to: " << path << "\n";

    // Fetch dynamic live timestamp from the operating system clock
    std::string current_timestamp = get_current_exif_timestamp();
    std::cout << "-> Auto-generated active timestamp: " << current_timestamp << "\n";

    // Construct the terminal execution command with dynamic variables
    std::string cmd =
        "exiftool -overwrite_original "
        "-Make=\"Internal_Camera\" "
        "-Model=\"Sandbox_v1.0\" "
        "-Software=\"System_CLI_Writer\" "
        "-Copyright=\"Project_Internal_2026\" "
        "-DateTimeOriginal=\"" +
        current_timestamp +
        "\" "
        "-CreateDate=\"" +
        current_timestamp +
        "\" "
        "-ModifyDate=\"" +
        current_timestamp + "\" \"" + path + "\"";

    // Commit changes directly to disk storage
    int result = std::system(cmd.c_str());

    if (result == 0) {
        std::cout << "-> Successfully forced Metadata value directly into JPEG stream!\n";
    } else {
        std::cerr << "-> Write failed! Please verify system configuration parameters.\n";
        return 1;
    }

    return 0;
}
