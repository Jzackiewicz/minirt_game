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
    return static_cast<int>((buttons.size() + 1) / 2);
}

void PauseMenu::adjust_button_metrics(float, int &button_width, int &button_height,
                                      int &button_gap) const {
    button_width = static_cast<int>(button_width * 0.9f);
    button_height = static_cast<int>(button_height * 0.85f);
    button_gap = static_cast<int>(button_gap * 0.6f);
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
    int vertical_gap = std::max(1, button_gap);
    int column_gap = std::max(vertical_gap, left_column_width / 12);
    layout_two_column_grid(buttons_list, width, button_height, vertical_gap, start_y,
                           left_column_width, right_column_width, column_gap);
}

bool PauseMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    PauseMenu menu;
    ButtonAction action = menu.run(window, renderer, width, height, true);
    return action == ButtonAction::Resume;
}
