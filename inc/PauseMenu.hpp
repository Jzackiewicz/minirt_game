#pragma once
#include "AMenu.hpp"

struct SDL_Window;
struct SDL_Renderer;

// Menu shown when the game is paused
class PauseMenu : public AMenu {
protected:
    int button_rows() const override;
    void layout_buttons(std::vector<Button> &buttons, int width, int height,
                        float scale_factor, int button_width, int button_height,
                        int button_gap, int start_y, int center_x) override;
    void adjust_layout_metrics(float scale_factor, int &button_width, int &button_height,
                               int &button_gap) override;

public:
    PauseMenu();
    static bool show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};
