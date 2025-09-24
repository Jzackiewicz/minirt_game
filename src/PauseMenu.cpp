#include "PauseMenu.hpp"
#include <algorithm>

PauseMenu::PauseMenu() : AMenu("PAUSE") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"RESUME", ButtonAction::Resume, MenuColors::PastelGreen});
    buttons.push_back(
        Button{"HOW TO PLAY", ButtonAction::HowToPlay, MenuColors::PastelYellow});
    buttons.push_back(
        Button{"LEADERBOARD", ButtonAction::Leaderboard, MenuColors::PastelBlue});
    buttons.push_back(Button{"SETTINGS", ButtonAction::Settings, MenuColors::PastelGray});
    buttons.push_back(Button{"QUIT", ButtonAction::Quit, MenuColors::PastelRed});
}

int PauseMenu::button_rows() const {
    if (buttons.empty())
        return 0;
    if (buttons.size() == 1)
        return 1;
    int remaining = static_cast<int>(buttons.size()) - 1;
    return 1 + (remaining + 1) / 2;
}

void PauseMenu::adjust_layout_metrics(float scale_factor, int &button_width,
                                      int &button_height, int &button_gap) {
    (void)scale_factor;
    button_width = static_cast<int>(button_width * 0.9f);
    button_height = static_cast<int>(button_height * 0.85f);
    button_gap = static_cast<int>(button_gap * 0.7f);
    if (button_width < 200)
        button_width = 200;
    if (button_height < 70)
        button_height = 70;
    if (button_gap < 6)
        button_gap = 6;
}

void PauseMenu::layout_buttons(std::vector<Button> &buttons_list, int width, int height,
                               float scale_factor, int button_width, int button_height,
                               int button_gap, int start_y, int center_x) {
    (void)height;
    (void)scale_factor;
    (void)center_x;
    if (buttons_list.size() < 2) {
        AMenu::layout_buttons(buttons_list, width, height, scale_factor, button_width,
                              button_height, button_gap, start_y, center_x);
        return;
    }

    int left_column_width = button_width;
    int right_column_width = button_width;
    int column_gap = std::max(button_gap, button_width / 10);
    if (column_gap < 1)
        column_gap = 1;
    int total_width = left_column_width + column_gap + right_column_width;
    int left_x = width / 2 - total_width / 2;
    int right_x = left_x + left_column_width + column_gap;
    int vertical_gap = button_gap;
    auto set_button = [&](std::size_t index, int x, int y, int w) {
        if (index >= buttons_list.size())
            return;
        buttons_list[index].rect = {x, y, w, button_height};
    };

    int current_y = start_y;
    set_button(0, left_x, current_y, total_width);
    current_y += button_height + vertical_gap;

    std::size_t index = 1;
    while (index < buttons_list.size()) {
        set_button(index, left_x, current_y, left_column_width);
        ++index;
        if (index < buttons_list.size()) {
            set_button(index, right_x, current_y, right_column_width);
            ++index;
        }
        current_y += button_height + vertical_gap;
    }
}

bool PauseMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    PauseMenu menu;
    ButtonAction action = menu.run(window, renderer, width, height, true);
    return action == ButtonAction::Resume;
}
