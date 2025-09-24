#pragma once
#include "AMenu.hpp"

// Main menu displayed before starting the game
class MainMenu : public AMenu {
protected:
    int button_rows() const override;
    void button_metrics(float scale_factor, int &button_width, int &button_height,
                        int &button_gap) override;
    void layout_buttons(std::vector<Button> &buttons, int width, int height,
                        float scale_factor, int button_width, int button_height,
                        int button_gap, int start_y, int center_x) override;

public:
    MainMenu();
    static bool show(int width, int height);
};
