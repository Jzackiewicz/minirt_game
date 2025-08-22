#pragma once
#include "Scene.h"
#include "Camera.h"
#include "material.h"
#include <vector>
#include <string>

namespace rt {

class Parser {
public:
    static bool parse_rt_file(const std::string& path,
                              Scene& outScene,
                              Camera& outCamera,
                              int width, int height);

    static const std::vector<Material>& get_materials();

private:
    // TO JEST KLUCZOWE: deklaracja storage'u
    static std::vector<Material> materials;
};

} // namespace rt
