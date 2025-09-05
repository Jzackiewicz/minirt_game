#include "Application.hpp"
#include "Camera.hpp"
#include "Parser.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include <iostream>
#include <thread>

void run_application(const std::string &scene_path, int width, int height, char quality)
{
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
	rt::Scene scene;
	rt::Camera camera({0, 0, -10}, {0, 0, 0}, 60.0, static_cast<double>(width) / static_cast<double>(height));
	bool parsed;
	parsed = rt::Parser::parse_rt_file(scene_path, scene, camera, width, height);
	if (!parsed)
	{
		std::cerr << "Failed to parse scene: " << scene_path << "\n";
		return;
	}
	std::vector<rt::Material> materials;
	materials = rt::Parser::get_materials();

	scene.update_beams(materials);
	scene.build_bvh();
	rt::RenderSettings render_settings;
	render_settings.width = width;
	render_settings.height = height;
	render_settings.threads = thread_count;
	render_settings.downscale = downscale;
	rt::Renderer renderer(scene, camera);
	renderer.render_window(materials, render_settings, scene_path);
}

