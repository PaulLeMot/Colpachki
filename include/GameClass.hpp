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
        Game(SDL_Renderer* renderer, TTF_Font* font, 
            const float width, const float height, 
            const std::string Name, const int64_t Seed,
            int* StatePtr) : 
            m_renderer(renderer), m_font(font),
            m_width(width), m_height(height), 
            Name(Name), Seed(Seed), m_state(StatePtr){
                Otstup = (m_width - m_height) / 2;
                srand(time(0));
                CreateMap();
                SmoothClimate(3);
                InitButtons();
                m_buttonsObjects.emplace_back(
                    m_renderer,m_font,m_buttons[0],
                    m_width, m_height,
                    m_width-(m_width/10),
                    0,
                    m_width/10,m_width/10,240,240,240);
        }
    void Render() {
        static const SDL_Color ZONE_COLORS[6] = {
            {222, 254, 247, 255},
            {4, 166, 128, 255},
            {4, 166, 47, 255},
            {186, 251, 68, 255},
            {251, 209, 68, 255},
            {0, 128, 0, 255}
        };
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_renderer);
    
        float baseTileSize = m_height / N;
        float tileSize = baseTileSize * zoom;
    
        float visibleWidth  = m_height / tileSize;
        float visibleHeight = m_height / tileSize;
    
        int startX = static_cast<int>(std::floor(panX));
        int startY = static_cast<int>(std::floor(panY));
        int endX   = static_cast<int>(std::ceil(panX + visibleWidth));
        int endY   = static_cast<int>(std::ceil(panY + visibleHeight));
    
        for (int worldY = startY; worldY <= endY; ++worldY) {
            for (int worldX = startX; worldX <= endX; ++worldX) {
                int mapX = ((worldX % N) + N) % N;
                int mapY = ((worldY % N) + N) % N;
    
                float screenX = Otstup + (worldX - panX) * tileSize;
                float screenY = (worldY - panY) * tileSize;
    
                if (screenX + tileSize < Otstup || screenX > Otstup + m_height ||
                    screenY + tileSize < 0 || screenY > m_height) {
                    continue;
                }
                const Tile& tile = Map[mapY * N + mapX];
                if (tile.biome == 0) {
                    SDL_Color col = ZONE_COLORS[tile.zone];
                    SDL_SetRenderDrawColor(m_renderer, col.r, col.g, col.b, 255);
                } else {
                    SDL_SetRenderDrawColor(m_renderer, 0, 0, 100, 255); // вода
                }
                SDL_FRect tileRect = {screenX, screenY, tileSize, tileSize};
                SDL_RenderFillRect(m_renderer, &tileRect);
            }
        }
    
        m_buttonsObjects[0].RenderButton();
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
        const float lacunarity = 2.7f;
        const float scale = 0.02f;
        const float T = N * scale;

        std::vector<float> rawValues(N * N);

        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                float x = i * scale;
                float y = j * scale;

                float wx = x / T;
                float wy = y / T;

                auto fractal_noise = [&](float px, float py) {
                    float amp = 1.0f;
                    float freq = 1.0f;
                    float value = 0.0f;
                    float maxAmp = 0.0f;
                    for (int o = 0; o < octaves; ++o) {
                        value += amp * noise(px * freq, py * freq);
                        maxAmp += amp;
                        amp *= persistence;
                        freq *= lacunarity;
                    }
                    return value / maxAmp;
                };

                float v00 = fractal_noise(x, y);
                float v10 = fractal_noise(x - T, y);
                float v01 = fractal_noise(x, y - T);
                float v11 = fractal_noise(x - T, y - T);

                float raw = (1.0f - wx) * (1.0f - wy) * v00
                        + wx * (1.0f - wy) * v10
                        + (1.0f - wx) * wy * v01
                        + wx * wy * v11;

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
                int zone = -1;

                if (biome == 0) {
                    float pos = (float)i / N;
                    float p = pos <= 0.5f ? pos : 1.0f - pos;

                    const float borders[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.47f};
                    const float blend_width = 0.02f;

                    int main_zone;
                    if (p <= borders[0]) main_zone = 0;
                    else if (p <= borders[1]) main_zone = 1;
                    else if (p <= borders[2]) main_zone = 2;
                    else if (p <= borders[3]) main_zone = 3;
                    else if (p <= borders[4]) main_zone = 4;
                    else main_zone = 5;

                    int selected_zone = main_zone;

                    for (int b = 0; b < 5; ++b) {
                        if (p >= borders[b] - blend_width && p <= borders[b] + blend_width) {
                            float t = (p - (borders[b] - blend_width)) / (2.0f * blend_width);
                            float rnd = (float)rand() / RAND_MAX;
                            selected_zone = (rnd < t) ? b + 1 : b;
                            break;
                        }
                    }
                    zone = selected_zone;
                }

                Map[i * N + j] = {i, j, biome, zone};
            }
        }
    }

    void SmoothClimate(int iterations = 5) {
        std::vector<Tile> newMap = Map;

        for (int iter = 0; iter < iterations; ++iter) {
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    if (tile.biome == 1) continue;

                    int votes[6] = {0};

                    for (int di = -1; di <= 1; ++di) {
                        for (int dj = -1; dj <= 1; ++dj) {
                            if (di == 0 && dj == 0) continue;
                            int ni = (i + di + N) % N;
                            int nj = (j + dj + N) % N;
                            const Tile& neighbor = Map[ni * N + nj];
                            if (neighbor.biome == 1) continue;
                            if (neighbor.zone >= 0 && neighbor.zone < 6)
                                votes[neighbor.zone]++;
                        }
                    }

                    int bestZone = tile.zone;
                    int bestCount = 0;
                    for (int z = 0; z < 6; ++z) {
                        if (votes[z] > bestCount) {
                            bestCount = votes[z];
                            bestZone = z;
                        }
                    }

                    if (bestCount > 2) {
                        newMap[i * N + j].zone = bestZone;
                    } else {
                        newMap[i * N + j].zone = tile.zone;
                    }
                }
            }
            std::swap(Map, newMap);
        }
    }

    void HandleEvent(const SDL_Event& event) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int mouseX = static_cast<int>(event.button.x);
            int mouseY = static_cast<int>(event.button.y);
            if (m_buttonsObjects[0].GetButtonAt(mouseX, mouseY)) {
                if(m_state)*m_state=1;
                return;
            }
        }
    
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            isDragging = true;
            startMouseX = event.button.x;
            startMouseY = event.button.y;
            startPanX = panX;
            startPanY = panY;
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            isDragging = false;
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION && isDragging) {
            int mouseX = event.motion.x;
            int mouseY = event.motion.y;
            float tileSize = (m_height / N) * zoom;
        
            panX = startPanX - (mouseX - startMouseX) / tileSize;
            panY = startPanY - (mouseY - startMouseY) / tileSize;
        
            panX = fmod(panX, (float)N);
            if (panX < 0) panX += N;
            panY = fmod(panY, (float)N);
            if (panY < 0) panY += N;
        }
    
        if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            float mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
        
            float oldTileSize = (m_height / N) * zoom;
        
            float worldX = panX + (mouseX - Otstup) / oldTileSize;
            float worldY = panY + mouseY / oldTileSize;
        
            float wheel = event.wheel.y;
            float zoomSpeed = 0.1f;
            zoom *= (1.0f + wheel * zoomSpeed);
        
            const float minZoom = 1.0f;
            const float maxZoom = 15.0f;
            if (zoom < minZoom) zoom = minZoom;
            if (zoom > maxZoom) zoom = maxZoom;
        
            float newTileSize = (m_height / N) * zoom;
        
            panX = worldX - (mouseX - Otstup) / newTileSize;
            panY = worldY - mouseY / newTileSize;
        
            panX = fmod(panX, (float)N);
            if (panX < 0) panX += N;
            panY = fmod(panY, (float)N);
            if (panY < 0) panY += N;
            if (isDragging) {
                startPanX = panX;
                startPanY = panY;
                startMouseX = static_cast<int>(mouseX);
                startMouseY = static_cast<int>(mouseY);
            }
        }
    }

    struct Tile{
        int x,y;
        int biome;
        int zone;
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
        const uint16_t N = 256;
        int Otstup=(m_width-m_height)/2;
        std::vector<std::string>m_buttons;
        std::vector<Button>m_buttonsObjects;
        bool isDragging = false;
        float panX = 0.0f, panY = 0.0f;
        float zoom = 1.0f;
        float startPanX = 0.0f, startPanY = 0.0f;
        int startMouseX = 0, startMouseY = 0;
        int* m_state;
};
