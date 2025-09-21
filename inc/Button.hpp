#pragma once
#include <SDL.h>
#include <string>

// Possible actions triggered by menu buttons
enum class ButtonAction {
    None,
    Play,
    Resume,
    NextLevel,
    Settings,
    Leaderboard,
    Back,
    Quit
};

// Represents an interactive button in a menu
class Button {
public:
    std::string text;
    ButtonAction action;
    SDL_Color hover_color;
    SDL_Rect rect;

    Button(const std::string &t, ButtonAction a, SDL_Color hover)
        : text(t), action(a), hover_color(hover), rect{0, 0, 0, 0} {}
};
