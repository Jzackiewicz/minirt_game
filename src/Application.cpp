#include "Application.hpp"
#include "Camera.hpp"
#include "Parser.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "Settings.hpp"
#include <iostream>
#include <thread>

// Launch the rendering pipeline and display the interactive window.
void run_application(const std::string &scene_path, int width, int height,
                                         char quality)
{
        std::string current_scene = scene_path;
        int cur_w = width;
        int cur_h = height;
        char cur_q = quality;

        for (;;)
        {
                g_reload_requested = false;
                unsigned int thread_count = std::thread::hardware_concurrency();
                if (thread_count == 0)
                        thread_count = 8;

                float downscale = 1.0f;
                if (cur_q == 'M' || cur_q == 'm')
                        downscale = 1.5f;
                else if (cur_q == 'L' || cur_q == 'l')
                        downscale = 2.5f;

                Scene scene;
                Camera camera({0, 0, -10}, {0, 0, 0}, 60.0,
                                      static_cast<double>(cur_w) / static_cast<double>(cur_h));
                bool parsed = Parser::parse_rt_file(current_scene, scene, camera,
                                                   cur_w, cur_h);
                if (!parsed)
                {
                        std::cerr << "Failed to parse scene: " << current_scene << "\n";
                        return;
                }
                std::vector<Material> materials = Parser::get_materials();

                scene.update_beams(materials);
                scene.build_bvh();
                RenderSettings render_settings;
                render_settings.width = cur_w;
                render_settings.height = cur_h;
                render_settings.threads = thread_count;
                render_settings.downscale = downscale;
                Renderer renderer(scene, camera);
                renderer.render_window(materials, render_settings, current_scene);

                if (!g_reload_requested)
                        break;

                current_scene = g_reload_scene_path.empty() ? current_scene
                                                            : g_reload_scene_path;
                cur_w = g_settings.width;
                cur_h = g_settings.height;
                cur_q = g_settings.quality;
        }
}
