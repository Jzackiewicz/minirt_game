#include "Application.hpp"
#include "Camera.hpp"
#include "Parser.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include <iostream>
#include <thread>

// Launch the rendering pipeline and display the interactive window.
bool run_application(const std::string &scene_path, int width, int height,
                                        char quality, bool tutorial_mode,
                                        GameSession *session)
{
        std::string initial_scene_path = scene_path;
        if (session && session->has_progress &&
            session->tutorial_mode == tutorial_mode &&
            !session->next_scene_path.empty())
        {
                initial_scene_path = session->next_scene_path;
        }
        unsigned int thread_count;
        thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0)
        {
                thread_count = 8;
	}
	float downscale;
	downscale = 1.0f;
	if (quality == 'M' || quality == 'm')
	{
		downscale = 1.5f;
	}
	else if (quality == 'L' || quality == 'l')
	{
		downscale = 2.5f;
	}
	Scene scene;
	Camera camera({0, 0, -10}, {0, 0, 0}, 60.0,
				  static_cast<double>(width) / static_cast<double>(height));
	bool parsed;
        parsed = Parser::parse_rt_file(initial_scene_path, scene, camera, width, height);
        if (!parsed)
        {
                std::cerr << "Failed to parse scene: " << initial_scene_path << "\n";
                return false;
        }
        std::vector<Material> materials;
        materials = Parser::get_materials();

	scene.update_beams(materials);
	scene.build_bvh();
	RenderSettings render_settings;
	render_settings.width = width;
	render_settings.height = height;
	render_settings.threads = thread_count;
	render_settings.downscale = downscale;
	Renderer renderer(scene, camera);
        return renderer.render_window(materials, render_settings, initial_scene_path,
                                                              tutorial_mode, session);
}
