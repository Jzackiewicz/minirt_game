#include "Renderer.hpp"
#include "AABB.hpp"
#include "Config.hpp"
#include "Settings.hpp"
#include "Parser.hpp"
#include "PauseMenu.hpp"
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <thread>

static bool in_shadow(const Scene &scene, const std::vector<Material> &mats,
					  const Vec3 &p, const PointLight &L)
{
	Vec3 to_light = L.position - p;
	double dist_to_light = to_light.length();
	if (L.range > 0.0 && dist_to_light > L.range)
		return false;
	Vec3 dir = to_light.normalized();
	Ray shadow_ray(p + dir * 1e-4, dir);
	HitRecord tmp;
	for (const auto &obj : scene.objects)
	{
		if (obj->is_beam())
			continue;
		if (std::find(L.ignore_ids.begin(), L.ignore_ids.end(),
					  obj->object_id) != L.ignore_ids.end())
			continue;
		const Material &m = mats[obj->material_id];
		if (m.alpha < 1.0)
			continue;
		if (obj->hit(shadow_ray, 1e-4, dist_to_light - 1e-4, tmp))
		{
			return true;
		}
	}
	return false;
}

static Vec3 trace_ray(const Scene &scene, const std::vector<Material> &mats,
					  const Ray &r, std::mt19937 &rng,
					  std::uniform_real_distribution<double> &dist,
					  int depth = 0)
{
	if (depth > 10)
		return Vec3(0.0, 0.0, 0.0);
	HitRecord rec;
	if (!scene.hit(r, 1e-4, 1e9, rec))
	{
		return Vec3(0.0, 0.0, 0.0);
	}
	const Material &m = mats[rec.material_id];
	Vec3 eye = (r.dir * -1.0).normalized();
	Vec3 base = m.base_color;
	Vec3 col = m.color;
	if (m.checkered)
	{
		Vec3 inv = Vec3(1.0, 1.0, 1.0) - base;
		int chk = (static_cast<int>(std::floor(rec.p.x * 5)) +
				   static_cast<int>(std::floor(rec.p.y * 5)) +
				   static_cast<int>(std::floor(rec.p.z * 5))) &
				  1;
		col = chk ? base : inv;
	}
	Vec3 sum(col.x * scene.ambient.color.x * scene.ambient.intensity,
			 col.y * scene.ambient.color.y * scene.ambient.intensity,
			 col.z * scene.ambient.color.z * scene.ambient.intensity);
	for (const auto &L : scene.lights)
	{
		if (std::find(L.ignore_ids.begin(), L.ignore_ids.end(),
					  rec.object_id) != L.ignore_ids.end())
			continue;
		Vec3 to_light = L.position - rec.p;
		double dist = to_light.length();
		if (L.range > 0.0 && dist > L.range)
			continue;
		Vec3 ldir = to_light / dist;
		if (L.cutoff_cos > -1.0)
		{
			Vec3 spot_dir = (rec.p - L.position).normalized();
			if (Vec3::dot(L.direction, spot_dir) < L.cutoff_cos)
				continue;
		}
		if (in_shadow(scene, mats, rec.p, L))
			continue;
		double atten = 1.0;
		if (L.range > 0.0)
			atten = std::max(0.0, 1.0 - dist / L.range);
		double diff = std::max(0.0, Vec3::dot(rec.normal, ldir));
		Vec3 h = (ldir + eye).normalized();
		double spec =
			std::pow(std::max(0.0, Vec3::dot(rec.normal, h)), m.specular_exp) *
			m.specular_k;
		sum += Vec3(col.x * L.color.x * L.intensity * diff * atten +
						L.color.x * spec * atten,
					col.y * L.color.y * L.intensity * diff * atten +
						L.color.y * spec * atten,
					col.z * L.color.z * L.intensity * diff * atten +
						L.color.z * spec * atten);
	}
	if (m.mirror)
	{
		Vec3 refl_dir =
			r.dir - rec.normal * (2.0 * Vec3::dot(r.dir, rec.normal));
		Ray refl(rec.p + refl_dir * 1e-4, refl_dir);
		Vec3 refl_col = trace_ray(scene, mats, refl, rng, dist, depth + 1);
		double refl_ratio = REFLECTION / 100.0;
		sum = sum * (1.0 - refl_ratio) + refl_col * refl_ratio;
	}
	double alpha = m.alpha;
	if (m.random_alpha)
	{
		double tpos = std::clamp(rec.beam_ratio, 0.0, 1.0);
		alpha *= (1.0 - tpos);
	}
	if (alpha < 1.0)
	{
		Ray next(rec.p + r.dir * 1e-4, r.dir);
		Vec3 behind = trace_ray(scene, mats, next, rng, dist, depth + 1);
		return sum * alpha + behind * (1.0 - alpha);
	}
	return sum;
}

