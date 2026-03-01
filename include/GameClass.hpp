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
#include "ColpEngine.hpp"

class Game{
    public:
        Game(SDL_Renderer* renderer, TTF_Font* font, const float width, const float height, const std::string Name, const int64_t Seed) : m_renderer(renderer), m_font(font), m_width(width), m_height(height), Name(Name), Seed(Seed), w(m_height/N), h(w){
            srand(time(0));
            CreateMap();
            InitButtons();
            m_buttonsObjects.emplace_back(
                m_renderer,m_font,m_buttons[0],
                m_width, m_height,
                m_width-(m_width/10),
                0,
                m_width/10,m_width/10,240,240,240);
        }
    void Render(){
        SDL_SetRenderDrawColor(m_renderer, 0,0,0,255);
        SDL_RenderClear(m_renderer);
        RenderMap();
        m_buttonsObjects[0].RenderButton();
    }
    void RenderMap(){
        for(auto i = 0;i<N;++i){
            for(auto j=0;j<N;++j){
                x=Otstup+(j*w);
                y=i*h;
                switch(Map[i*N+j].biome){
                    case 1: Col={0,0,100,255};
                            break;
                    case 0: Col={0,100,0,255};
                            break;
                    case 2: Col={0,0,100,255};
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
                Map[i*N+j]={i,j,rand()%3};
            }
        }
    }
    void HandleEvent(const SDL_Event& event){
        if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN){
            int mouseX=static_cast<int>(event.button.x);
            int mouseY=static_cast<int>(event.button.y);
            if(m_buttonsObjects[0].GetButtonAt(mouseX,mouseY)){
                SDL_Event quitEvent;
                quitEvent.type=SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            }
        }
    }

    struct Tile{
        int x,y;
        int biome;
    };
    void InitButtons(){
        m_buttons={"..."};
    }
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
        const int Otstup=(m_width-m_height)/2;
        std::vector<std::string>m_buttons;
        std::vector<Button>m_buttonsObjects;
};
