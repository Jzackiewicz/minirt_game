#pragma once
#include "Camera.hpp"
#include "Scene.hpp"
#include "material.hpp"
#include <string>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class RenderSettings
{
	public:
	int width = 800;
	int height = 600;
	int threads = 0;		// 0 => auto
	float downscale = 1.0f; // 1.0 => full res, 1.5 => medium, 2.0 => low
};

class Renderer
{
	public:
	Renderer(Scene &s, Camera &c);
        void render_ppm(const std::string &path, const std::vector<Material> &mats,
                                        const RenderSettings &rset);
        void render_window(std::vector<Material> &mats, const RenderSettings &rset,
                                           const std::string &scene_path);
        private:
        struct RenderState;
        void mark_scene_dirty(RenderState &st);
        bool init_sdl(SDL_Window *&win, SDL_Renderer *&ren, SDL_Texture *&tex,
                                       int W, int H, int RW, int RH);
        void process_events(RenderState &st, SDL_Window *win, SDL_Renderer *ren,
                                               int W, int H, std::vector<Material> &mats,
                                               const std::string &scene_path);
        void handle_keyboard(RenderState &st, double dt,
                                               std::vector<Material> &mats);
        void update_selection(RenderState &st, std::vector<Material> &mats);
        void render_frame(RenderState &st, SDL_Renderer *ren, SDL_Texture *tex,
                                          std::vector<Vec3> &framebuffer,
                                          std::vector<unsigned char> &pixels, int RW,
                                          int RH, int W, int H, int T,
                                          std::vector<Material> &mats);
        Scene &scene;
        Camera &cam;
};
