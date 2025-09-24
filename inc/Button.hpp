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
    HowToPlay,
    Tutorial,
    Back,
    Quit
};

namespace MenuColors {
constexpr SDL_Color PastelGreen{96, 255, 128, 255};
constexpr SDL_Color PastelBlue{96, 128, 255, 255};
constexpr SDL_Color PastelYellow{255, 224, 128, 255};
constexpr SDL_Color PastelRed{255, 96, 96, 255};
constexpr SDL_Color PastelGray{176, 176, 176, 255};
constexpr SDL_Color PastelPurple{200, 160, 255, 255};
} // namespace MenuColors

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
