#pragma once
#include "Button.hpp"
#include <vector>

class ButtonsCluster {
    std::vector<Button> buttons;
    int selected;
public:
    ButtonsCluster(const std::vector<std::string> &names, int initial = 0);
    void set_rect(int x, int y, int w, int h, int gap);
    void handle_event(const SDL_Event &e);
    void render(SDL_Renderer *renderer, int scale) const;
    int current_index() const { return selected; }
    const std::string &current_text() const { return buttons[selected].text; }
};
