#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
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
            uint8_t* StatePtr) : 
            m_renderer(renderer), m_font(font),
            m_width(width), m_height(height), 
            Name(Name), Seed(Seed), m_state(StatePtr),
            m_mapTexture(nullptr){
                Otstup = (m_width - m_height) / 2;
                srand(time(0));
                m_atlasSurface = IMG_Load("../atlas.png");
                if (!m_atlasSurface) {
                    SDL_Log("Failed to load atlas.png: %s", SDL_GetError());
                } else {
                    if (m_atlasSurface->format != SDL_PIXELFORMAT_RGBA8888) {
                        SDL_Surface* converted = SDL_ConvertSurface(m_atlasSurface, SDL_PIXELFORMAT_RGBA8888);
                        SDL_DestroySurface(m_atlasSurface);
                        m_atlasSurface = converted;
                    }
                    if (m_atlasSurface->w != 128 || m_atlasSurface->h != 128) {
                        SDL_Log("Warning: atlas.png size is %dx%d, expected 256x256", m_atlasSurface->w, m_atlasSurface->h);
                    }
                }
                CreateMap();
                SmoothClimate(2+(N/256));
                ApplyCoastalInfluence(1);
                InitButtons();
                m_buttonsObjects.emplace_back(
                    m_renderer,m_font,m_buttons[0],
                    m_width, m_height,
                    m_width-(m_width/10),
                    0,
                    m_width/10,m_width/10,240,240,240);
                    CreateMapTexture(); 
        }
        ~Game() {
            if (m_mapTexture) SDL_DestroyTexture(m_mapTexture);
        }
        void Render() {
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
            SDL_RenderClear(m_renderer);

            SDL_Rect clipRect = { (int)Otstup, 0, (int)m_height, (int)m_height };
            SDL_SetRenderClipRect(m_renderer, &clipRect);

            float scale = zoom * (m_height / TEX_SIZE);
            float tileTexSize = TEX_SIZE / N;
            float offsetX = fmod(panX * tileTexSize, TEX_SIZE);
            if (offsetX < 0) offsetX += TEX_SIZE;
            float offsetY = fmod(panY * tileTexSize, TEX_SIZE);
            if (offsetY < 0) offsetY += TEX_SIZE;

            float destW = TEX_SIZE * scale;
            float destH = TEX_SIZE * scale;

            float startX = Otstup - offsetX * scale;
            float startY = -offsetY * scale;

            for (float y = startY; y < m_height; y += destH) {
                for (float x = startX; x < Otstup + m_height; x += destW) {
                    SDL_FRect dest = { x, y, destW, destH };
                    SDL_RenderTexture(m_renderer, m_mapTexture, nullptr, &dest);
                }
            }

            SDL_SetRenderClipRect(m_renderer, nullptr);

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
            const float scale = 0.02f / (N / 256.0f);
            const float T = N * scale;

            const int gridSize = 2 * N;
            std::vector<float> grid(gridSize * gridSize);

            #pragma omp parallel for
            for (int gi = 0; gi < gridSize; ++gi) {
                for (int gj = 0; gj < gridSize; ++gj) {
                    int ix = gi - N;
                    int iy = gj - N;
                    float x = ix * scale;
                    float y = iy * scale;

                    float amp = 1.0f;
                    float freq = 1.0f;
                    float value = 0.0f;
                    float maxAmp = 0.0f;
                    for (int o = 0; o < octaves; ++o) {
                        value += amp * noise(x * freq, y * freq);
                        maxAmp += amp;
                        amp *= persistence;
                        freq *= lacunarity;
                    }
                    grid[gi * gridSize + gj] = value / maxAmp;
                }
            }

            std::vector<float> rawValues(N * N);
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    float wx = (float)i / N;
                    float wy = (float)j / N;

                    float v00 = grid[(i + N) * gridSize + (j + N)];
                    float v10 = grid[i * gridSize + (j + N)];
                    float v01 = grid[(i + N) * gridSize + j];
                    float v11 = grid[i * gridSize + j];

                    float raw = (1.0f - wx) * (1.0f - wy) * v00
                            + wx * (1.0f - wy) * v10
                            + (1.0f - wx) * wy * v01
                            + wx * wy * v11;

                    rawValues[i * N + j] = raw;
                }
            }

            std::vector<float> sorted = rawValues;
            std::sort(sorted.begin(), sorted.end());
            size_t thresholdIndex = static_cast<size_t>(sorted.size() * 0.33f);
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

                    Map[i * N + j] = {static_cast<uint16_t>(i), static_cast<uint16_t>(j), 
                    static_cast<uint8_t>(biome), static_cast<int8_t>(zone)};
                }
            }
        }

        void SmoothClimate(int iterations) {
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

        void ApplyCoastalInfluence(int iterations) {
            std::vector<Tile> newMap = Map;
            for (int iter = 0; iter < iterations; ++iter) {
                for (int i = 0; i < N; ++i) {
                    for (int j = 0; j < N; ++j) {
                        const Tile& tile = Map[i * N + j];
                        if (tile.biome == 1) {
                            newMap[i * N + j].zone = -1;
                            continue;
                        }

                        float pos = (float)i / N;
                        float p = pos <= 0.5f ? pos : 1.0f - pos;
                        int base_zone;
                        if (p <= 0.1f) base_zone = 0;
                        else if (p <= 0.2f) base_zone = 1;
                        else if (p <= 0.3f) base_zone = 2;
                        else if (p <= 0.4f) base_zone = 3;
                        else if (p <= 0.47f) base_zone = 4;
                        else base_zone = 5;

                        int newZone = tile.zone;

                        bool waterNear = false;
                        bool tropicalNear = false;
                        bool temperateNear = false;
                        bool desertNear = false;
                        for (int di = -1; di <= 1; ++di) {
                            for (int dj = -1; dj <= 1; ++dj) {
                                if (di == 0 && dj == 0) continue;
                                int ni = (i + di + N) % N;
                                int nj = (j + dj + N) % N;
                                const Tile& nb = Map[ni * N + nj];
                                if (nb.biome == 1)
                                    waterNear = true;
                                else if (nb.zone == 5)
                                    tropicalNear = true;
                                else if (nb.zone == 3)
                                    temperateNear = true;
                                else if (nb.zone == 4)
                                    desertNear = true;
                            }
                        }

                        if (tile.zone == 4) {
                            if (base_zone >= 4 && (waterNear || tropicalNear)) {
                                if (rand() % 100 < 80) {
                                    newZone = 5;
                                }
                            }
                            if (newZone != 5) {
                                if (base_zone <= 3 && waterNear && !tropicalNear) {
                                    newZone = 3;
                                }
                                else if (temperateNear && !tropicalNear && !waterNear) {
                                    newZone = 3;
                                }
                            }
                            
                        }

                        newMap[i * N + j].zone = newZone;
                    }
                }
                std::swap(Map, newMap);
            }
        }
        void HandleTileClick(int mouseX, int mouseY) {
            if (mouseX >= Otstup && mouseX <= Otstup + m_height && mouseY >= 0 && mouseY <= m_height) {
                float tileSize = (m_height / N) * zoom;
                float worldX = panX + (mouseX - Otstup) / tileSize;
                float worldY = panY + mouseY / tileSize;
                int ix = static_cast<int>(std::floor(worldX)) % N;
                if (ix < 0) ix += N;
                int iy = static_cast<int>(std::floor(worldY)) % N;
                if (iy < 0) iy += N;

                const Tile& tile = Map[iy * N + ix];
                SDL_Log("Tile clicked: (%d, %d) biome=%d zone=%d", ix, iy, tile.biome, tile.zone);
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
                int mouseX = static_cast<int>(event.button.x);
                int mouseY = static_cast<int>(event.button.y);
                if (m_buttonsObjects[0].GetButtonAt(mouseX, mouseY)) {
                    if(m_state) *m_state = 1;
                    return;
                }
                HandleTileClick(mouseX, mouseY);
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
            const float maxZoom = (N/256.0f)*15.0f;
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
            uint16_t x,y;
            uint8_t biome;
            int8_t zone;
        };
        
        void InitButtons(){
            m_buttons={"..."};
        }
    
        private:
        SDL_Texture* m_mapTexture;
        SDL_Surface* m_atlasSurface;
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
        uint8_t* m_state;
        static constexpr float TEX_SIZE = 4096.0f;

        void CreateMapTexture() {
            if (!m_atlasSurface) return;

            SDL_Surface* surface = SDL_CreateSurface(TEX_SIZE, TEX_SIZE, SDL_PIXELFORMAT_RGBA8888);
            if (!surface) return;

            const SDL_PixelFormatDetails* dstFormat = SDL_GetPixelFormatDetails(surface->format);
            if (!dstFormat) {
                SDL_DestroySurface(surface);
                return;
            }

            float baseTileSize = TEX_SIZE / N;
            Uint32* dstPixels = (Uint32*)surface->pixels;
            int dstPitch = surface->pitch / 4;

            Uint32* srcPixels = (Uint32*)m_atlasSurface->pixels;
            int srcPitch = m_atlasSurface->pitch / 4;
            int atlasTileSize = m_atlasSurface->w / 8;

            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    int tileType;
                    if (tile.biome == 0) {
                        tileType = tile.zone;
                    } else {
                        tileType = 6;
                    }

                    int srcX0 = 0;
                    int srcY0 = tileType * atlasTileSize;

                    int dstX0 = static_cast<int>(j * baseTileSize);
                    int dstY0 = static_cast<int>(i * baseTileSize);

                    for (int y = 0; y < atlasTileSize; ++y) {
                        for (int x = 0; x < atlasTileSize; ++x) {
                            Uint32 srcPixel = srcPixels[(srcY0 + y) * srcPitch + (srcX0 + x)];
                            dstPixels[(dstY0 + y) * dstPitch + (dstX0 + x)] = srcPixel;
                        }
                    }
                }
            }

            m_mapTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
            SDL_DestroySurface(surface);

            if (m_mapTexture) {
                SDL_SetTextureScaleMode(m_mapTexture, SDL_SCALEMODE_PIXELART);
            }
        }
};
