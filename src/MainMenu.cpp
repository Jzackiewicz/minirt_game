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

void MainMenu::adjust_layout_metrics(float scale_factor, int &button_width,
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

void MainMenu::layout_buttons(std::vector<Button> &buttons_list, int width, int height,
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

    int rows = button_rows();
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

    for (int row = 0; row < rows; ++row) {
        int y = start_y + row * (button_height + vertical_gap);
        std::size_t left_index = static_cast<std::size_t>(row * 2);
        std::size_t right_index = left_index + 1;
        set_button(left_index, left_x, y, left_column_width);
        if (right_index < buttons_list.size()) {
            set_button(right_index, right_x, y, right_column_width);
        }
    }
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
