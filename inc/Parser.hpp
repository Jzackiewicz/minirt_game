#pragma once
#include "Camera.hpp"
#include "Scene.hpp"
#include "material.hpp"
#include <string>
#include <vector>

class Parser
{
	public:
        static bool parse_rt_file(const std::string &path, Scene &outScene,
                                                          Camera &outCamera, int width, int height,
                                                          bool tutorial_mode = false);

	static const std::vector<Material> &get_materials();
	private:
	// TO JEST KLUCZOWE: deklaracja storage'u
	static std::vector<Material> materials;
};
