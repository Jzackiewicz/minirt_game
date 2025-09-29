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

struct SessionProgress
{
        bool has_progress = false;
        bool tutorial_mode = false;
        std::string next_scene_path;
        double cumulative_score = 0.0;
        int completed_levels = 0;
        std::string player_name;
};

class Renderer
{
        public:
        Renderer(Scene &s, Camera &c);
        void render_ppm(const std::string &path, const std::vector<Material> &mats,
                                        const RenderSettings &rset);
        bool render_window(std::vector<Material> &mats, const RenderSettings &rset,
                                           const std::string &scene_path, bool tutorial_mode,
                                           SessionProgress *progress);
                struct RenderState;
        private:
        void mark_scene_dirty(RenderState &st);
        bool init_sdl(SDL_Window *&win, SDL_Renderer *&ren, SDL_Texture *&tex,
                                       int W, int H, int RW, int RH);
        void process_events(RenderState &st, SDL_Window *win, SDL_Renderer *ren,
                                               int W, int H, std::vector<Material> &mats);
        void handle_keyboard(RenderState &st, double dt,
                                               std::vector<Material> &mats);
        void update_selection(RenderState &st, std::vector<Material> &mats);
        void render_frame(RenderState &st, SDL_Renderer *ren, SDL_Texture *tex,
                                          std::vector<Vec3> &framebuffer,
                                          std::vector<unsigned char> &pixels, int RW,
                                          int RH, int W, int H, int T,
                                          std::vector<Material> &mats);
        int render_hud(const RenderState &st, SDL_Renderer *ren, int W, int H);
        Scene &scene;
        Camera &cam;
};
