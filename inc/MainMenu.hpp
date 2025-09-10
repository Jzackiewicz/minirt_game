#pragma once
#include "AMenu.hpp"

// Main menu displayed before starting the game
class MainMenu : public AMenu {
public:
    MainMenu();
    static bool show(int width, int height);
};
