
#pragma once
#include "Texture.hpp"
#include "Vec3.hpp"
#include "light.hpp"
#include <memory>
#include <string>
#include <vector>

#define REFLECTION 50

class Material
{
	public:
	Vec3 color;		 // current color used for rendering
	Vec3 base_color; // original color stored for edits
	double alpha = 1.0;
	double specular_exp = 50.0;
	double specular_k = 0.5;
	bool mirror = false;
	bool random_alpha = false;
	bool checkered = false; // render as checkered pattern when true
	std::shared_ptr<Texture> texture;
	std::string texture_path;

	bool has_texture() const { return static_cast<bool>(texture); }
};

Vec3 phong(const Material &m, const Ambient &ambient,
		   const std::vector<PointLight> &lights, const Vec3 &p, const Vec3 &n,
		   const Vec3 &eye);
