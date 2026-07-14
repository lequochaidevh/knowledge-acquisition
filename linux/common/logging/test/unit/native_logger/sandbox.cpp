#include "native_logger.h"

int main() {
    // 1. Test stream mode (operator <<, auto remove std::endl)
    HarisLinux::stdcout << "Stream mode: Hello system!" << std::endl;
    HarisLinux::native_log_warn << "Stream warning code: " << 404;

    // 2. Test basic format mode ("{} {}")
    std::string user           = "Admin";
    int         active_threads = 8;

    HarisLinux::native_print_info("User {} logged in. Active threads: {}", user, active_threads);
    HarisLinux::native_print_warn("System status: {} | Risk level: {}%", "Overloaded", 85);
    HarisLinux::native_print_error("Critical error at address: 0x{}", "7FFF1234");

    HarisLinux::native_print_info("Pure text with no brackets");
    HarisLinux::native_print_info("Missing variable inside this brackets: {}");

    return 0;
}