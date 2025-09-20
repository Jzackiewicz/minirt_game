#pragma once

#include "Vec3.hpp"

class Scene;
class Material;
class HitRecord;

Vec3 sample_surface_color(const Scene &scene, const HitRecord &rec, const Material &mat);
double compute_effective_alpha(const Material &mat, const HitRecord &rec);

