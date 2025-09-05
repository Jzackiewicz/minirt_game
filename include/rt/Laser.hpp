#pragma once
#include <memory>
#include "LightRay.hpp"

namespace rt {
struct Laser {
  std::shared_ptr<LightRay> ray;
  explicit Laser(const std::shared_ptr<LightRay> &r) : ray(r) {}
};
} // namespace rt
