#pragma once

struct SDL_Window;
struct SDL_Renderer;

/**
 * Displays the game's pause menu.
 *
 * @param window The game window.
 * @param renderer The renderer associated with the window.
 * @param width Window width.
 * @param height Window height.
 * @return True if the user chose to resume the game. False if they quit.
*/
class PauseMenu
{
public:
    static bool show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};

