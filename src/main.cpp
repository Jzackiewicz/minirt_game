#include "Application.hpp"
#include "CommandLine.hpp"
#include "MainMenu.hpp"
#include "Settings.hpp"
#include <string>

/**
 * Program entry point.
 *
 */
int main(int argc, char **argv)
{
        std::string scene_path;
        int width;
        int height;
        char quality;
        bool parsed;
        parsed = parse_arguments(argc, argv, scene_path, width, height, quality);
        if (!parsed)
        {
                return 1;
        }
        load_settings("settings.yaml");
        Settings &cfg = get_settings();
        width = cfg.width;
        height = cfg.height;
        quality = cfg.quality;
        bool play;
        play = MainMenu::show(width, height);
        if (!play)
        {
                return 0;
        }
        run_application(scene_path, cfg.width, cfg.height, cfg.quality);
        return 0;
}
