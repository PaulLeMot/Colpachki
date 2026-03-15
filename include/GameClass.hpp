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
                const auto& capitals = m_generator.GetCapitals();
                if (!capitals.empty()) {
                    std::uniform_int_distribution<int> pick(0, capitals.size() - 1);
                    int idx = pick(m_rng);
                    m_playerCapitalX = capitals[idx].first;
                    m_playerCapitalY = capitals[idx].second;

                    zoom = 15.0f;

                    float baseTileSize = m_height / N;
                    float scaledTileSize = baseTileSize * zoom;
                    float targetWorldX = m_playerCapitalX + 0.5f;
                    float targetWorldY = m_playerCapitalY + 0.5f;

                    panX = targetWorldX - (m_width / 2.0f - Otstup) / scaledTileSize;
                    panY = targetWorldY - (m_height / 2.0f) / scaledTileSize;

                    panX = fmod(panX, (float)N);
                    if (panX < 0) panX += N;
                    panY = fmod(panY, (float)N);
                    if (panY < 0) panY += N;
                }
                Map[m_playerCapitalY * N + m_playerCapitalX].buildingLevel = 1;
                InitButtons();
                m_buttonsObjects.emplace_back(
                    m_renderer,m_font,m_buttons[0],
                    m_width, m_height,
                    m_width-(m_width/20),
                    0,
                    m_width/20,m_width/20,210,210,210);
                float freeWidth = m_width - (Otstup + m_height);
                float infoW = freeWidth * 0.8f;
                float infoX = Otstup + m_height + (freeWidth - infoW) / 2;
                float infoY = m_height * 0.75f;
                float infoH = 30.0f;
                m_infoLabel = std::make_unique<Label>(
                    m_renderer, m_font, "",
                    infoX, infoY, infoW, infoH,
                    0, 0, 0,
                    255, 255, 255 
                );
                m_infoLabel->SetActive(false);
                float upgrW = freeWidth * 0.8f;
                float upgrY = m_height * 0.75f;
                float upgrH = 30.0f;
                m_upgradeButton = std::make_unique<Button>(
                    m_renderer, m_font, "Upgrade",
                    m_width, m_height,
                    20, upgrY, upgrW, upgrH,
                    210, 210, 210
                );
                float recruitY = upgrY + upgrH + 10;
                m_recruitButton = std::make_unique<Button>(
                    m_renderer, m_font, "Recruit",
                    m_width, m_height,
                    20, recruitY, upgrW, upgrH,
                    210, 210, 210
                );
                float btnH = m_width / 20;
                float btnW = m_width / 20;
                float leftBound = Otstup + m_height;
                float rightBound = m_width - btnW - 2;
                float availableWidth = rightBound - leftBound;
                const float gap = 2;
                float btnWidth = (availableWidth - 3 * gap) / 4;
                if (btnWidth < 10) btnWidth = 10;

                const char* speedLabels[4] = { "II", ">", ">>", ">>>" };
                for (int i = 0; i < 4; ++i) {
                    float x = leftBound + i * (btnWidth + gap);
                    m_speedButtons.emplace_back(
                        m_renderer, m_font, speedLabels[i],
                        m_width, m_height,
                        x, 0, btnWidth, btnH,
                        210, 210, 210
                    );
                }
                CreateMapTexture();
                //m_atlasTexture = SDL_CreateTextureFromSurface(m_renderer, m_atlasSurface);
                //if (!m_atlasTexture) {
                //    SDL_Log("Failed to create atlas texture: %s", SDL_GetError());
                //}
                //if (m_atlasTexture) {
                //    SDL_SetTextureScaleMode(m_atlasTexture, SDL_SCALEMODE_PIXELART);
                //}
                m_atlasTileSize = m_atlasSurface->w / 8;
        }
        ~Game() {
            if (m_mapTexture) SDL_DestroyTexture(m_mapTexture);
            if (m_atlasSurface) SDL_DestroySurface(m_atlasSurface);
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
            if (m_hasSelection) {
                const Tile& tile = Map[m_selY * N + m_selX];
                Uint8 r = 255, g = 255, b = 255;
                if (tile.zone == 0) {
                    r = 0; g = 0; b = 20;
                }

                float tileSize = (m_height / N) * zoom;
                float baseX = Otstup + (m_selX - panX) * tileSize;
                float baseY = (m_selY - panY) * tileSize;
                float thickness = tileSize * 0.0625f;

                float worldSizePx = m_height * zoom;

                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        float screenX = baseX + dx * worldSizePx;
                        float screenY = baseY + dy * worldSizePx;

                        if (screenX + tileSize < 0 || screenX > m_width || 
                            screenY + tileSize < 0 || screenY > m_height) continue;

                        SDL_SetRenderDrawColor(m_renderer, r, g, b, 255);
                        SDL_FRect topRect = { screenX, screenY, tileSize, thickness };
                        SDL_RenderFillRect(m_renderer, &topRect);
                        SDL_FRect bottomRect = { screenX, screenY + tileSize - thickness, tileSize, thickness };
                        SDL_RenderFillRect(m_renderer, &bottomRect);
                        SDL_FRect leftRect = { screenX, screenY, thickness, tileSize };
                        SDL_RenderFillRect(m_renderer, &leftRect);
                        SDL_FRect rightRect = { screenX + tileSize - thickness, screenY, thickness, tileSize };
                        SDL_RenderFillRect(m_renderer, &rightRect);
                    }
                }
            }
            SDL_SetRenderClipRect(m_renderer, nullptr);

            m_buttonsObjects[0].RenderButton();
            if (m_menuOpen) {
                for (auto& btn : m_menuButtons) {
                    btn.RenderButton();
                }
            }
            for (auto& btn : m_speedButtons) {
                btn.RenderButton();
            }
            m_infoLabel->Render();
            if (CanRecruit()) {
                m_recruitButton->RenderButton();
                if (CanUpgrade()) {
                    m_upgradeButton->RenderButton();
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
                m_hasSelection = true;
                m_selX = ix;
                m_selY = iy;
                std::string info = GetTileInfo(tile);
                m_infoLabel->SetText(info);
                m_infoLabel->SetActive(true);
                if (tile.buildingLevel == 3) {
                    SDL_Log("Capital RGB: %d %d %d", tile.capitalColor.r, tile.capitalColor.g, tile.capitalColor.b);
                }
            }
        }
        void HandleEvent(const SDL_Event& event) {
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int mouseX = static_cast<int>(event.button.x);
                int mouseY = static_cast<int>(event.button.y);
                if (CanUpgrade() && m_upgradeButton->GetButtonAt(mouseX, mouseY)) {
                    UpgradeCapital();
                    return;
                }
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
                case SDLK_ESCAPE:
                m_hasSelection = false;
                m_infoLabel->SetActive(false);
                break;
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
            float btnX = m_width - m_width/20;
            float btnY = 0;
            float btnW = m_width/20;
            float btnH = m_width/20;
            m_menuButtons.emplace_back(
                m_renderer, m_font, "Exit", m_width, m_height,
                btnX, btnY + btnH, btnW, btnH,
                210, 210, 210
            );
            m_menuButtons.emplace_back(
                m_renderer, m_font, "*", m_width, m_height,
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
        std::vector<Button>m_buttonsObjects, m_speedButtons;
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
        bool m_hasSelection = false;
        int m_selX = 0, m_selY = 0;
        std::unique_ptr<Label> m_infoLabel;
        std::unique_ptr<Button> m_upgradeButton;
        std::unique_ptr<Button> m_recruitButton;
        uint16_t m_playerCapitalX = 0, m_playerCapitalY = 0;

        bool CanUpgrade() const {
            return m_hasSelection && 
                m_selX == m_playerCapitalX && 
                m_selY == m_playerCapitalY && 
                Map[m_selY * N + m_selX].buildingLevel < 3;
        }
        bool CanRecruit() const {
            return m_hasSelection && 
                m_selX == m_playerCapitalX && 
                m_selY == m_playerCapitalY;
        }

        void CreateMapTexture() {
            if (m_mapTexture) {
                SDL_DestroyTexture(m_mapTexture);
                m_mapTexture = nullptr;
            }
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
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    if (tile.capitalColor.a != 0) {
                        int dstX0 = static_cast<int>(j * baseTileSize);
                        int dstY0 = static_cast<int>(i * baseTileSize);
                        Uint8 a_src = 64;
                        Uint8 r_src = tile.capitalColor.r;
                        Uint8 g_src = tile.capitalColor.g;
                        Uint8 b_src = tile.capitalColor.b;

                        for (int y = 0; y < atlasTileSize; ++y) {
                            for (int x = 0; x < atlasTileSize; ++x) {
                                Uint32& dstPixel = dstPixels[(dstY0 + y) * dstPitch + (dstX0 + x)];
                                Uint8 r_dst = (dstPixel >> 24) & 0xFF;
                                Uint8 g_dst = (dstPixel >> 16) & 0xFF;
                                Uint8 b_dst = (dstPixel >> 8) & 0xFF;
                                Uint8 a_dst = dstPixel & 0xFF;

                                float a = a_src / 255.0f;
                                Uint8 r = static_cast<Uint8>(r_src * a + r_dst * (1.0f - a));
                                Uint8 g = static_cast<Uint8>(g_src * a + g_dst * (1.0f - a));
                                Uint8 b = static_cast<Uint8>(b_src * a + b_dst * (1.0f - a));
                                Uint8 a_out = a_src + a_dst * (1.0f - a);
                                dstPixel = (r << 24) | (g << 16) | (b << 8) | a_out;
                            }
                        }
                    }
                }
            }

            int cornerSize = static_cast<int>(baseTileSize / 16.0f + 0.5f);
            int tileSizePx = static_cast<int>(baseTileSize);
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    if (tile.capitalColor.a != 0) {
                        int dstX0 = static_cast<int>(j * baseTileSize);
                        int dstY0 = static_cast<int>(i * baseTileSize);
                        Uint32 color = (tile.capitalColor.r << 24) |
                                    (tile.capitalColor.g << 16) |
                                    (tile.capitalColor.b << 8) |
                                    0xFF;

                        for (int y = 0; y < cornerSize; ++y)
                            for (int x = 0; x < tileSizePx; ++x)
                                dstPixels[(dstY0 + y) * dstPitch + (dstX0 + x)] = color;
                        for (int y = 0; y < cornerSize; ++y)
                            for (int x = 0; x < tileSizePx; ++x)
                                dstPixels[(dstY0 + tileSizePx - cornerSize + y) * dstPitch + (dstX0 + x)] = color;
                        for (int x = 0; x < cornerSize; ++x)
                            for (int y = 0; y < tileSizePx; ++y)
                                dstPixels[(dstY0 + y) * dstPitch + (dstX0 + x)] = color;
                        for (int x = 0; x < cornerSize; ++x)
                            for (int y = 0; y < tileSizePx; ++y)
                                dstPixels[(dstY0 + y) * dstPitch + (dstX0 + tileSizePx - cornerSize + x)] = color;
                    }
                }
            }
            m_mapTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
            SDL_DestroySurface(surface);
            if (m_mapTexture) {
                SDL_SetTextureScaleMode(m_mapTexture, SDL_SCALEMODE_PIXELART);
            }
        }
    std::string GetTileInfo(const Tile& tile) const {
        if (tile.biome == 1) return "Waters";
        std::string info;
        switch (tile.zone) {
            case 0: info = "Snowy"; break;
            case 1: info = "Taiga"; break;
            case 2: info = "Temperate"; break;
            case 3: info = "Savanna"; break;
            case 4: info = "Desert"; break;
            case 5: info = "Jungle"; break;
            default: info = "Unknown";
        }
        if (tile.mountain) info += " Mountains";
        else if (tile.forest) info += " Forest";
        else if (tile.zone == 4 && !tile.mountain && !tile.forest) info += "";
        else info += " Plains";
        return info;
    }

    void UpgradeCapital() {
        if (!CanUpgrade()) return;
        Tile& tile = Map[m_playerCapitalY * N + m_playerCapitalX];
        int oldLevel = tile.buildingLevel;
        
        if (oldLevel == 1) {
            tile.buildingLevel = 2;
        } else if (oldLevel == 2) {
            tile.buildingLevel = 3;
            if (tile.capitalColor.a == 0) {
                std::uniform_int_distribution<int> hueDist(0, 359);
                float h = hueDist(m_rng) / 360.0f;
                float r, g, b;
                auto hsv_to_rgb = [](float h, float& r, float& g, float& b) {
                    int i = int(h * 6);
                    float f = h * 6 - i;
                    float p = 0, q = 0, t = 0;
                    switch (i % 6) {
                        case 0: r = 1; g = t; b = p; break;
                        case 1: r = q; g = 1; b = p; break;
                        case 2: r = p; g = 1; b = t; break;
                        case 3: r = p; g = q; b = 1; break;
                        case 4: r = t; g = p; b = 1; break;
                        case 5: r = 1; g = p; b = q; break;
                    }
                };
                hsv_to_rgb(h, r, g, b);
                tile.capitalColor = { (Uint8)(r*255), (Uint8)(g*255), (Uint8)(b*255), 255 };
            }
        } else if (oldLevel == 3) {
            return;
        }
        
        CreateMapTexture();
        m_infoLabel->SetText(GetTileInfo(tile));
    }
};
