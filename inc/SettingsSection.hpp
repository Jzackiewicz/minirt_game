#pragma once
#include <SDL.h>
#include <string>
#include "ButtonsCluster.hpp"
#include "CustomCharacter.hpp"

class SettingsSection {
protected:
    std::string label;
    SDL_Rect area;

public:
    explicit SettingsSection(const std::string &l);
    virtual ~SettingsSection() = default;
    void set_area(int x, int y, int w, int h);
    virtual void layout(int scale);
    virtual void render(SDL_Renderer *renderer, int scale) const = 0;
    virtual void handle_event(const SDL_Event &event);
};

class QualitySection : public SettingsSection {
    ButtonsCluster cluster;

public:
    QualitySection();
    void layout(int scale) override;
    void render(SDL_Renderer *renderer, int scale) const override;
    void handle_event(const SDL_Event &event) override;
};

class MouseSensitivitySection : public SettingsSection {
public:
    MouseSensitivitySection();
    void render(SDL_Renderer *renderer, int scale) const override;
};

class ResolutionSection : public SettingsSection {
public:
    ResolutionSection();
    void render(SDL_Renderer *renderer, int scale) const override;
};

