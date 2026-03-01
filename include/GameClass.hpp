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
#include <random>
#include <algorithm>

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
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                float offsetX = Otstup + col * cellSize;
                float offsetY = row * cellSize;
                RenderOneMap(offsetX, offsetY, tileSize);
            }
        }
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
    void RenderOneMap(float offsetX, float offsetY, float tileSize) {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                float x = offsetX + j * tileSize;
                float y = offsetY + i * tileSize;
                SDL_Color color;
                switch (Map[i * N + j].biome) {
                    case 1: color = {0, 0, 100, 255}; break;
                    case 0: color = {0, 100, 0, 255}; break;
                    default: color = {0, 0, 100, 255}; break;
                }
                SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, 255);
                SDL_FRect tile = {x, y, tileSize, tileSize};
                SDL_RenderFillRect(m_renderer, &tile);
            }
        }
    }
    void CreateMap() {
        std::mt19937 rng(Seed);
        std::vector<int> permutation(256);
        for (int i = 0; i < 256; ++i) permutation[i] = i;
        std::shuffle(permutation.begin(), permutation.end(), rng);
        
        std::vector<int> p(512);
        for (int i = 0; i < 512; ++i) p[i] = permutation[i % 256];
    
        auto fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };
        auto lerp = [](float a, float b, float t) { return a + t * (b - a); };
        auto grad = [](int hash, float x, float y) {
            int h = hash & 15;
            float u = h < 8 ? x : y;
            float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
            return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
        };
    
        auto noise = [&](float x, float y) {
            int X = (int)std::floor(x) & 255;
            int Y = (int)std::floor(y) & 255;
            x -= std::floor(x);
            y -= std::floor(y);
            float u = fade(x);
            float v = fade(y);
            int aa = p[p[X] + Y];
            int ab = p[p[X] + Y + 1];
            int ba = p[p[X + 1] + Y];
            int bb = p[p[X + 1] + Y + 1];
            float g1 = grad(aa, x, y);
            float g2 = grad(ba, x - 1, y);
            float g3 = grad(ab, x, y - 1);
            float g4 = grad(bb, x - 1, y - 1);
            float x1 = lerp(g1, g2, u);
            float x2 = lerp(g3, g4, u);
            return lerp(x1, x2, v);
        };
    
        const int octaves = 4;
        const float persistence = 0.5f;
        const float lacunarity = 2.0f;
        const float scale = 0.08f;
    
        std::vector<float> rawValues(N * N);
    
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                float x = i * scale;
                float y = j * scale;
                float amplitude = 1.0f;
                float frequency = 1.0f;
                float noiseValue = 0.0f;
                float maxAmplitude = 0.0f;
    
                for (int o = 0; o < octaves; ++o) {
                    noiseValue += amplitude * noise(x * frequency, y * frequency);
                    maxAmplitude += amplitude;
                    amplitude *= persistence;
                    frequency *= lacunarity;
                }
    
                float raw = noiseValue / maxAmplitude;
                rawValues[i * N + j] = raw;
            }
        }
    
        std::vector<float> sorted = rawValues;
        std::sort(sorted.begin(), sorted.end());
        size_t thresholdIndex = static_cast<size_t>(sorted.size() * 0.33);
        float threshold = sorted[thresholdIndex];
    
        Map.resize(N * N);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                float raw = rawValues[i * N + j];
                int biome = (raw > threshold) ? 1 : 0;
                Map[i * N + j] = {i, j, biome};
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
        float cellSize=m_height/3.0f;
        float tileSize=cellSize/N;
};
