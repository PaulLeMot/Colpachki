#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL_main.h>
#include <string>

class Game{
    public:
        Game(SDL_Renderer* renderer, TTF_Font* font, const float width, const float height, const std::string Name, const int64_t Seed) : m_renderer(renderer), m_font(font), m_width(width), m_height(height), Name(Name), Seed(Seed){

        }
    void Render(){
        SDL_SetRenderDrawColor(m_renderer, 0,0,0,255);
        SDL_RenderClear(m_renderer);
    }

    void HandleEvent(const SDL_Event& event){}
    private:
        SDL_Renderer* m_renderer;
        TTF_Font* m_font;
        const float m_width, m_height;
        std::string Name;
        uint64_t Seed;
};
