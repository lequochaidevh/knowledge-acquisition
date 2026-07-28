#include <iostream>
#include <vector>
#include <string>

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

int main() {
    // Initialize a full-screen interactive terminal session
    auto screen = ScreenInteractive::Fullscreen();

    // --------------------------------------------------------
    // Navigation / Window Management State
    // 0: Main Dashboard (Sidebar Menu + Dynamic Content Panel)
    // 1: Independent Fullscreen New Window
    // --------------------------------------------------------
    int current_window = 0;

    // --------------------------------------------------------
    // COMPONENTS FOR WINDOW 0 (MAIN DASHBOARD)
    // --------------------------------------------------------

    // 1. Sidebar Navigation Menu
    int                      menu_selected = 0;
    std::vector<std::string> menu_entries  = {"📊 Data Table", "🎛️ Slider Gallery", "🔲 Launch New Window"};

    // The call back OnEvent auto increase or decrease meunu_selected
    auto menu = Menu(&menu_entries, &menu_selected);

    // 2. Data Table Component (Renders inside the main dynamic content panel)
    auto table_content = Renderer([&] {
        return vbox({text(" 📊 NEW COMPONENT STOCK INVENTORY ") | bold | color(Color::Cyan), separator(),
                     gridbox({
                         {text(" ID ") | bold, text(" Hardware Name ") | bold, text(" Unit Price ") | bold},
                         {text(" 001 "), text(" AMD Ryzen 9 7950X "), text(" $549 ")},
                         {text(" 002 "), text(" Corsair DDR5 64GB "), text(" $210 ")},
                         {text(" 003 "), text(" NVIDIA RTX 5090 Ti "), text(" $2499 ")},
                     }) | border});
    });

    // 3. Various Slider Controls Initialization
    int slider_horiz_val = 40;
    int slider_vert_val  = 60;
    int slider_gauge_val = 75;

    // Standard Horizontal Slider using basic shorthand function
    auto slider_horiz = Slider("Horizontal: ", &slider_horiz_val, 0, 100, 2);

    // Advanced Vertical Slider configured via SliderOption for FTXUI compliance
    SliderOption<int> vert_options;
    vert_options.value     = &slider_vert_val;
    vert_options.min       = 0;
    vert_options.max       = 100;
    vert_options.increment = 5;
    vert_options.direction = Direction::Up;  // Controls move vertically upwards
    auto slider_vert       = Slider(vert_options);

    // Gauge Slider (Attached to an analytical visual bar below)
    auto slider_gauge = Slider("Intensity: ", &slider_gauge_val, 0, 100, 1);

    // Layout configuration for the Slider Showcase View
    auto sliders_content = Renderer([&] {
        return vbox(
            {text(" 🎛️ MIXED SLIDERS AND CONTROLS ") | bold | color(Color::Green), separator(),

             hbox({// Left Column: Horizontal layouts
                   vbox({text("1. Standard Horizontal Slider:") | bold, slider_horiz->Render(), separatorDashed(),

                         text("2. Slider coupled with Visual Gauge:") | bold, slider_gauge->Render(),
                         gauge(slider_gauge_val / 100.0) | color(Color::Red),
                         text(std::to_string(slider_gauge_val) + "%") | hcenter}) |
                       flex | border,

                   // Right Column: Vertical layouts
                   vbox({text("3. Vertical Slider:") | bold | hcenter,
                         hbox({text("Value: "),
                               // Explicitly constraints rendering space dimensions for the vertical slider
                               slider_vert->Render() | size(HEIGHT, EQUAL, 8) | size(WIDTH, EQUAL, 3)}) |
                             hcenter | flex,
                         text("Level: " + std::to_string(slider_vert_val)) | hcenter}) |
                       size(WIDTH, EQUAL, 25) | border})});
    });

    // 4. Action Button to transition state into the independent screen
    auto btn_open_window = Button(
        " 🚀 LOAD NEW FULLSCREEN WINDOW ",
        [&] {
            current_window = 1;  // Shifts view state instantly to window 1
        },
        ButtonOption::Animated());

    // Switch-case UI coordinator resolving which view loads relative to menu index selection
    auto main_dynamic_content = Renderer([&] {
        switch (menu_selected) {
            case 0:
                return table_content->Render();
            case 1:
                return sliders_content->Render();
            case 2:
                return vbox({filler(), hbox({filler(), btn_open_window->Render(), filler()}),
                             filler()});  // center and padding full box
            default:
                return text("Page Not Found");
        }
    });

    // --------------------------------------------------------
    // COMPONENTS FOR WINDOW 1 (INDEPENDENT WINDOW / FULLSCREEN)
    // --------------------------------------------------------

    // Back navigation control element
    auto btn_back = Button(
        " ⬅️ GO BACK (Or press Esc) ",
        [&] {
            current_window = 0;  // Reverts state control back to Dashboard
        },
        ButtonOption::Animated());

    // Full screen view structure configuration for Window 1
    auto new_window_renderer = Renderer([&] {
        return vbox(
            {// Application Top Header Line
             hbox({
                 text(" 🖥️ ISOLATED WORKSPACE ENVIRONMENT (NEW WINDOW) ") | bold, filler(),
                 btn_back->Render() | color(Color::Red)  // Anchored on top-right edge
             }) | border,

             separator(),

             // Central Content Container
             vbox({text("This workspace layer has successfully scaled over the whole terminal!") | hcenter,
                   text("The background sidebar menu and dashboard have been fully isolated.") | hcenter, filler(),
                   text("UX Tip: Press 'Esc' key on your physical keyboard to back-route immediately.") |
                       color(Color::GrayLight) | hcenter,
                   filler()}) |
                 flex | borderDashed});
    });

    // --------------------------------------------------------
    // GLOBAL INTERACTION MATRIX (CONTAINERS & MAIN EVENT LOOP)
    // --------------------------------------------------------

    // Container Tab establishes key focus isolation groups based on active integer tracking index
    auto global_container = Container::Tab(
        {// State 0 Focus Tree Group
         Container::Horizontal({menu, Container::Vertical({slider_horiz, slider_vert, slider_gauge, btn_open_window})}),
         // State 1 Focus Tree Group
         Container::Vertical({btn_back})},
        &current_window);

    // Global Key Event Interceptor providing shortcut mapping options
    auto event_handler = CatchEvent(global_container, [&](Event event) {
        // If window 1 is running and user triggers escape button, switch back automatically
        if (current_window == 1 && event == Event::Escape) {
            current_window = 0;
            return true;  // Resolves true to consume input event lifecycle
        }
        return false;
    });

    // Final Rendering Assembly rendering elements per frame based on context index
    auto final_renderer = Renderer(event_handler, [&] {
        if (current_window == 0) {
            // Render classic split dashboard view pane
            return hbox({vbox({
                             text(" 🛠️ CONTROL HUB ") | bold | color(Color::Yellow),
                             separator(),
                             menu->Render(),
                         }) | size(WIDTH, EQUAL, 28) |
                             border,

                         main_dynamic_content->Render() | flex | border});
        } else {
            // Render absolute canvas wrapper framework completely covering viewport space
            return new_window_renderer->Render() | flex;
        }
    });

    // Start Interactive Event Processor Thread
    screen.Loop(final_renderer);
    return 0;
}
