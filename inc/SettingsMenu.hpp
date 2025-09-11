//
// Settings menu and supporting classes
//

#pragma once
#include "AMenu.hpp"
#include <vector>
#include <string>

struct SDL_Window;
struct SDL_Renderer;

// -----------------------------------------------------------------------------
// A cluster of switch-like buttons. Only one button can be pressed at a time.
// -----------------------------------------------------------------------------
class ButtonsCluster {
public:
    std::vector<Button> buttons; // Buttons displayed next to each other
    int selected;                // Index of the currently pressed button

    ButtonsCluster(const std::vector<std::string> &labels);

    // Distribute buttons evenly in the given rectangle
    void layout(int x, int y, int width, int height, int gap);

    // Handle mouse click events
    void handle_event(const SDL_Event &event);

    // Draw the buttons
    void draw(SDL_Renderer *renderer, int scale) const;
};

// -----------------------------------------------------------------------------
// Slider with discrete values represented as evenly spaced ticks
// -----------------------------------------------------------------------------
class Slider {
    std::vector<std::string> values; // textual representation of options
    int selected;                    // currently chosen value
    SDL_Rect track_rect{};           // drawable slider bar
    SDL_Point value_pos{};           // position for value text
    bool dragging;                   // true while the knob is being dragged

    void update_from_position(int x); // translate mouse x into selected index

public:
    Slider(const std::vector<std::string> &vals, int initial = 0);

    // Layout the slider inside the given rectangle
    void layout(int x, int y, int width, int height, int scale);

    // Handle mouse interaction events
    void handle_event(const SDL_Event &event);

    // Draw the slider
    void draw(SDL_Renderer *renderer, int scale) const;

    const std::string &current() const { return values[selected]; }
};

// -----------------------------------------------------------------------------
// Base class for all settings sections. Each section has a label and an area
// below it for its content.
// -----------------------------------------------------------------------------
class SettingsSection {
protected:
    std::string label;
    SDL_Rect content_rect{}; // Area reserved for the section's content

public:
    explicit SettingsSection(const std::string &text);
    virtual ~SettingsSection() = default;

    // Compute layout of the section inside the given rectangle
    virtual void layout(int x, int y, int width, int height, int scale);

    // Handle events directed at the section
    virtual void handle_event(const SDL_Event &event) { (void)event; }

    // Draw the section
    virtual void draw(SDL_Renderer *renderer, int scale) const;
};

// -----------------------------------------------------------------------------
// Quality section consisting of a button cluster
// -----------------------------------------------------------------------------
class QualitySection : public SettingsSection {
    ButtonsCluster cluster;

public:
    QualitySection();

    void layout(int x, int y, int width, int height, int scale) override;
    void handle_event(const SDL_Event &event) override;
    void draw(SDL_Renderer *renderer, int scale) const override;
};

class MouseSensitivitySection : public SettingsSection {
    Slider slider;

public:
    MouseSensitivitySection();

    void layout(int x, int y, int width, int height, int scale) override;
    void handle_event(const SDL_Event &event) override;
    void draw(SDL_Renderer *renderer, int scale) const override;
};

class ResolutionSection : public SettingsSection {
    Slider slider;

public:
    ResolutionSection();

    void layout(int x, int y, int width, int height, int scale) override;
    void handle_event(const SDL_Event &event) override;
    void draw(SDL_Renderer *renderer, int scale) const override;
};

// -----------------------------------------------------------------------------
// Settings menu containing three sections and bottom buttons
// -----------------------------------------------------------------------------
class SettingsMenu : public AMenu {
    QualitySection quality;
    MouseSensitivitySection mouse_sensitivity;
    ResolutionSection resolution;

public:
    SettingsMenu();
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height);

    // Run loop for the settings menu (hides AMenu::run)
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height);
};

