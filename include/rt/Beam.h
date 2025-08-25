#pragma once
#include "Cylinder.h"

namespace rt {

struct Beam : public Cylinder {
    Beam(const Vec3& origin,
         const Vec3& dir,
         double radius,
         double length,
         int oid,
         int mid)
        : Cylinder(origin + dir.normalized() * (length * 0.5),
                   dir, radius, length, oid, mid) {}

    bool is_beam() const override { return true; }
};

} // namespace rt
