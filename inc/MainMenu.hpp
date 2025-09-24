#pragma once
#include "AMenu.hpp"

// Main menu displayed before starting the game
class MainMenu : public AMenu {
protected:
    int compute_total_buttons_height(int button_height, int button_gap) const override;
    void layout_buttons(int width, int height, int start_y, int button_width,
                        int button_height, int button_gap) override;

public:
    MainMenu();
    static bool show(int width, int height);
};