static std::string next_save_path(const std::string &orig)
{
	namespace fs = std::filesystem;
	fs::path p(orig);
	fs::path dir = p.parent_path();
	std::string stem = p.stem().string();
	int base_id = 0;
	auto pos = stem.find_last_of('_');
	if (pos != std::string::npos)
	{
		bool num = true;
		for (size_t i = pos + 1; i < stem.size(); ++i)
			if (!std::isdigit(static_cast<unsigned char>(stem[i])))
			{
				num = false;
				break;
			}
		if (num)
		{
			base_id = std::atoi(stem.c_str() + pos + 1);
			stem = stem.substr(0, pos);
		}
	}
	int idx = base_id + 1;
	fs::path ext = p.extension();
	for (;; ++idx)
	{
		fs::path candidate =
			dir / (stem + "_" + std::to_string(idx) + ext.string());
		std::error_code ec;
		if (!fs::exists(candidate, ec))
			return candidate.string();
	}
}

Renderer::Renderer(Scene &s, Camera &c) : scene(s), cam(c) {}

struct Renderer::RenderState
{
        bool running = true;
        bool focused = false;
        bool edit_mode = false;
        bool rotating = false;
        int hover_obj = -1;
        int hover_mat = -1;
        int selected_obj = -1;
        int selected_mat = -1;
        double edit_dist = 0.0;
        Vec3 edit_pos;
};

/// Initialize SDL window, renderer and texture objects.
bool Renderer::init_sdl(SDL_Window *&win, SDL_Renderer *&ren, SDL_Texture *&tex,
                                               int W, int H, int RW, int RH)
{
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
                std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
                return false;
        }
        win = SDL_CreateWindow("MiniRT", SDL_WINDOWPOS_CENTERED,
                                                   SDL_WINDOWPOS_CENTERED, W, H, 0);
        if (!win)
        {
                std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
                SDL_Quit();
                return false;
        }
        SDL_SetWindowResizable(win, SDL_FALSE);
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
        if (!ren)
        {
                std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
                SDL_DestroyWindow(win);
                SDL_Quit();
                return false;
        }
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24,
                                                         SDL_TEXTUREACCESS_STREAMING, RW, RH);
        if (!tex)
        {
                std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << "\n";
                SDL_DestroyRenderer(ren);
                SDL_DestroyWindow(win);
                SDL_Quit();
                return false;
        }
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_SetWindowGrab(win, SDL_FALSE);
        return true;
}

