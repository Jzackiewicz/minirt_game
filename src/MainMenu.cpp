#include "MainMenu.hpp"
#include <SDL.h>
#include <algorithm>

MainMenu::MainMenu() : AMenu("MINIRT THE GAME") {
    buttons.push_back(Button{"PLAY", ButtonAction::Play, MenuColors::PastelGreen});
    buttons.push_back(
        Button{"TUTORIAL", ButtonAction::Tutorial, MenuColors::PastelPurple});
    buttons.push_back(
        Button{"HOW TO PLAY", ButtonAction::HowToPlay, MenuColors::PastelYellow});
    buttons.push_back(
        Button{"LEADERBOARD", ButtonAction::Leaderboard, MenuColors::PastelBlue});
    buttons.push_back(
        Button{"SETTINGS", ButtonAction::Settings, MenuColors::PastelGray});
    buttons.push_back(Button{"QUIT", ButtonAction::Quit, MenuColors::PastelRed});
}

int MainMenu::button_rows() const {
    if (buttons.empty())
        return 0;
    return static_cast<int>((buttons.size() + 1) / 2);
}

void MainMenu::button_metrics(float scale_factor, int &button_width, int &button_height,
                              int &button_gap) {
    button_width = static_cast<int>(button_width * 0.85f);
    button_height = static_cast<int>(button_height * 0.85f);
    int min_gap = static_cast<int>(6 * scale_factor);
    if (min_gap < 1)
        min_gap = 1;
    int adjusted_gap = static_cast<int>(button_gap * 0.6f);
    button_gap = std::max(min_gap, adjusted_gap);
}

void MainMenu::layout_buttons(std::vector<Button> &buttons_list, int width, int height,
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
    if (!row_widths.empty()) {
        int tutorial_width = (button_width * 3) / 4;
        if (tutorial_width < 1)
            tutorial_width = 1;
        row_widths.front().second = tutorial_width;
    }

    layout_two_column(buttons_list, width, start_y, row_widths, button_height, vertical_gap,
                      base_column_gap);
}

bool MainMenu::show(int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return false;
    }
    SDL_Window *window = SDL_CreateWindow("MiniRT", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, width, height, 0);
    if (!window) {
        SDL_Quit();
        return false;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    MainMenu menu;
    ButtonAction action = menu.run(window, renderer, width, height);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return action == ButtonAction::Play;
}
