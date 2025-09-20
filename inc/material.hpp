
#pragma once
#include "Vec3.hpp"
#include "light.hpp"
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
	int texture_width = 0;
	int texture_height = 0;
	std::vector<Vec3> texture_data;
	std::string texture_path;

	bool has_texture() const;
	Vec3 sample_texture(double u, double v) const;
};

Vec3 phong(const Material &m, const Ambient &ambient,
		   const std::vector<PointLight> &lights, const Vec3 &p, const Vec3 &n,
		   const Vec3 &eye);