/// Handle SDL events, updating render state and selection.
void Renderer::process_events(RenderState &st, SDL_Window *win, SDL_Renderer *ren,
                                                        int W, int H,
                                                        std::vector<Material> &mats,
                                                        const std::string &scene_path)
{
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
                if (e.type == SDL_QUIT)
                        st.running = false;
                else if (e.type == SDL_WINDOWEVENT &&
                                 e.window.event == SDL_WINDOWEVENT_LEAVE)
                {
                        st.focused = false;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        SDL_ShowCursor(SDL_ENABLE);
                        SDL_SetWindowGrab(win, SDL_FALSE);
                }
                else if (e.type == SDL_MOUSEBUTTONDOWN &&
                                 e.button.button == SDL_BUTTON_LEFT)
                {
                        st.focused = true;
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        SDL_ShowCursor(SDL_DISABLE);
                        SDL_SetWindowGrab(win, SDL_TRUE);
                        SDL_WarpMouseInWindow(win, W / 2, H / 2);
                        if (!st.edit_mode && st.hover_obj >= 0)
                        {
                                st.selected_obj = st.hover_obj;
                                st.selected_mat = st.hover_mat;
                                st.hover_obj = st.hover_mat = -1;
                                mats[st.selected_mat].checkered = true;
                                mats[st.selected_mat].color =
                                        mats[st.selected_mat].base_color;
                                AABB box;
                                if (scene.objects[st.selected_obj]->bounding_box(box))
                                {
                                        Vec3 center = (box.min + box.max) * 0.5;
                                        st.edit_dist =
                                                (center - cam.origin).length();
                                        Vec3 desired =
                                                cam.origin + cam.forward * st.edit_dist;
                                        Vec3 delta = desired - center;
                                        if (delta.length_squared() > 0)
                                        {
                                                Vec3 applied = scene.move_with_collision(
                                                        st.selected_obj, delta, mats);
                                                center += applied;
                                                if (applied.length_squared() > 0)
                                                {
                                                        scene.build_bvh();
                                                        st.edit_dist =
                                                                (center - cam.origin).length();
                                                }
                                        }
                                        st.edit_pos = center;
                                }
                                st.edit_mode = true;
                        }
                        else if (st.edit_mode)
                        {
                                mats[st.selected_mat].checkered = false;
                                mats[st.selected_mat].color =
                                        mats[st.selected_mat].base_color;
                                st.selected_obj = st.selected_mat = -1;
                                st.edit_mode = false;
                                st.rotating = false;
                        }
                }
                else if (e.type == SDL_MOUSEBUTTONDOWN &&
                                 e.button.button == SDL_BUTTON_RIGHT)
                {
                        if (st.edit_mode)
                                st.rotating = true;
                }
                else if (e.type == SDL_MOUSEBUTTONUP &&
                                 e.button.button == SDL_BUTTON_RIGHT)
                {
                        st.rotating = false;
                }
                else if (st.focused && e.type == SDL_MOUSEMOTION)
                {
                        if (st.edit_mode && st.rotating)
                        {
                                double sens = get_mouse_sensitivity();
                                bool changed = false;
                                double yaw = -e.motion.xrel * sens;
                                if (yaw != 0.0)
                                {
                                        scene.objects[st.selected_obj]->rotate(cam.up, yaw);
                                        if (scene.collides(st.selected_obj))
                                                scene.objects[st.selected_obj]->rotate(
                                                        cam.up, -yaw);
                                        else
                                                changed = true;
                                }
                                double pitch = -e.motion.yrel * sens;
                                if (pitch != 0.0)
                                {
                                        scene.objects[st.selected_obj]->rotate(cam.right,
                                                                                                          pitch);
                                        if (scene.collides(st.selected_obj))
                                                scene.objects[st.selected_obj]->rotate(
                                                        cam.right, -pitch);
                                        else
                                                changed = true;
                                }
                                if (changed)
                                {
                                        scene.update_beams(mats);
                                        scene.build_bvh();
                                }
                        }
                        else
                        {
                                double sens = get_mouse_sensitivity();
                                cam.rotate(-e.motion.xrel * sens,
                                                   -e.motion.yrel * sens);
                        }
                }
                else if (e.type == SDL_MOUSEWHEEL)
                {
                        double step = e.wheel.y * SCROLL_STEP;
                        if (st.edit_mode)
                        {
                                st.edit_dist = std::clamp(st.edit_dist + step,
                                                           OBJECT_MIN_DIST,
                                                           OBJECT_MAX_DIST);
                        }
                        else if (st.focused)
                        {
                                scene.move_camera(cam, cam.up * step, mats);
                        }
                }
                else if (st.focused && e.type == SDL_KEYDOWN &&
                                 e.key.keysym.scancode == SDL_SCANCODE_C)
                {
                        scene.update_beams(mats);
                        scene.build_bvh();
                        std::string save = next_save_path(scene_path);
                        if (Parser::save_rt_file(save, scene, cam, mats))
                                std::cout << "Saved scene to: " << save << "\n";
                        else
                                std::cerr << "Failed to save scene to: " << save << "\n";
                }
                else if (st.focused && e.type == SDL_KEYDOWN &&
                                 e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                        st.focused = false;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        SDL_ShowCursor(SDL_ENABLE);
                        SDL_SetWindowGrab(win, SDL_FALSE);
                        SDL_WarpMouseInWindow(win, W / 2, H / 2);
                        int current_w = W;
                        int current_h = H;
                        SDL_GetWindowSize(win, &current_w, &current_h);
                        bool resume = PauseMenu::show(win, ren, current_w, current_h);
                        if (resume)
                        {
                                st.focused = true;
                                SDL_SetRelativeMouseMode(SDL_TRUE);
                                SDL_ShowCursor(SDL_DISABLE);
                                SDL_SetWindowGrab(win, SDL_TRUE);
                                SDL_WarpMouseInWindow(win, W / 2, H / 2);
                        }
                        else
                        {
                                st.running = false;
                        }
                }
        }
}

