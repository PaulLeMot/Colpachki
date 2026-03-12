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
#include <queue>
#include "ColpEngine.hpp"
#include "Settings.hpp"
#include <random>
#include <algorithm>
#include <MapGenerator.hpp>

class Game{
    public:
        Game(SDL_Renderer* renderer, TTF_Font* font, 
            const float width, const float height, 
            const std::string Name, const int64_t Seed,
            uint8_t* StatePtr,
            SettingsMenu* settingsMenu) : 
            m_renderer(renderer), m_font(font),
            m_width(width), m_height(height), 
            Name(Name), Seed(Seed), m_state(StatePtr),
            m_mapTexture(nullptr),
            m_settingsMenu(settingsMenu),
            m_rng(Seed), m_generator(m_rng){
                Otstup = (m_width - m_height) / 2;
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
                m_generator.CreateMap(Map, m_heightMap, m_perm, N);
                m_generator.SmoothClimate(Map, N, 2 + (N / 256));
                m_generator.ApplyCoastalInfluence(Map, N, 1);
                m_generator.GenerateCapitals(Map, N);
                m_generator.GenerateRivers(Map, m_heightMap, m_riverSegments, N);
                m_generator.DesrtifyJungles(Map, m_riverSegments, N);
                m_generator.JungleifyDeserts(Map, N);
                m_generator.DesrtifyJungles(Map, m_riverSegments, N);
                m_generator.DesrtifyJungles(Map, m_riverSegments, N);
                m_generator.GenerateMountains(Map, N);
                m_generator.AdjustMountainZones(Map, N);
                m_generator.RemoveIsolatedMountains(Map, N);
                m_generator.GenerateForest(Map, m_perm, N);
                //GenerateRivers();
                InitButtons();
                m_buttonsObjects.emplace_back(
                    m_renderer,m_font,m_buttons[0],
                    m_width, m_height,
                    m_width-(m_width/10),
                    0,
                    m_width/10,m_width/10,240,240,240);
                    CreateMapTexture();
                    //m_atlasTexture = SDL_CreateTextureFromSurface(m_renderer, m_atlasSurface);
                    //if (!m_atlasTexture) {
                    //    SDL_Log("Failed to create atlas texture: %s", SDL_GetError());
                    //}
                    //if (m_atlasTexture) {
                    //    SDL_SetTextureScaleMode(m_atlasTexture, SDL_SCALEMODE_PIXELART);
                    //}
                    m_atlasTileSize = m_atlasSurface->w / 8;
                    SDL_DestroySurface(m_atlasSurface);
                    m_atlasSurface = nullptr;
        }
        ~Game() {
            if (m_mapTexture) SDL_DestroyTexture(m_mapTexture);
            //if (m_atlasTexture) SDL_DestroyTexture(m_atlasTexture);
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
            //RenderBuildings();

            SDL_SetRenderClipRect(m_renderer, nullptr);

            m_buttonsObjects[0].RenderButton();
            if (m_menuOpen) {
                for (auto& btn : m_menuButtons) {
                    btn.RenderButton();
                }
            }
        }
/*       void RenderBuildings() {
            float tileSize = (m_height / N) * zoom;

            float worldLeft   = panX - Otstup / tileSize;
            float worldRight  = worldLeft + m_width / tileSize;
            float worldTop    = panY;
            float worldBottom = panY + m_height / tileSize;

            int startJ = static_cast<int>(std::floor(worldLeft));
            int endJ   = static_cast<int>(std::ceil(worldRight));
            int startI = static_cast<int>(std::floor(worldTop));
            int endI   = static_cast<int>(std::ceil(worldBottom));

            for (int i = startI; i < endI; ++i) {
                for (int j = startJ; j < endJ; ++j) {
                    int ii = i % N;
                    if (ii < 0) ii += N;
                    int jj = j % N;
                    if (jj < 0) jj += N;

                    const Tile& tile = Map[ii * N + jj];
                    if (tile.buildingLevel == 0) continue;

                    float screenX = Otstup + (j - panX) * tileSize;
                    float screenY = (i - panY) * tileSize;

                    int srcX0 = (2 + tile.buildingLevel) * m_atlasTileSize;
                    int srcY0 = (tile.zone >= 0) ? tile.zone * m_atlasTileSize : 0;

                    SDL_FRect srcRect = { (float)srcX0, (float)srcY0, (float)m_atlasTileSize, (float)m_atlasTileSize };
                    SDL_FRect dstRect = { screenX, screenY, tileSize, tileSize };

                    SDL_RenderTexture(m_renderer, m_atlasTexture, &srcRect, &dstRect);
                }
            }
        }*/

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
                    m_menuOpen = !m_menuOpen;
                    return;
                }

                if (m_menuOpen) {
                    if (m_menuButtons[0].GetButtonAt(mouseX, mouseY)) {
                        if (m_state) *m_state = 0;
                        m_menuOpen = false;
                        return;
                    }
                    if (m_menuButtons[1].GetButtonAt(mouseX, mouseY)) {
                        if (m_settingsMenu) {
                            m_settingsMenu->SetReturnState(2);
                            *m_state = 3;
                        }
                        m_menuOpen = false;
                        return;
                    } else m_menuOpen = 0;
                }
                if (event.button.button == SDL_BUTTON_LEFT) {
                    HandleTileClick(mouseX, mouseY);
                }
            }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_MIDDLE) {
            isDragging = true;
            startMouseX = event.button.x;
            startMouseY = event.button.y;
            startPanX = panX;
            startPanY = panY;
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_MIDDLE) {
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
            float sens = m_settingsMenu ? m_settingsMenu->GetZoomSensitivity() : 1.0f;
            float factor = expf(event.wheel.y * m_wheelZoomSpeed * sens);
            if (factor < 0.1f) factor = 0.1f;
            ZoomAt(mouseX, mouseY, factor);
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            switch (event.key.key) {
                case SDLK_W: m_upPressed = true; break;
                case SDLK_S: m_downPressed = true; break;
                case SDLK_A: m_leftPressed = true; break;
                case SDLK_D: m_rightPressed = true; break;
                case SDLK_Z: m_zoomInPressed = true; break;
                case SDLK_X: m_zoomOutPressed = true; break;
            }
        }
        if (event.type == SDL_EVENT_KEY_UP) {
            switch (event.key.key) {
                case SDLK_W: m_upPressed = false; break;
                case SDLK_S: m_downPressed = false; break;
                case SDLK_A: m_leftPressed = false; break;
                case SDLK_D: m_rightPressed = false; break;
                case SDLK_Z: m_zoomInPressed = false; break;
                case SDLK_X: m_zoomOutPressed = false; break;
            }
        }
        }
        void Update(float deltaTime) {
            float moveSpeed = 300.0f / zoom;
            float dx = 0.0f, dy = 0.0f;
            if (m_leftPressed) dx -= moveSpeed * deltaTime;
            if (m_rightPressed) dx += moveSpeed * deltaTime;
            if (m_upPressed) dy -= moveSpeed * deltaTime;
            if (m_downPressed) dy += moveSpeed * deltaTime;
        
            if (dx != 0.0f || dy != 0.0f) {
                panX += dx;
                panY += dy;
                panX = fmod(panX, (float)N);
                if (panX < 0) panX += N;
                panY = fmod(panY, (float)N);
                if (panY < 0) panY += N;
            }
            if (m_zoomInPressed || m_zoomOutPressed) {
                float direction = 0.0f;
                if (m_zoomInPressed) direction += 1.0f;
                if (m_zoomOutPressed) direction -= 1.0f;
                if (direction != 0.0f) {
                    float sens = m_settingsMenu ? m_settingsMenu->GetZoomSensitivity() : 1.0f;
                    float factor = expf(direction * m_keyZoomSpeed * sens * deltaTime);
                    ZoomAt(m_width/2.0f, m_height/2.0f, factor);
                }
            }
        }

        void ZoomAt(float mouseX, float mouseY, float factor) {
            float oldTileSize = (m_height / N) * zoom;
            float worldX = panX + (mouseX - Otstup) / oldTileSize;
            float worldY = panY + mouseY / oldTileSize;
        
            zoom *= factor;
            if (zoom < m_minZoom) zoom = m_minZoom;
            if (zoom > m_maxZoom) zoom = m_maxZoom;
        
            float newTileSize = (m_height / N) * zoom;
            panX = worldX - (mouseX - Otstup) / newTileSize;
            panY = worldY - mouseY / newTileSize;
        
            panX = fmod(panX, (float)N);
            if (panX < 0) panX += N;
            panY = fmod(panY, (float)N);
            if (panY < 0) panY += N;
        }
        
        void InitButtons(){
            m_buttons={"..."};
            float btnX = m_width - m_width/10;
            float btnY = 0;
            float btnW = m_width/10;
            float btnH = m_width/10;
            m_menuButtons.emplace_back(
                m_renderer, m_font, "Exit", m_width, m_height,
                btnX, btnY + btnH, btnW, btnH,
                210, 210, 210
            );
            m_menuButtons.emplace_back(
                m_renderer, m_font, "Settings", m_width, m_height,
                btnX, btnY + 2*btnH, btnW, btnH,
                210, 210, 210
            );
        }
    
        private:
        SDL_Texture* m_mapTexture;
        //SDL_Texture* m_atlasTexture;
        SDL_Surface* m_atlasSurface;
        SDL_Renderer* m_renderer;
        TTF_Font* m_font;
        const float m_width, m_height;
        std::string Name;
        const uint64_t Seed;
        std::vector<int> m_perm;
        const uint16_t N = 256;
        int Otstup=(m_width-m_height)/2;
        int m_atlasTileSize;
        std::vector<std::string>m_buttons;
        std::vector<Button>m_buttonsObjects;
        bool isDragging = false;
        float panX = 0.0f, panY = 0.0f;
        float zoom = 1.0f;
        float startPanX = 0.0f, startPanY = 0.0f;
        int startMouseX = 0, startMouseY = 0;
        uint8_t* m_state;
        static constexpr float TEX_SIZE = 4096.0f;
        bool m_leftPressed = false, m_rightPressed = false, m_upPressed = false, m_downPressed = false;
        bool m_zoomInPressed = false;
        bool m_zoomOutPressed = false;
        float m_minZoom = 1.0f;
        float m_maxZoom = (N/256.0f)*25.0f;
        const float m_keyZoomSpeed = 2.0f;
        const float m_wheelZoomSpeed = 0.1f;
        bool m_menuOpen = false;
        std::vector<Button> m_menuButtons;
        SettingsMenu* m_settingsMenu;
        std::vector<float> m_heightMap;
        std::vector<std::pair<SDL_Point, SDL_Point>> m_riverSegments;
        std::mt19937 m_rng;
        MapGenerator m_generator;
        std::vector<Tile> Map; 

        void CreateMapTexture() {
            if (!m_atlasSurface) return;

            SDL_Surface* surface = SDL_CreateSurface(TEX_SIZE, TEX_SIZE, SDL_PIXELFORMAT_RGBA8888);
            if (!surface) return;

            float baseTileSize = TEX_SIZE / N;
            Uint32* dstPixels = (Uint32*)surface->pixels;
            int dstPitch = surface->pitch / 4;

            Uint32* srcPixels = (Uint32*)m_atlasSurface->pixels;
            int srcPitch = m_atlasSurface->pitch / 4;
            int atlasTileSize = m_atlasSurface->w / 8;

            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    int dstX0 = static_cast<int>(j * baseTileSize);
                    int dstY0 = static_cast<int>(i * baseTileSize);
                    int bgTileType;
                    if (tile.biome == 0) {
                        bgTileType = tile.zone;
                    } else {
                        bgTileType = 6;
                    }
                    int bgSrcX0 = 0;
                    if (tile.mountain) {
                        bgSrcX0 = 1 * atlasTileSize;
                    } else if (tile.forest) {
                        bgSrcX0 = 2 * atlasTileSize;
                    }

                    int bgSrcY0 = bgTileType * atlasTileSize;
                    if (bgTileType < 0) bgSrcY0 = 0;
                    for (int y = 0; y < atlasTileSize; ++y) {
                        for (int x = 0; x < atlasTileSize; ++x) {
                            Uint32 srcPixel = srcPixels[(bgSrcY0 + y) * srcPitch + (bgSrcX0 + x)];
                            dstPixels[(dstY0 + y) * dstPitch + (dstX0 + x)] = srcPixel;
                        }
                    }
                }
            }

