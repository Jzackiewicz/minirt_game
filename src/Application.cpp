#include "Application.hpp"
#include "Camera.hpp"
#include "Parser.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "Settings.hpp"
#include <filesystem>
#include <iostream>
#include <thread>

// Launch the rendering pipeline and display the interactive window.
void run_application(const std::string &scene_path, int width, int height,
                                         char quality)
{
        std::string current_scene = scene_path;
        int cur_width = width;
        int cur_height = height;
        char cur_quality = quality;
        g_reload_requested = false;

        for (;;)
        {
                unsigned int thread_count = std::thread::hardware_concurrency();
                if (thread_count == 0)
                {
                        thread_count = 8;
                }
                float downscale = 1.0f;
                if (cur_quality == 'M' || cur_quality == 'm')
                {
                        downscale = 1.5f;
                }
                else if (cur_quality == 'L' || cur_quality == 'l')
                {
                        downscale = 2.5f;
                }
                Scene scene;
                Camera camera({0, 0, -10}, {0, 0, 0}, 60.0,
                                          static_cast<double>(cur_width) /
                                                  static_cast<double>(cur_height));
                bool parsed = Parser::parse_rt_file(current_scene, scene, camera,
                                                     cur_width, cur_height);
                if (!parsed)
                {
                        std::cerr << "Failed to parse scene: " << current_scene
                                  << "\n";
                        return;
                }
                if (current_scene.size() >= 4 &&
                    current_scene.substr(current_scene.size() - 4) == ".tmp")
                {
                        std::filesystem::remove(current_scene);
                }
                std::vector<Material> materials = Parser::get_materials();

                scene.update_beams(materials);
                scene.build_bvh();
                RenderSettings render_settings;
                render_settings.width = cur_width;
                render_settings.height = cur_height;
                render_settings.threads = thread_count;
                render_settings.downscale = downscale;
                Renderer renderer(scene, camera);
                renderer.render_window(materials, render_settings, current_scene);

                if (g_reload_scene_path.empty())
                        break;

                current_scene = g_reload_scene_path;
                g_reload_scene_path.clear();
                cur_width = g_settings.width;
                cur_height = g_settings.height;
                cur_quality = g_settings.quality;
        }
}