/// Handle continuous keyboard input for camera or object control.
void Renderer::handle_keyboard(RenderState &st, double dt,
                                                          std::vector<Material> &mats)
{
        const Uint8 *state = SDL_GetKeyboardState(nullptr);
        Vec3 forward_xz = cam.forward;
        forward_xz.y = 0.0;
        if (forward_xz.length_squared() > 0.0)
                forward_xz = forward_xz.normalized();
        Vec3 right_xz = cam.right;

        if (st.edit_mode)
        {
                double cam_speed = CAMERA_MOVE_SPEED * dt;
                if (state[SDL_SCANCODE_W])
                        scene.move_camera(cam, forward_xz * cam_speed, mats);
                if (state[SDL_SCANCODE_S])
                        scene.move_camera(cam, forward_xz * -cam_speed, mats);
                if (state[SDL_SCANCODE_A])
                        scene.move_camera(cam, right_xz * -cam_speed, mats);
                if (state[SDL_SCANCODE_D])
                        scene.move_camera(cam, right_xz * cam_speed, mats);
                if (state[SDL_SCANCODE_SPACE])
                        scene.move_camera(cam, Vec3(0, 1, 0) * cam_speed, mats);
                if (state[SDL_SCANCODE_LCTRL])
                        scene.move_camera(cam, Vec3(0, -1, 0) * cam_speed, mats);

                double rot_speed = OBJECT_ROTATE_SPEED * dt;
                bool changed = false;
                if (state[SDL_SCANCODE_Q])
                {
                        scene.objects[st.selected_obj]->rotate(cam.forward, -rot_speed);
                        if (scene.collides(st.selected_obj))
                                scene.objects[st.selected_obj]->rotate(cam.forward,
                                                                                                      rot_speed);
                        else
                                changed = true;
                }
                if (state[SDL_SCANCODE_E])
                {
                        scene.objects[st.selected_obj]->rotate(cam.forward, rot_speed);
                        if (scene.collides(st.selected_obj))
                                scene.objects[st.selected_obj]->rotate(cam.forward,
                                                                                                      -rot_speed);
                        else
                                changed = true;
                }
                if (changed)
                {
                        scene.update_beams(mats);
                        scene.build_bvh();
                }
        }
        else if (st.focused)
        {
                double speed = CAMERA_MOVE_SPEED * dt;
                if (state[SDL_SCANCODE_W])
                        scene.move_camera(cam, forward_xz * speed, mats);
                if (state[SDL_SCANCODE_S])
                        scene.move_camera(cam, forward_xz * -speed, mats);
                if (state[SDL_SCANCODE_A])
                        scene.move_camera(cam, right_xz * -speed, mats);
                if (state[SDL_SCANCODE_D])
                        scene.move_camera(cam, right_xz * speed, mats);
                if (state[SDL_SCANCODE_SPACE])
                        scene.move_camera(cam, Vec3(0, 1, 0) * speed, mats);
                if (state[SDL_SCANCODE_LCTRL])
                        scene.move_camera(cam, Vec3(0, -1, 0) * speed, mats);
        }
}

