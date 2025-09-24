#include "MainMenu.hpp"
#include <SDL.h>

MainMenu::MainMenu() : AMenu("MINIRT THE GAME") {
    buttons.push_back(Button{"Play", ButtonAction::Play, MenuColors::PastelGreen});
    buttons.push_back(
        Button{"Tutorial", ButtonAction::Tutorial, MenuColors::PastelPurple});
    buttons.push_back(
        Button{"How to play", ButtonAction::HowToPlay, MenuColors::PastelGray});
    buttons.push_back(
        Button{"Leaderboard", ButtonAction::Leaderboard, MenuColors::PastelBlue});
    buttons.push_back(
        Button{"Settings", ButtonAction::Settings, MenuColors::PastelYellow});
    buttons.push_back(Button{"Quit", ButtonAction::Quit, MenuColors::PastelRed});
}

int MainMenu::layout_total_height(int button_height, int button_gap) const {
    if (buttons.size() < 6) {
        return AMenu::layout_total_height(button_height, button_gap);
    }
    int rows = 3;
    return rows * button_height + (rows - 1) * button_gap;
}

void MainMenu::layout_buttons(int width, int height, int button_width, int button_height,
                              int button_gap, int start_y, int center_x,
                              float scale_factor) {
    if (buttons.size() < 6) {
        AMenu::layout_buttons(width, height, button_width, button_height, button_gap, start_y,
                              center_x, scale_factor);
        return;
    }

    int column_gap = button_gap;
    int grid_width = button_width * 2 + column_gap;
    if (grid_width > width) {
        column_gap = width - button_width * 2;
        if (column_gap < 0) {
            column_gap = 0;
        }
        grid_width = button_width * 2 + column_gap;
        if (grid_width > width) {
            AMenu::layout_buttons(width, height, button_width, button_height, button_gap,
                                  start_y, center_x, scale_factor);
            return;
        }
    }

    int left_x = (width - grid_width) / 2;
    if (left_x < 0) {
        left_x = 0;
    }
    int right_slot_x = left_x + button_width + column_gap;

    int tutorial_width = static_cast<int>(static_cast<float>(button_width) * 0.75f);
    if (tutorial_width < 1) {
        tutorial_width = 1;
    }
    int tutorial_x = right_slot_x + (button_width - tutorial_width) / 2;

    auto row_y = [&](int row) {
        return start_y + row * (button_height + button_gap);
    };

    buttons[0].rect = {left_x, row_y(0), button_width, button_height};
    buttons[1].rect = {tutorial_x, row_y(0), tutorial_width, button_height};
    buttons[2].rect = {left_x, row_y(1), button_width, button_height};
    buttons[3].rect = {right_slot_x, row_y(1), button_width, button_height};
    buttons[4].rect = {left_x, row_y(2), button_width, button_height};
    buttons[5].rect = {right_slot_x, row_y(2), button_width, button_height};
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
