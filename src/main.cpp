
#include "rt/Scene.h"
#include "rt/Camera.h"
#include "rt/Parser.h"    // single translation include for brevity
#include "rt/Renderer.h"  // same trick
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: minirt <scene.rt> <output.ppm> [width height threads]\n";
        return 1;
    }
    std::string scene_path = argv[1];
    std::string out_path = argv[2];
    int width = (argc > 3) ? std::atoi(argv[3]) : 800;
    int height = (argc > 4) ? std::atoi(argv[4]) : 600;
    int threads = (argc > 5) ? std::atoi(argv[5]) : std::thread::hardware_concurrency();

    rt::Scene scene;
    rt::Camera cam({0,0,-10}, {0,0,0}, 60.0, double(width)/double(height));
    if (!rt::Parser::parse_rt_file(scene_path, scene, cam, width, height)) {
        std::cerr << "Failed to parse scene: " << scene_path << "\n";
        return 2;
    }
    auto mats = rt::Parser::get_materials();
    scene.build_bvh();

    rt::RenderSettings rset;
    rset.width = width;
    rset.height = height;
    rset.threads = threads > 0 ? threads : 8;

    rt::Renderer renderer(scene, cam);
    renderer.render_ppm(out_path, mats, rset);

    std::cerr << "Wrote " << out_path << "\n";
    return 0;
}