/// Update object selection or hover based on current state.
void Renderer::update_selection(RenderState &st,
                                                          std::vector<Material> &mats)
{
        if (st.edit_mode)
        {
                if (st.hover_mat >= 0 && st.hover_mat != st.selected_mat)
                        mats[st.hover_mat].color =
                                mats[st.hover_mat].base_color;
                st.hover_obj = st.hover_mat = -1;

                Vec3 desired = cam.origin + cam.forward * st.edit_dist;
                Vec3 delta = desired - st.edit_pos;
                if (delta.length_squared() > 0)
                {
                        Vec3 applied = scene.move_with_collision(st.selected_obj, delta, mats);
                        st.edit_pos += applied;
                        if (applied.length_squared() > 0)
                        {
                                scene.build_bvh();
                        }
                }

                Vec3 cam_target = st.edit_pos - cam.forward * st.edit_dist;
                Vec3 cam_delta = cam_target - cam.origin;
                if (cam_delta.length_squared() > 0)
                        scene.move_camera(cam, cam_delta, mats);
                st.edit_dist = (st.edit_pos - cam.origin).length();
        }
        else
        {
                Ray center_ray = cam.ray_through(0.5, 0.5);
                HitRecord hrec;
                if (scene.hit(center_ray, 1e-4, 1e9, hrec) &&
                        scene.objects[hrec.object_id]->movable)
                {
                        if (st.hover_mat != hrec.material_id)
                        {
                                if (st.hover_mat >= 0)
                                        mats[st.hover_mat].color =
                                                mats[st.hover_mat].base_color;
                                st.hover_obj = hrec.object_id;
                                st.hover_mat = hrec.material_id;
                        }
                        bool blink = ((SDL_GetTicks() / 250) % 2) == 0;
                        mats[st.hover_mat].color =
                                blink ? (Vec3(1.0, 1.0, 1.0) -
                                                 mats[st.hover_mat].base_color)
                                          : mats[st.hover_mat].base_color;
                }
                else
                {
                        if (st.hover_mat >= 0)
                                mats[st.hover_mat].color =
                                        mats[st.hover_mat].base_color;
                        st.hover_obj = st.hover_mat = -1;
                }
        }
}

