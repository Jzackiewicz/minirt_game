#include "MainMenu.hpp"
#include <SDL.h>

MainMenu::MainMenu() : AMenu("MINIRT THE GAME") {
    buttons.push_back(Button{"PLAY", ButtonAction::Play, MenuColors::PastelGreen});
    buttons.push_back(
        Button{"LEADERBOARD", ButtonAction::Leaderboard, MenuColors::PastelBlue});
    buttons.push_back(Button{"SETTINGS", ButtonAction::Settings, MenuColors::PastelYellow});
    buttons.push_back(Button{"QUIT", ButtonAction::Quit, MenuColors::PastelRed});
    corner_buttons.push_back(
        Button{"HOW TO PLAY", ButtonAction::HowToPlay, MenuColors::PastelGray});
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
