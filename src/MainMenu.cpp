#include "MainMenu.hpp"
#include <SDL.h>
#include <cstddef>

MainMenu::MainMenu() : AMenu("MINIRT THE GAME") {
    buttons.push_back(Button{"PLAY", ButtonAction::Play, MenuColors::PastelGreen});
    buttons.push_back(Button{"Tutorial", ButtonAction::None, MenuColors::PastelPurple});
    buttons.push_back(Button{"How to play", ButtonAction::HowToPlay, MenuColors::PastelGray});
    buttons.push_back(
        Button{"Leaderboard", ButtonAction::Leaderboard, MenuColors::PastelBlue});
    buttons.push_back(Button{"Settings", ButtonAction::Settings, MenuColors::PastelYellow});
    buttons.push_back(Button{"Quit", ButtonAction::Quit, MenuColors::PastelRed});
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

int MainMenu::compute_total_buttons_height(int button_height, int button_gap) const {
    if (buttons.size() == 6)
        return 3 * button_height + 2 * button_gap;
    return AMenu::compute_total_buttons_height(button_height, button_gap);
}

void MainMenu::layout_buttons(int width, int height, int start_y, int button_width,
                              int button_height, int button_gap) {
    if (buttons.size() != 6) {
        AMenu::layout_buttons(width, height, start_y, button_width, button_height, button_gap);
        return;
    }

    (void)height;
    int column_gap = button_gap * 2;
    int total_width = button_width * 2 + column_gap;
    if (total_width > width) {
        column_gap = button_gap;
        total_width = button_width * 2 + column_gap;
        if (total_width > width) {
            AMenu::layout_buttons(width, height, start_y, button_width, button_height,
                                  button_gap);
            return;
        }
    }

    int left_x = width / 2 - total_width / 2;
    int right_x_base = left_x + button_width + column_gap;
    if (left_x < 0 || right_x_base + button_width > width) {
        AMenu::layout_buttons(width, height, start_y, button_width, button_height, button_gap);
        return;
    }

    int right_center = right_x_base + button_width / 2;
    int rows = 3;
    for (int row = 0; row < rows; ++row) {
        int y = start_y + row * (button_height + button_gap);
        std::size_t left_index = static_cast<std::size_t>(row * 2);
        std::size_t right_index = left_index + 1;
        if (right_index >= buttons.size())
            break;

        auto &left_button = buttons[left_index];
        left_button.rect = {left_x, y, button_width, button_height};

        auto &right_button = buttons[right_index];
        int right_width = button_width;
        if (row == 0) {
            right_width = (button_width * 3) / 4;
            if (right_width < 1)
                right_width = 1;
        }
        int right_x = right_center - right_width / 2;
        right_button.rect = {right_x, y, right_width, button_height};
    }
}
