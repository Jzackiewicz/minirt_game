#pragma once
#include "AMenu.hpp"

// Main menu displayed before starting the game
class MainMenu : public AMenu {
public:
    MainMenu();
    static bool show(int width, int height);

protected:
    void layout_buttons(int width, int height, int scale_factor, int button_width,
                        int button_height, int button_gap, int start_y,
                        int center_x) override;
};
