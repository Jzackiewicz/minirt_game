#pragma once
#include <SDL.h>
#include <string>
#include "ButtonsCluster.hpp"
#include "CustomCharacter.hpp"

// Base class for a section in the settings menu
class SettingsSection {
protected:
    std::string label;

public:
    explicit SettingsSection(const std::string &l) : label(l) {}
    virtual ~SettingsSection() = default;

    virtual void handle_event(const SDL_Event &e) {}

    // Draw section at given position. Returns bottom y position after drawing.
    virtual int draw(SDL_Renderer *renderer, int center_x, int y, int width,
                     int scale) = 0;
};

// Section with quality buttons
class QualitySection : public SettingsSection {
private:
    ButtonsCluster cluster;
    int btn_height;

public:
    QualitySection();
    int layout(int center_x, int y, int width, int scale);
    void handle_event(const SDL_Event &e) override { cluster.handle_event(e); }
    int draw(SDL_Renderer *renderer, int center_x, int y, int width,
             int scale) override;
};

// Placeholder for mouse sensitivity slider
class MouseSensitivitySection : public SettingsSection {
public:
    MouseSensitivitySection();
    int draw(SDL_Renderer *renderer, int center_x, int y, int width,
             int scale) override;
};

// Placeholder for resolution slider
class ResolutionSection : public SettingsSection {
public:
    ResolutionSection();
    int draw(SDL_Renderer *renderer, int center_x, int y, int width,
             int scale) override;
};
