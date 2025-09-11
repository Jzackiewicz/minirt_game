#pragma once
#include <SDL.h>
#include <string>
#include "ButtonsCluster.hpp"
#include "CustomCharacter.hpp"

// Base class for settings sections with a label
class SettingsSection {
protected:
    std::string label;

public:
    explicit SettingsSection(const std::string &l);
    virtual ~SettingsSection() = default;
    virtual void handle_event(const SDL_Event &event) {}
    virtual void draw(SDL_Renderer *renderer, int x, int y, int width, int scale);
};

// Section with quality preset buttons
class QualitySection : public SettingsSection {
    ButtonsCluster cluster;

public:
    QualitySection();
    void handle_event(const SDL_Event &event) override;
    void draw(SDL_Renderer *renderer, int x, int y, int width, int scale) override;
};

// Placeholder sections for mouse sensitivity and resolution
class MouseSensitivitySection : public SettingsSection {
public:
    MouseSensitivitySection();
    void draw(SDL_Renderer *renderer, int x, int y, int width, int scale) override;
};

class ResolutionSection : public SettingsSection {
public:
    ResolutionSection();
    void draw(SDL_Renderer *renderer, int x, int y, int width, int scale) override;
};
