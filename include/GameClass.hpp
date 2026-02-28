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
#include<vector>
#include<ctime>

class Game{
    public:
        Game(SDL_Renderer* renderer, TTF_Font* font, const float width, const float height, const std::string Name, const int64_t Seed) : m_renderer(renderer), m_font(font), m_width(width), m_height(height), Name(Name), Seed(Seed), w(m_height/N), h(w){
            srand(time(0));
            CreateMap();
        }
    void Render(){
        SDL_SetRenderDrawColor(m_renderer, 0,0,0,255);
        SDL_RenderClear(m_renderer);
        RenderMap();
    }
    void RenderMap(){
        for(auto i = 0;i<N;++i){
            for(auto j=0;j<N;++j){
                x=j*w;
                y=i*h;
                switch(Map[i*N+j].biome){
                    case 1: Col={0,0,100,255};
                            break;
                    case 0: Col={0,100,0,255};
                            break;
                }
                RenderTile(x,y, Col);
                
            }
        }
    }
    void RenderTile(float& x, float& y, SDL_Color& Col){
        SDL_SetRenderDrawColor(m_renderer,Col.r,Col.g,Col.b,255);
        SDL_FRect Tile = {x,y,w,h};
        SDL_RenderFillRect(m_renderer, &Tile);
    }
    void CreateMap(){
        Map.resize(N*N);
        for(auto i = 0;i<N;++i){
            for(auto j = 0; j<N;++j){
                Map[i*N+j]={i,j,rand()%2};
            }
        }
    }
    void HandleEvent(const SDL_Event& event){}

    struct Tile{
        int x,y;
        int biome;
    };
    private:
        SDL_Renderer* m_renderer;
        TTF_Font* m_font;
        const float m_width, m_height;
        std::string Name;
        const uint64_t Seed;
        std::vector<Tile>Map{};
        const uint16_t N = 64;
        float x,y,w,h;
        SDL_Color Col;
};
