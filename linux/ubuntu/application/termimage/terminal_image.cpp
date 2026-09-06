#include "termimage_common.h"

enum TerminalType { TERMINAL_KITTY, TERMINAL_NATIVE };

TerminalType detect_terminal() {
    // 1. Check for the unique Kitty Window ID variable
    const char* kitty_id = std::getenv("KITTY_WINDOW_ID");
    if (kitty_id != nullptr) {
        return TERMINAL_KITTY;
    }

    // 2. Fallback check: Look at the standard TERM string variable
    const char* term_env = std::getenv("TERM");
    if (term_env != nullptr) {
        std::string term_str(term_env);
        // If the string contains "kitty", it's a Kitty environment
        if (term_str.find("kitty") != std::string::npos) {
            return TERMINAL_KITTY;
        }
    }

    // If neither matches, it is a native / standard terminal
    return TERMINAL_NATIVE;
}

extern int native_terminal(int argc, char* argv[]);
extern int kitty_terminal(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    TerminalType my_term = detect_terminal();

    if (my_term == TERMINAL_KITTY) {
        std::cout << "🚀 Terminal Detected: Kitty!\n";
        std::cout << "-> Running the high-speed GPU rendering protocol (f=32).\n";
        // Call your high-speed kitty_fast code here
        kitty_terminal(argc, argv);
    } else {
        std::cout << "🖥️ Terminal Detected: Standard/Native Terminal.\n";
        std::cout << "-> Falling back to the text character grid loop (▄).\n";

        // Call your character-block pixel drawing code here
        native_terminal(argc, argv);
    }

    return 0;
}