#include "Application.hpp"
#include "Camera.hpp"
#include "Parser.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

// Launch the rendering pipeline and display the interactive window.
void run_application(const std::string &scene_path, int width, int height,
                                         char quality)
{
        namespace fs = std::filesystem;

        unsigned int thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0)
        {
                thread_count = 8;
        }
        float downscale = 1.0f;
        if (quality == 'M' || quality == 'm')
        {
                downscale = 1.5f;
        }
        else if (quality == 'L' || quality == 'l')
        {
                downscale = 2.5f;
        }

        fs::path initial_path = fs::absolute(scene_path);
        fs::path scenes_dir = initial_path.parent_path();
        std::vector<std::string> level_paths;
        try
        {
                for (const auto &entry : fs::directory_iterator(scenes_dir))
                {
                        if (!entry.is_regular_file())
                                continue;
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                                return static_cast<char>(std::tolower(c));
                        });
                        if (ext == ".toml")
                                level_paths.push_back(fs::absolute(entry.path()).string());
                }
        }
        catch (const std::exception &)
        {
                // Ignore directory iteration errors; fall back to the provided scene.
        }
        level_paths.push_back(initial_path.string());
        std::sort(level_paths.begin(), level_paths.end());
        level_paths.erase(std::unique(level_paths.begin(), level_paths.end()), level_paths.end());
        std::string initial_str = initial_path.string();
        std::size_t level_index = 0;
        for (std::size_t i = 0; i < level_paths.size(); ++i)
        {
                if (level_paths[i] == initial_str)
                {
                        level_index = i;
                        break;
                }
        }

        RenderSettings render_settings;
        render_settings.width = width;
        render_settings.height = height;
        render_settings.threads = thread_count;
        render_settings.downscale = downscale;

        double accumulated_score = 0.0;

        while (level_index < level_paths.size())
        {
                const std::string &current_level = level_paths[level_index];
                Scene scene;
                Camera camera({0, 0, -10}, {0, 0, 0}, 60.0,
                                      static_cast<double>(width) / static_cast<double>(height));
                if (!Parser::parse_rt_file(current_level, scene, camera, width, height))
                {
                        std::cerr << "Failed to parse scene: " << current_level << "\n";
                        return;
                }
                std::vector<Material> materials = Parser::get_materials();
                scene.update_beams(materials);
                scene.build_bvh();

                Renderer renderer(scene, camera);
                Renderer::RenderResult result =
                    renderer.render_window(materials, render_settings, current_level,
                                            static_cast<int>(level_index),
                                            static_cast<int>(level_paths.size()),
                                            accumulated_score);

                if (result.action == ButtonAction::NextLevel && level_index + 1 < level_paths.size())
                {
                        accumulated_score += result.score;
                        ++level_index;
                        continue;
                }
                break;
        }
}
