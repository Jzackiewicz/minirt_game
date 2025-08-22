#pragma once
#include "Scene.h"
#include "Camera.h"
#include "material.h"
#include <string>
#include <vector>

namespace rt {

struct RenderSettings {
    int width = 800;
    int height = 600;
    int threads = 0; // 0 => auto
};

class Renderer {
public:
    Renderer(const Scene& s, const Camera& c);
    void render_ppm(const std::string& path,
                    const std::vector<Material>& mats,
                    const RenderSettings& rset);
    void render_window(const std::vector<Material>& mats,
                       const RenderSettings& rset);
private:
    const Scene& scene;
    const Camera& cam;
};

} // namespace rt