const SDL_PixelFormatDetails* dstFormat = SDL_GetPixelFormatDetails(surface->format);

if (!m_riverSegments.empty()) {
    Uint32 riverColor = SDL_MapRGBA(dstFormat, nullptr, 0, 0, 100, 255);
    float thickness = std::max(1.0f, baseTileSize / 6.0f);
    int texSizeI = static_cast<int>(TEX_SIZE);

    for (const auto& seg : m_riverSegments) {
        SDL_Point p1 = seg.first;
        SDL_Point p2 = seg.second;

        if (p1.x < 0 || p1.x > N || p1.y < 0 || p1.y > N ||
            p2.x < 0 || p2.x > N || p2.y < 0 || p2.y > N) continue;

        float x1 = p1.x * baseTileSize;
        float y1 = p1.y * baseTileSize;
        float x2 = p2.x * baseTileSize;
        float y2 = p2.y * baseTileSize;

        if (p1.x == p2.x) {
            float rectX = x1 - thickness * 0.4f;
            float rectY = std::min(y1, y2);
            float rectW = thickness;
            float rectH = baseTileSize;
            for (int dx = -1; dx <= 1; ++dx) {
                float shiftedX = rectX + dx * TEX_SIZE;
                if (shiftedX + rectW < 0 || shiftedX >= TEX_SIZE) continue;
                for (int dy = -1; dy <= 1; ++dy) {
                    float shiftedY = rectY + dy * TEX_SIZE;
                    if (shiftedY + rectH < 0 || shiftedY >= TEX_SIZE) continue;
                    int startX = std::max(0, (int)std::floor(shiftedX));
                    int endX = std::min(texSizeI, (int)std::ceil(shiftedX + rectW));
                    int startY = std::max(0, (int)std::floor(shiftedY));
                    int endY = std::min(texSizeI, (int)std::ceil(shiftedY + rectH));
                    for (int py = startY; py < endY; ++py) {
                        for (int px = startX; px < endX; ++px) {
                            dstPixels[py * dstPitch + px] = riverColor;
                        }
                    }
                }
            }
        } else {
            float rectX = std::min(x1, x2);
            float rectY = y1 - thickness * 0.4f;
            float rectW = baseTileSize;
            float rectH = thickness;
            for (int dx = -1; dx <= 1; ++dx) {
                float shiftedX = rectX + dx * TEX_SIZE;
                if (shiftedX + rectW < 0 || shiftedX >= TEX_SIZE) continue;
                for (int dy = -1; dy <= 1; ++dy) {
                    float shiftedY = rectY + dy * TEX_SIZE;
                    if (shiftedY + rectH < 0 || shiftedY >= TEX_SIZE) continue;
                    int startX = std::max(0, (int)std::floor(shiftedX));
                    int endX = std::min(texSizeI, (int)std::ceil(shiftedX + rectW));
                    int startY = std::max(0, (int)std::floor(shiftedY));
                    int endY = std::min(texSizeI, (int)std::ceil(shiftedY + rectH));
                    for (int py = startY; py < endY; ++py) {
                        for (int px = startX; px < endX; ++px) {
                            dstPixels[py * dstPitch + px] = riverColor;
                        }
                    }
                }
            }
        }
    }
}

            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    if (tile.buildingLevel > 0) {
                        int dstX0 = static_cast<int>(j * baseTileSize);
                        int dstY0 = static_cast<int>(i * baseTileSize);
                        int buildingSrcX0 = (2 + tile.buildingLevel) * atlasTileSize;
                        int buildingSrcY0 = (tile.zone >= 0) ? tile.zone * atlasTileSize : 0;

                        for (int y = 0; y < atlasTileSize; ++y) {
                            for (int x = 0; x < atlasTileSize; ++x) {
                                Uint32 srcPixel = srcPixels[(buildingSrcY0 + y) * srcPitch + (buildingSrcX0 + x)];
                                Uint32& dstPixel = dstPixels[(dstY0 + y) * dstPitch + (dstX0 + x)];
                                Uint8 alpha = (srcPixel >> 24) & 0xFF;
                                if (alpha > 0) {
                                    dstPixel = srcPixel;
                                }
                            }
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
