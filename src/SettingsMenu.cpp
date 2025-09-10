#include "SettingsMenu.hpp"
#include "ButtonsCluster.hpp"
#include "Slider.hpp"
#include "Settings.hpp"
#include "CustomCharacter.hpp"
#include <SDL.h>
#include <sstream>

SettingsMenu::SettingsMenu() : AMenu("SETTINGS") {
    buttons.push_back(Button{"BACK", ButtonAction::Back, SDL_Color{255,0,0,255}});
    buttons.push_back(Button{"APPLY", ButtonAction::Apply, SDL_Color{0,255,0,255}});
}

ButtonAction SettingsMenu::run(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    Settings &cfg = get_settings();
    int qidx = 2;
    if (cfg.quality == 'L') qidx = 0;
    else if (cfg.quality == 'M') qidx = 1;
    ButtonsCluster quality({"LOW","MEDIUM","HIGH"}, qidx);

    std::vector<std::string> sensVals;
    for (int i=1;i<=20;i++) {
        double v = i*0.1;
        std::ostringstream ss; ss.setf(std::ios::fixed); ss.precision(1); ss<<v; sensVals.push_back(ss.str());
    }
    int sensIndex = static_cast<int>((cfg.mouse_sensitivity-0.1)/0.1);
    if (sensIndex<0) sensIndex=0; if (sensIndex>=(int)sensVals.size()) sensIndex=sensVals.size()-1;
    Slider sensSlider("MOUSE SENSITIVITY", sensVals, sensIndex);

    std::vector<std::string> resVals = {"720x480","1080x720","1366x768","1920x1080"};
    std::string curRes = std::to_string(cfg.width)+"x"+std::to_string(cfg.height);
    int resIndex = 1;
    for (size_t i=0;i<resVals.size();++i) if (resVals[i]==curRes) resIndex=i;
    Slider resSlider("RESOLUTION", resVals, resIndex);

    bool running=true;
    ButtonAction result = ButtonAction::Back;
    while (running) {
        SDL_GetWindowSize(window,&width,&height);
        float scale_factor = static_cast<float>(height)/600.0f;
        int button_width = static_cast<int>(120*scale_factor);
        int button_height = static_cast<int>(50*scale_factor);
        int button_gap = static_cast<int>(20*scale_factor);
        int scale = std::max(1, static_cast<int>(4*scale_factor));
        int title_scale = scale*2;

        int bottom_y = height - button_height - 20*scale;
        buttons[0].rect = {width/2 - button_width - button_gap/2, bottom_y, button_width, button_height};
        buttons[1].rect = {width/2 + button_gap/2, bottom_y, button_width, button_height};

        int content_y = 60*scale + 7*title_scale;
        quality.set_rect(width/2 - (3*button_width + 2*button_gap)/2, content_y + 20*scale, button_width, button_height, button_gap);
        int next_y = content_y + button_height + 60*scale;
        sensSlider.set_rect(width/2 - 150*scale, next_y, 300*scale, 2*scale);
        next_y += 60*scale;
        resSlider.set_rect(width/2 - 150*scale, next_y, 300*scale, 2*scale);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type==SDL_QUIT) { running=false; result=ButtonAction::Quit; }
            quality.handle_event(e);
            sensSlider.handle_event(e);
            resSlider.handle_event(e);
            if (e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT) {
                int mx=e.button.x,my=e.button.y;
                for (auto &btn:buttons) {
                    if (mx>=btn.rect.x && mx<btn.rect.x+btn.rect.w && my>=btn.rect.y && my<btn.rect.y+btn.rect.h) {
                        if (btn.action==ButtonAction::Back) {
                            running=false; result=ButtonAction::Back;
                        } else if (btn.action==ButtonAction::Apply) {
                            int qi = quality.current_index();
                            cfg.quality = (qi==0?'L':qi==1?'M':'H');
                            cfg.mouse_sensitivity = std::stod(sensSlider.current());
                            std::string r = resSlider.current();
                            int w,h; char x; std::stringstream ss(r); ss>>w>>x>>h;
                            cfg.width=w; cfg.height=h;
                            save_settings("settings.yaml");
                            SDL_SetWindowSize(window,cfg.width,cfg.height);
                            running=false; result=ButtonAction::Apply;
                        }
                        break;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);
        SDL_Color white{255,255,255,255};
        int title_x = width/2 - CustomCharacter::text_width(title, title_scale)/2;
        CustomCharacter::draw_text(renderer,title,title_x,20*scale,white,title_scale);
        int qlabel_x = width/2 - CustomCharacter::text_width("QUALITY", scale)/2;
        CustomCharacter::draw_text(renderer,"QUALITY",qlabel_x,content_y,white,scale);
        quality.render(renderer, scale);
        sensSlider.render(renderer, scale);
        resSlider.render(renderer, scale);
        for (auto &btn : buttons) {
            SDL_SetRenderDrawColor(renderer,0,0,0,255);
            SDL_RenderFillRect(renderer,&btn.rect);
            SDL_SetRenderDrawColor(renderer,255,255,255,255);
            SDL_RenderDrawRect(renderer,&btn.rect);
            int tx = btn.rect.x + (btn.rect.w - CustomCharacter::text_width(btn.text, scale))/2;
            int ty = btn.rect.y + (btn.rect.h - 7*scale)/2;
            CustomCharacter::draw_text(renderer, btn.text, tx, ty, white, scale);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return result;
}

void SettingsMenu::show(SDL_Window *window, SDL_Renderer *renderer, int width, int height) {
    SettingsMenu menu;
    menu.run(window, renderer, width, height);
}
