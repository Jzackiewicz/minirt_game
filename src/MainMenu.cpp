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
    int minimum_gap = std::max(10, left_column_width / 18);
    int column_gap = std::max(button_gap, minimum_gap);
    int total_width = left_column_width + column_gap + right_column_width;
    int left_x = width / 2 - total_width / 2;
    int right_x = left_x + left_column_width + column_gap;
    int tutorial_width = std::max(1, (left_column_width * 3) / 4);
    auto set_button = [&](std::size_t index, int x, int y, int w) {
        if (index >= buttons_list.size())
            return;
        buttons_list[index].rect = {x, y, w, button_height};
    };

    for (int row = 0; row < rows; ++row) {
        int y = start_y + row * (button_height + button_gap);
        std::size_t left_index = static_cast<std::size_t>(row * 2);
        std::size_t right_index = left_index + 1;
        set_button(left_index, left_x, y, left_column_width);
        if (right_index < buttons_list.size()) {
            int width_for_button = right_column_width;
            int x_position = right_x;
            if (buttons_list[right_index].action == ButtonAction::Tutorial) {
                width_for_button = tutorial_width;
                x_position += (right_column_width - width_for_button) / 2;
            }
            set_button(right_index, x_position, y, width_for_button);
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
