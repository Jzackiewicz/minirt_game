#pragma once

#include "Sphere.hpp"

class LightMarker : public Sphere
{
        public:
        LightMarker(const Vec3 &c, double r, int oid, int mid) : Sphere(c, r, oid, mid) {}
        ShapeType shape_type() const override { return ShapeType::LightMarker; }
        bool casts_shadow() const override { return false; }
};
