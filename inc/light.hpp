
#pragma once
#include "Vec3.hpp"
#include <vector>

class PointLight
{
	public:
	Vec3 position;
	Vec3 color;
        double intensity;
        std::vector<int> ignore_ids;
        int attached_id;
        Vec3 direction;
        double cutoff_cos;
        double range;
        bool reflected;
        double radius;

        PointLight(const Vec3 &p, const Vec3 &c, double i,
                           std::vector<int> ignore_ids = {}, int attached_id = -1,
                           const Vec3 &dir = Vec3(0, 0, 0), double cutoff_cos = -1.0,
                           double range = -1.0, bool reflected = false,
                           double radius = 0.0);
};

class Ambient
{
	public:
	Vec3 color;
	double intensity;

	Ambient(const Vec3 &c, double i);
};
