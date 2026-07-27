#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <array>
#include <cstdio>
#include <algorithm>
#include <ftxui/ftxui.hpp>  // Comprehensive consolidated header for FTXUI v7.0.1

using namespace ftxui;

// =========================================================================
// CROSS-PLATFORM SYSTEM EXECUTION HELPER (Captures STDOUT streams)
// =========================================================================
std::vector<std::string> ExecuteShellCommand(const std::string& command) {
    std::vector<std::string> output_lines;
    std::array<char, 128>    buffer;

    // Select the platform-specific piping pipeline utility wrapper
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe) {
        output_lines.push_back("❌ ERROR: Failed to allocate process descriptor runtime.");
        return output_lines;
    }

    // Process pipeline output buffers line by line
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string line(buffer.data());
        // Clean out terminal control feeds and trailing carriage return line feeds
        if (!line.empty() && line.back() == '\n') line.pop_back();
        if (!line.empty() && line.back() == '\r') line.pop_back();
        output_lines.push_back(line);
    }

    if (output_lines.empty()) {
        output_lines.push_back("👉 Operation finished executing with an empty output buffer response.");
    }
    return output_lines;
}

int main() {
    // =========================================================================
    // 1. STATE MANAGEMENT & METADATA CONFIGURATION TREE
    // =========================================================================

    // Level 1: Primary Architectural Node Categories
    int                      lvl1_selected = 0;
    std::vector<std::string> lvl1_entries  = {" System Engine ", " Network Admin  "};

    // Level 2: Discrete vectors corresponding to explicit Level 1 selection indexes
    std::vector<std::string> lvl2_data_0 = {"-> Process Overview", "-> Storage Space"};
    std::vector<std::string> lvl2_data_1 = {"-> Active Interfaces", "-> Routing Maps"};

    int lvl2_selected_0 = 0;
    int lvl2_selected_1 = 0;

    // Level 3: Leaf Nodes targeting exact OS process hooks
    // Coordinate Mapping Format: { Level_1_Selection_Idx, Level_2_Selection_Idx }
    std::map<std::pair<int, int>, std::vector<std::string>> lvl3_data = {
        {{0, 0}, {"ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%cpu | head -n 10", "uname -a", "uptime"}},
        {{0, 1}, {"df -h", "du -sh * 2>/dev/null | head -n 5", "ls -lh"}},
        {{1, 0}, {"ifconfig || ip link show || ip a", "cat /etc/resolv.conf 2>/dev/null", "hostname -I"}},
        {{1, 1}, {"netstat -rn || ip route show || route print", "ping -c 3 1.1.1.1 || ping -n 3 1.1.1.1"}}};

    // Tracker map storing selections for individual active terminal leaf options
    std::map<std::pair<int, int>, int> lvl3_selected;

    // Container tracking terminal engine stream entries
    std::vector<std::string> terminal_output_logs = {
        "System Engine ready. Navigate with Arrows. Press [Enter] on Column [C] to execute tasks."};

    // =========================================================================
    // 2. INTERACTIVE TUI COMPONENT CONFIGURATION TREE
    // =========================================================================

    // Initialize base master Level 1 system category menu component
    auto menu_lvl1 = Menu(&lvl1_entries, &lvl1_selected);

    // Initialize Level 2 dependent components directly using matching flat vectors
    auto menu_lvl2_0 = Menu(&lvl2_data_0, &lvl2_selected_0);
    auto menu_lvl2_1 = Menu(&lvl2_data_1, &lvl2_selected_1);

    // Bind Level 2 menu instances into a unified layout context keyed to Level 1
    auto container_lvl2 = Container::Tab({menu_lvl2_0, menu_lvl2_1}, &lvl1_selected);

    // Functional lambda utility factory generating responsive Level 3 interface layers
    auto make_lvl3_menu = [&](int l1, int l2) {
        auto       key = std::make_pair(l1, l2);
        MenuOption option;

        // Define action callback execution logic on hitting Enter
        option.on_enter = [&, key]() {
            int         selected_action = lvl3_selected[key];
            std::string command_to_run  = lvl3_data[key][selected_action];

            // Clean viewframe and publish pipeline indicator state
            terminal_output_logs.clear();
            terminal_output_logs.push_back("🚀 Executing Shell Script Target: " + command_to_run);
            terminal_output_logs.push_back(
                "--------------------------------------------------------------------------------");

            // Fetch structural data array directly from host runtime context
            auto results = ExecuteShellCommand(command_to_run);
            for (const auto& line : results) {
                terminal_output_logs.push_back(line);
            }
        };

        return Menu(&lvl3_data[key], &lvl3_selected[key], option);
    };

    // Instantiation pool binding all runtime menu configurations ahead of execution
    std::vector<Component> lvl3_components_0 = {make_lvl3_menu(0, 0), make_lvl3_menu(0, 1)};
    std::vector<Component> lvl3_components_1 = {make_lvl3_menu(1, 0), make_lvl3_menu(1, 1)};

    auto container_lvl3_sub0 = Container::Tab(lvl3_components_0, &lvl2_selected_0);
    auto container_lvl3_sub1 = Container::Tab(lvl3_components_1, &lvl2_selected_1);

    // Consolidated Level 3 Tab wrapper mapping dynamically down into sub-trackers
    auto container_lvl3 = Container::Tab({container_lvl3_sub0, container_lvl3_sub1}, &lvl1_selected);

    auto screen = ScreenInteractive::Fullscreen();
    screen.TrackMouse(true);

    // --- ADD THIS CODE: Initialize the explicit exit button ---
    auto exit_btn = Button(
        " [X] EXIT ", [&]() { screen.ExitLoopClosure()(); }, ButtonOption::Simple());  // Compact style for the top bar
    // Global layout vector managing unified spatial keyboard tracking arrays
    auto global_input_layout = Container::Horizontal(  //
        {menu_lvl1, container_lvl2, container_lvl3, exit_btn});

    // =========================================================================
    // 3. GRAPHICAL USER INTERFACE LAYOUT DEFINITION (DOM MODULE)
    // =========================================================================
    auto interface_renderer = Renderer(global_input_layout, [&] {
        // Populate dedicated ftxui::Elements object ensuring clean template translation
        ftxui::Elements log_elements;
        for (const auto& line : terminal_output_logs) {
            if (line.rfind("🚀", 0) == 0) {
                log_elements.push_back(text(line) | bold | color(Color::Yellow));
            } else if (line.rfind("❌", 0) == 0) {
                log_elements.push_back(text(line) | bold | color(Color::Red));
            } else {
                log_elements.push_back(text(line) | color(Color::Cyan));
            }
        }

        return vbox(
            {// Application Top Status Header Block
             hbox({text(" 🛠️  REAL-TIME CASCADING SYSTEM DESKTOP HELPER v7.0.1 ")  //
                       | bold | color(Color::White) | bgcolor(Color::Blue),
                   filler(), exit_btn->Render() | color(Color::Red)}),

             // Row 2: Subtitle Hint Text (Aligned using hspace to match Row 1)
             hbox({filler(), text("[Press ESC or Click Exit to Quit Workspace] ") | dim}),

             // Nested Split Panel Multi-Column Configuration Selector Grid
             hbox({window(text(" [A] Primary Class ") | bold | color(Color::Yellow), menu_lvl1->Render()) |
                       size(WIDTH, EQUAL, 26),

                   window(text(" [B] Sub-System Target ") | bold | color(Color::Green), container_lvl2->Render()) |
                       size(WIDTH, EQUAL, 28),

                   window(text(" [C] Shell Actions (Enter) ") | bold | color(Color::Cyan), container_lvl3->Render()) |
                       flex}) |
                 size(HEIGHT, EQUAL, 10),  // Pin menu vertical height footprint safely

             // Real-time Standard Output Log Stream Terminal Viewframe Viewport Panel
             window(text(" Live Host System STDOUT Stream Pipeline ") | bold | color(Color::White),
                    vbox(log_elements) | vscroll_indicator | frame | flex) |
                 flex});
    });

    // =========================================================================
    // 4. MAIN TERMINAL WORKSPACE LOOP INITIALIZATION
    // =========================================================================
    // Assign global lifecycle catch patterns targeting terminating sequences
    auto interactive_loop = CatchEvent(interface_renderer, [&](Event event) {
        if (event == Event::Escape) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen.Loop(interactive_loop);
    return 0;
}