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
// Slider with discrete values
// -----------------------------------------------------------------------------
class Slider {
public:
    std::vector<std::string> values; // Available values displayed as strings
    int selected;                    // Currently chosen index
    SDL_Rect bar_rect{};             // Background bar rectangle
    SDL_Rect knob_rect{};            // Draggable knob rectangle
    SDL_Rect value_rect{};           // Rectangle displaying the current value
    bool dragging;                   // True while the user is dragging the knob
    int text_scale;                  // Scale used for value text

    Slider(const std::vector<std::string> &vals, int default_index = 0);

    // Layout slider within the given rectangle
    void layout(int x, int y, int width, int height, int scale);

    // Process mouse events
    void handle_event(const SDL_Event &event);

    // Draw the slider and the currently selected value
    void draw(SDL_Renderer *renderer) const;

    const std::string &current() const { return values[selected]; }
private:
    void update_knob();
    int index_from_pos(int x) const;
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

    char current() const;
    void layout(int x, int y, int width, int height, int scale) override;
    void handle_event(const SDL_Event &event) override;
    void draw(SDL_Renderer *renderer, int scale) const override;
};

// Section using a slider for mouse sensitivity
class MouseSensitivitySection : public SettingsSection {
    Slider slider;

public:
    MouseSensitivitySection();

    float current() const;
    void layout(int x, int y, int width, int height, int scale) override;
    void handle_event(const SDL_Event &event) override;
    void draw(SDL_Renderer *renderer, int scale) const override;
};

// Section using a slider for screen resolution
class ResolutionSection : public SettingsSection {
    Slider slider;

public:
    ResolutionSection();

    std::string current() const;
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
    static void show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                     bool transparent = false);

    // Run loop for the settings menu (hides AMenu::run)
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                    bool transparent = false);
};

