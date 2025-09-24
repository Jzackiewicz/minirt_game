#include "MainMenu.hpp"
#include <SDL.h>

MainMenu::MainMenu() : AMenu("MINIRT THE GAME") {
    buttons.push_back(Button{"PLAY", ButtonAction::Play, MenuColors::PastelGreen});
    buttons.push_back(Button{"TUTORIAL", ButtonAction::Tutorial, MenuColors::PastelPurple});
    buttons.push_back(Button{"HOW TO PLAY", ButtonAction::HowToPlay, MenuColors::PastelGray});
    buttons.push_back(
        Button{"LEADERBOARD", ButtonAction::Leaderboard, MenuColors::PastelBlue});
    buttons.push_back(Button{"SETTINGS", ButtonAction::Settings, MenuColors::PastelYellow});
    buttons.push_back(Button{"QUIT", ButtonAction::Quit, MenuColors::PastelRed});
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

void MainMenu::layout_buttons(int width, int height, int scale, int button_width,
                              int button_height, int button_gap, int start_y,
                              int center_x) {
    (void)height;
    (void)scale;
    (void)center_x;
    if (buttons.size() != 6) {
        AMenu::layout_buttons(width, height, scale, button_width, button_height, button_gap,
                              start_y, center_x);
        return;
    }

    int column_gap = button_gap;
    int left_width = button_width;
    int right_max_width = button_width;
    int tutorial_width = (button_width * 3) / 4;
    int layout_width = left_width + column_gap + right_max_width;
    int left_x = (width - layout_width) / 2;
    int right_x = left_x + left_width + column_gap;
    int tutorial_x = right_x + (right_max_width - tutorial_width) / 2;

    int row_y = start_y;
    buttons[0].rect = {left_x, row_y, left_width, button_height};
    buttons[1].rect = {tutorial_x, row_y, tutorial_width, button_height};

    row_y += button_height + button_gap;
    buttons[2].rect = {left_x, row_y, left_width, button_height};
    buttons[3].rect = {right_x, row_y, right_max_width, button_height};

    row_y += button_height + button_gap;
    buttons[4].rect = {left_x, row_y, left_width, button_height};
    buttons[5].rect = {right_x, row_y, right_max_width, button_height};
}
