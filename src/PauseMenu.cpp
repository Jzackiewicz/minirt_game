#include "PauseMenu.hpp"
#include <algorithm>

PauseMenu::PauseMenu() : AMenu("PAUSE") {
    title_colors.assign(title.size(), SDL_Color{255, 255, 255, 255});
    buttons.push_back(Button{"RESUME", ButtonAction::Resume, MenuColors::PastelGreen});
    buttons.push_back(
        Button{"LEADERBOARD", ButtonAction::Leaderboard, MenuColors::PastelBlue});
    buttons.push_back(Button{"SETTINGS", ButtonAction::Settings, MenuColors::PastelGray});
    buttons.push_back(Button{"QUIT", ButtonAction::Quit, MenuColors::PastelRed});
    corner_buttons.push_back(
        Button{"HOW TO PLAY", ButtonAction::HowToPlay, MenuColors::PastelYellow});
}

int PauseMenu::button_rows() const {
    if (buttons.empty())
        return 0;
    return static_cast<int>((buttons.size() + 1) / 2);
}

void PauseMenu::button_metrics(float scale_factor, int &button_width, int &button_height,
                               int &button_gap) {
    button_width = static_cast<int>(button_width * 0.85f);
    button_height = static_cast<int>(button_height * 0.85f);
    int min_gap = static_cast<int>(6 * scale_factor);
    if (min_gap < 1)
        min_gap = 1;
    int adjusted_gap = static_cast<int>(button_gap * 0.6f);
    button_gap = std::max(min_gap, adjusted_gap);
}

void PauseMenu::layout_buttons(std::vector<Button> &buttons_list, int width, int height,
                               float scale_factor, int button_width, int button_height,
                               int button_gap, int start_y, int center_x) {
    (void)height;
    (void)center_x;
    if (buttons_list.size() < 2) {
        AMenu::layout_buttons(buttons_list, width, height, scale_factor, button_width,
                              button_height, button_gap, start_y, center_x);
        return;
    }

    int rows = button_rows();
    if (rows <= 0)
        return;

    int vertical_gap = button_gap;
    int base_column_gap = static_cast<int>(14 * scale_factor);
    if (base_column_gap < button_gap)
        base_column_gap = button_gap;

    std::vector<std::pair<int, int>> row_widths(static_cast<std::size_t>(rows),
                                                {button_width, button_width});

    layout_two_column(buttons_list, width, start_y, row_widths, button_height, vertical_gap,
                      base_column_gap);
}

bool PauseMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    PauseMenu menu;
    ButtonAction action = menu.run(window, renderer, width, height, true);
    return action == ButtonAction::Resume;
}
