#pragma once
#include "BVH.hpp"
#include "Hittable.hpp"
#include "light.hpp"
#include "material.hpp"
#include <memory>
#include <vector>

class Camera;

class Scene
{
	public:
	std::vector<HittablePtr> objects;
	std::vector<PointLight> lights;
	Ambient ambient{Vec3(1, 1, 1), 0.0};
	std::shared_ptr<Hittable> accel;

	// Update beam objects and associated lights in the scene.
	void update_beams(const std::vector<Material> &materials);

	// Build bounding volume hierarchy for static geometry.
	void build_bvh();

	// Test a ray against all objects.
	bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const;

	// Determine whether object at index collides with others.
	bool collides(int index) const;

	// Move object while preventing collisions.
	Vec3 move_with_collision(int index, const Vec3 &delta);

	// Move camera while avoiding obstacles.
	Vec3 move_camera(Camera &cam, const Vec3 &delta,
					 const std::vector<Material> &materials) const;
	private:
	bool is_movable(int index) const;
	void apply_translation(const HittablePtr &object, const Vec3 &delta);
	void attempt_axis_move(int index, const Vec3 &axis_delta, Vec3 &moved);
};
