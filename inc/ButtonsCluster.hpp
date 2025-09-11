#pragma once
#include <vector>
#include <string>
#include <SDL.h>
#include "Button.hpp"
#include "CustomCharacter.hpp"

class ButtonsCluster {
    std::vector<Button> buttons;
    int active_index;

public:
    explicit ButtonsCluster(const std::vector<std::string> &labels);
    void layout(int x, int y, int width, int height);
    void render(SDL_Renderer *renderer, int scale) const;
    void handle_event(const SDL_Event &event);
};