/// Render the current frame and display it to the window.
void Renderer::render_frame(RenderState &st, SDL_Renderer *ren, SDL_Texture *tex,
                                                       std::vector<Vec3> &framebuffer,
                                                       std::vector<unsigned char> &pixels,
                                                       int RW, int RH, int W, int H, int T,
                                                       std::vector<Material> &mats)
{
        std::atomic<int> next_row{0};
        auto worker = [&]()
        {
                std::mt19937 rng(std::random_device{}());
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                for (;;)
                {
                        int y = next_row.fetch_add(1);
                        if (y >= RH)
                                break;
                        for (int x = 0; x < RW; ++x)
                        {
                                double u = (x + 0.5) / RW;
                                double v = (y + 0.5) / RH;
                                Ray r = cam.ray_through(u, v);
                                Vec3 col = trace_ray(scene, mats, r, rng, dist);
                                framebuffer[y * RW + x] = col;
                        }
                }
        };

        std::vector<std::thread> pool;
        pool.reserve(T);
        for (int i = 0; i < T; ++i)
                pool.emplace_back(worker);
        for (auto &th : pool)
                th.join();

        for (int y = 0; y < RH; ++y)
        {
                for (int x = 0; x < RW; ++x)
                {
                        Vec3 c = framebuffer[y * RW + x];
                        c.x = std::clamp(c.x, 0.0, 1.0);
                        c.y = std::clamp(c.y, 0.0, 1.0);
                        c.z = std::clamp(c.z, 0.0, 1.0);
                        pixels[(y * RW + x) * 3 + 0] =
                                static_cast<unsigned char>(std::lround(c.x * 255.0));
                        pixels[(y * RW + x) * 3 + 1] =
                                static_cast<unsigned char>(std::lround(c.y * 255.0));
                        pixels[(y * RW + x) * 3 + 2] =
                                static_cast<unsigned char>(std::lround(c.z * 255.0));
                }
        }

        SDL_UpdateTexture(tex, nullptr, pixels.data(), RW * 3);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        if (st.edit_mode)
        {
                auto project = [&](const Vec3 &p, int &sx, int &sy) -> bool
                {
                        Vec3 rel = p - cam.origin;
                        double z = Vec3::dot(rel, cam.forward);
                        if (z <= 0.0)
                                return false;
                        double x = Vec3::dot(rel, cam.right);
                        double y = Vec3::dot(rel, cam.up);
                        double fov_rad = cam.fov_deg * M_PI / 180.0;
                        double half_h = std::tan(fov_rad * 0.5);
                        double half_w = cam.aspect * half_h;
                        double u = (x / z / half_w + 1.0) * 0.5;
                        double v = (1.0 - y / z / half_h) * 0.5;
                        sx = static_cast<int>(u * W);
                        sy = static_cast<int>(v * H);
                        return true;
                };
                const int edges[12][2] = {{0, 1},  {0, 2},  {0, 4},  {1, 3},
                                                                           {1, 5},  {2, 3},  {2, 6},  {3, 7},
                                                                           {4, 5},  {4, 6},  {5, 7},  {6, 7}};
                for (size_t i = 0; i < scene.objects.size(); ++i)
                {
                        auto &obj = scene.objects[i];
                        if (obj->is_beam() || obj->is_plane())
                                continue;
                        AABB box;
                        if (!obj->bounding_box(box))
                                continue;
                        Vec3 corners[8] = {Vec3(box.min.x, box.min.y, box.min.z),
                                                           Vec3(box.max.x, box.min.y, box.min.z),
                                                           Vec3(box.min.x, box.max.y, box.min.z),
                                                           Vec3(box.max.x, box.max.y, box.min.z),
                                                           Vec3(box.min.x, box.min.y, box.max.z),
                                                           Vec3(box.max.x, box.min.y, box.max.z),
                                                           Vec3(box.min.x, box.max.y, box.max.z),
                                                           Vec3(box.max.x, box.max.y, box.max.z)};
                        SDL_Point pts[8];
                        bool ok = true;
                        for (int j = 0; j < 8; ++j)
                        {
                                int sx, sy;
                                if (!project(corners[j], sx, sy))
                                {
                                        ok = false;
                                        break;
                                }
                                pts[j].x = sx;
                                pts[j].y = sy;
                        }
                        if (!ok)
                                continue;
                        if (static_cast<int>(i) == st.selected_obj)
                                SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
                        else
                                SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
                        for (const auto &e : edges)
                                SDL_RenderDrawLine(ren, pts[e[0]].x, pts[e[0]].y,
                                                                                   pts[e[1]].x, pts[e[1]].y);
                }
        }
        SDL_RenderPresent(ren);
}

