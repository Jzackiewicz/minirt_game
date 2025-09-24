#pragma once
#include "AMenu.hpp"

// Main menu displayed before starting the game
class MainMenu : public AMenu {
protected:
    void layout_buttons(int width, int height, int button_width, int button_height,
                        int button_gap, int start_y, int center_x,
                        float scale_factor) override;
    int layout_total_height(int button_height, int button_gap) const override;

public:
    MainMenu();
    static bool show(int width, int height);
};
