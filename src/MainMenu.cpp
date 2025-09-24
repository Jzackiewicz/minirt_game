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

void MainMenu::adjust_button_metrics(float, int &button_width, int &button_height,
                                     int &button_gap) const {
    button_width = static_cast<int>(button_width * 0.9f);
    button_height = static_cast<int>(button_height * 0.85f);
    button_gap = static_cast<int>(button_gap * 0.6f);
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

    int left_column_width = button_width;
    int right_column_width = button_width;
    int vertical_gap = std::max(1, button_gap);
    int column_gap = std::max(vertical_gap, left_column_width / 12);
    auto adjust_rect = [&](std::size_t index, SDL_Rect &rect) {
        if (index < buttons_list.size() &&
            buttons_list[index].action == ButtonAction::Tutorial) {
            int tutorial_width = static_cast<int>(left_column_width * 0.75f);
            if (tutorial_width < 1)
                tutorial_width = 1;
            rect.x += (rect.w - tutorial_width) / 2;
            rect.w = tutorial_width;
        }
    };

    layout_two_column_grid(buttons_list, width, button_height, vertical_gap, start_y,
                           left_column_width, right_column_width, column_gap,
                           adjust_rect);
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