void Renderer::render_ppm(const std::string &path,
						  const std::vector<Material> &mats,
						  const RenderSettings &rset)
{
	const float scale = std::max(1.0f, rset.downscale);
	const int W = std::max(1, static_cast<int>(rset.width / scale));
	const int H = std::max(1, static_cast<int>(rset.height / scale));
	const int T = (rset.threads > 0)
					  ? rset.threads
					  : (std::thread::hardware_concurrency()
							 ? (int)std::thread::hardware_concurrency()
							 : 8);

	std::vector<Vec3> framebuffer(W * H);
	std::atomic<int> next_row{0};

	auto worker = [&]()
	{
		std::mt19937 rng(std::random_device{}());
		std::uniform_real_distribution<double> dist(0.0, 1.0);
		for (;;)
		{
			int y = next_row.fetch_add(1);
			if (y >= H)
				break;
			for (int x = 0; x < W; ++x)
			{
				double u = (x + 0.5) / W;
				double v = (y + 0.5) / H;
				Ray r = cam.ray_through(u, v);
				Vec3 col = trace_ray(scene, mats, r, rng, dist);
				framebuffer[y * W + x] = col;
			}
		}
	};

	std::vector<std::thread> pool;
	pool.reserve(T);
	for (int i = 0; i < T; ++i)
		pool.emplace_back(worker);
	for (auto &th : pool)
		th.join();

	std::ofstream out(path, std::ios::binary);
	out << "P6\n" << W << " " << H << "\n255\n";
	for (int y = 0; y < H; ++y)
	{
		for (int x = 0; x < W; ++x)
		{
			Vec3 c = framebuffer[y * W + x];
			c.x = std::clamp(c.x, 0.0, 1.0);
			c.y = std::clamp(c.y, 0.0, 1.0);
			c.z = std::clamp(c.z, 0.0, 1.0);
			unsigned char r =
				static_cast<unsigned char>(std::lround(c.x * 255.0));
			unsigned char g =
				static_cast<unsigned char>(std::lround(c.y * 255.0));
			unsigned char b =
				static_cast<unsigned char>(std::lround(c.z * 255.0));
			out.put(r).put(g).put(b);
		}
	}
}

void Renderer::render_window(std::vector<Material> &mats,
                                                         const RenderSettings &rset,
                                                         const std::string &scene_path)
{
        const int W = rset.width;
        const int H = rset.height;
        auto quality_scale = [](char q) -> float {
                if (q == 'M' || q == 'm')
                        return 1.5f;
                if (q == 'L' || q == 'l')
                        return 2.5f;
                return 1.0f;
        };
        float scale = std::max(1.0f, quality_scale(g_settings.quality));
        int RW = std::max(1, static_cast<int>(W / scale));
        int RH = std::max(1, static_cast<int>(H / scale));
        const int T = (rset.threads > 0)
                                          ? rset.threads
                                          : (std::thread::hardware_concurrency()
                                                         ? (int)std::thread::hardware_concurrency()
                                                         : 8);

        SDL_Window *win = nullptr;
        SDL_Renderer *ren = nullptr;
        SDL_Texture *tex = nullptr;
        if (!init_sdl(win, ren, tex, W, H, RW, RH))
                return;

        RenderState st;
        st.focused = true;
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_ShowCursor(SDL_DISABLE);
        SDL_SetWindowGrab(win, SDL_TRUE);
        SDL_WarpMouseInWindow(win, W / 2, H / 2);

        std::vector<Vec3> framebuffer(RW * RH);
        std::vector<unsigned char> pixels(RW * RH * 3);
        Uint32 last = SDL_GetTicks();
        char current_quality = g_settings.quality;

        while (st.running)
        {
                Uint32 now = SDL_GetTicks();
                double dt = (now - last) / 1000.0;
                last = now;

                if (g_settings.quality != current_quality)
                {
                        current_quality = g_settings.quality;
                        scale = std::max(1.0f, quality_scale(current_quality));
                        RW = std::max(1, static_cast<int>(W / scale));
                        RH = std::max(1, static_cast<int>(H / scale));
                        SDL_DestroyTexture(tex);
                        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24,
                                                                        SDL_TEXTUREACCESS_STREAMING, RW, RH);
                        if (!tex)
                                break;
                        framebuffer.assign(RW * RH, Vec3());
                        pixels.assign(RW * RH * 3, 0);
                }

                process_events(st, win, ren, W, H, mats, scene_path);
                handle_keyboard(st, dt, mats);
                scene.update_goal_targets(dt, mats);
                update_selection(st, mats);
                render_frame(st, ren, tex, framebuffer, pixels, RW, RH, W, H, T,
                                         mats);
        }

        SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
}
