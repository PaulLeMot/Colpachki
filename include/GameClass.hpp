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
                //m_generator.AdjustMountainZones(Map, N);
                m_generator.RemoveIsolatedMountains(Map, N);
                m_generator.AdjustMountainZones(Map, N);
                m_generator.GenerateForest(Map, m_perm, N);
                //GenerateRivers();
                const auto& capitals = m_generator.GetCapitals();
                m_playerColors.resize(capitals.size());
                for (size_t i = 0; i < capitals.size(); ++i) {
                    int x = capitals[i].first;
                    int y = capitals[i].second;
                    m_playerColors[i] = Map[y * N + x].capitalColor;
                    Map[y * N + x].owner = i;
                }
                if (!capitals.empty()) {
                    std::uniform_int_distribution<int> pick(0, capitals.size() - 1);
                    int idx = pick(m_rng);
                    m_playerCapitalX = capitals[idx].first;
                    m_playerCapitalY = capitals[idx].second;
                }
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
                float unitsY = infoY + infoH + 5;
                m_unitsLabel = std::make_unique<Label>(
                    m_renderer, m_font, "",
                    infoX, unitsY, infoW, infoH,
                    0, 0, 0,
                    255, 255, 255
                );
                m_unitsLabel->SetActive(false);
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
                m_gameTimeHours = 0.0f;
                m_speedMode = 0;
                float progressBarY = btnH + 5 + btnH;
                float progressBarX = Otstup + m_height;
                float progressBarW = (m_width - m_width/20 - 2) - progressBarX;
                m_progressBarX = progressBarX;
                m_progressBarY = progressBarY;
                m_progressBarW = progressBarW;
                m_progressBarH = 10;
                m_year = 0;
                m_month = 1;
                m_day = 1;
                float calendarY = btnH + 5;
                float calendarX = Otstup + m_height;
                float calendarW = m_progressBarW;
                float calendarH = btnH * 0.6f;

                m_calendarLabel = std::make_unique<Label>(
                    m_renderer, m_font,
                    "Year 0 Month 1 Day 1",
                    calendarX, calendarY, calendarW, calendarH,
                    0, 0, 0,
                    240, 240, 240
                );
                m_calendarLabel->SetActive(true);
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
for (const auto& mov : m_movements) {
    if (m_hasSelection && mov.fromX == m_selX && mov.fromY == m_selY) {
        const float thickness = 5.0f;
        float worldSizePx = m_height * zoom;
        float tileSize = (m_height / N) * zoom;
        float maxDist2 = tileSize * tileSize * 1.5f;

        SDL_FPoint from_base = getTileScreenCenter(mov.fromX, mov.fromY);
        SDL_FPoint to_base = getTileScreenCenter(mov.toX, mov.toY);
        for (int dfx = -1; dfx <= 1; ++dfx) {
            for (int dfy = -1; dfy <= 1; ++dfy) {
                SDL_FPoint from = { from_base.x + dfx * worldSizePx, from_base.y + dfy * worldSizePx };
                for (int dtx = -1; dtx <= 1; ++dtx) {
                    for (int dty = -1; dty <= 1; ++dty) {
                        SDL_FPoint to = { to_base.x + dtx * worldSizePx, to_base.y + dty * worldSizePx };

                        float dist2 = (to.x - from.x)*(to.x - from.x) + (to.y - from.y)*(to.y - from.y);
                        if (dist2 > maxDist2) continue;

                        float dx_line = to.x - from.x;
                        float dy_line = to.y - from.y;

                        if (fabs(dx_line) > fabs(dy_line)) {
                            float len = fabs(dx_line);
                            float dir = (dx_line > 0) ? 1.0f : -1.0f;
                            float y = from.y - thickness/2;
                            float xStart = std::min(from.x, to.x);

                            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
                            SDL_FRect bgRect = { xStart, y, len, thickness };
                            SDL_RenderFillRect(m_renderer, &bgRect);

                            float progressX = from.x + dir * (len * mov.progress);
                            float fillStart = std::min(from.x, progressX);
                            float fillWidth = fabs(progressX - from.x);
                            SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
                            SDL_FRect fillRect = { fillStart, y, fillWidth, thickness };
                            SDL_RenderFillRect(m_renderer, &fillRect);
                        } else {
                            float len = fabs(dy_line);
                            float dir = (dy_line > 0) ? 1.0f : -1.0f;
                            float x = from.x - thickness/2;
                            float yStart = std::min(from.y, to.y);

                            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
                            SDL_FRect bgRect = { x, yStart, thickness, len };
                            SDL_RenderFillRect(m_renderer, &bgRect);

                            float progressY = from.y + dir * (len * mov.progress);
                            float fillStart = std::min(from.y, progressY);
                            float fillHeight = fabs(progressY - from.y);
                            SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
                            SDL_FRect fillRect = { x, fillStart, thickness, fillHeight };
                            SDL_RenderFillRect(m_renderer, &fillRect);
                        }
                    }
                }
            }
        }
    }
}
            m_buttonsObjects[0].RenderButton();
            if (m_menuOpen) {
                for (auto& btn : m_menuButtons) {
                    btn.RenderButton();
                }
            }
            for (auto& btn : m_speedButtons) {
                btn.RenderButton();
            }
            SDL_SetRenderDrawColor(m_renderer, 240, 240, 240, 255);
            SDL_FRect bgRect = { m_progressBarX, m_progressBarY, m_progressBarW, m_progressBarH };
            SDL_RenderFillRect(m_renderer, &bgRect);

            float fillWidth = (m_gameTimeHours / 24.0f) * m_progressBarW;
            SDL_SetRenderDrawColor(m_renderer, 0, 210, 0, 255);
            SDL_FRect fillRect = { m_progressBarX, m_progressBarY, fillWidth, m_progressBarH };
            SDL_RenderFillRect(m_renderer, &fillRect);
            if (m_calendarLabel) {
                m_calendarLabel->Render();
            }
            m_infoLabel->Render();
            m_unitsLabel->Render();
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
                if (tile.buildingLevel > 0) {
                    SDL_Log("build RGB: %d %d %d", tile.capitalColor.r, tile.capitalColor.g, tile.capitalColor.b);
                }
                if (!tile.units.empty()) {
                    std::string unitsText = "Units: ";
                    for (size_t i = 0; i < tile.units.size(); ++i) {
                        if (i > 0) unitsText += ", ";
                        unitsText += tile.units[i].id;
                    }
                    m_unitsLabel->SetText(unitsText);
                    m_unitsLabel->SetActive(true);
                } else {
                    m_unitsLabel->SetActive(false);
                }
            }
        }
        void HandleEvent(const SDL_Event& event) {
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int mouseX = static_cast<int>(event.button.x);
                int mouseY = static_cast<int>(event.button.y);

                if (event.button.button == SDL_BUTTON_LEFT) {
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
                        } else {
                            m_menuOpen = false;
                        }
                    }
                    for (int i = 0; i < m_speedButtons.size(); ++i) {
                        if (m_speedButtons[i].GetButtonAt(mouseX, mouseY)) {
                            switch (i) {
                                case 0: m_speedMode = 0; break;
                                case 1: m_speedMode = 1; break;
                                case 2: m_speedMode = 2; break;
                                case 3: m_speedMode = 3; break;
                            }
                            return;
                        }
                    }
                    if (CanRecruit() && m_recruitButton->GetButtonAt(mouseX, mouseY)) {
                        Recruit();
                        return;
                    }
                    HandleTileClick(mouseX, mouseY);
                    return;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    if (m_hasSelection) {
                        Tile& srcTile = Map[m_selY * N + m_selX];
                        if (!srcTile.units.empty()) {
                            float tileSize = (m_height / N) * zoom;
                            float worldX = panX + (mouseX - Otstup) / tileSize;
                            float worldY = panY + mouseY / tileSize;
                            int ix = static_cast<int>(std::floor(worldX)) % N;
                            if (ix < 0) ix += N;
                            int iy = static_cast<int>(std::floor(worldY)) % N;
                            if (iy < 0) iy += N;

                            if (Map[iy * N + ix].biome == 1) {
                                SDL_Log("Cannot move to water");
                                return;
                            }
                            if (!Map[iy * N + ix].units.empty()) {
                                SDL_Log("Target tile already has a unit");
                                return;
                            }
                            if (Map[iy * N + ix].owner != -1 && Map[iy * N + ix].owner != srcTile.owner) {
                                SDL_Log("Target tile already belongs to another player");
                                return;
                            }
                            if (isTileInMovementAsFrom(m_selX, m_selY)) {
                                SDL_Log("Source tile is already moving");
                                return;
                            }
                            if (isTileInMovementAsTo(ix, iy)) {
                                SDL_Log("Target tile is already targeted by another movement");
                                return;
                            }
                            int dx = abs(ix - (int)m_selX);
                            int dy = abs(iy - (int)m_selY);
                            dx = std::min(dx, N - dx);
                            dy = std::min(dy, N - dy);
                            if (dx + dy != 1) {
                                SDL_Log("Target is not a neighboring tile");
                                return;
                            }

                            Movement mov;
                            mov.fromX = m_selX;
                            mov.fromY = m_selY;
                            mov.toX = ix;
                            mov.toY = iy;
                            mov.progress = 0.0f;
                            m_movements.push_back(mov);
                            //m_hasSelection = false;
                            //m_infoLabel->SetActive(false);
                            SDL_Log("Movement started from (%d,%d) to (%d,%d)",
                                mov.fromX, mov.fromY, mov.toX, mov.toY);
                        }
                    }
                    return;
                }
                else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    isDragging = true;
                    startMouseX = event.button.x;
                    startMouseY = event.button.y;
                    startPanX = panX;
                    startPanY = panY;
                    return;
                }
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
                        m_unitsLabel->SetActive(false);
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
            if (m_speedMode != 0) {
                float speedFactor = 1.0f;
                if (m_speedMode == 2) speedFactor = 6.0f;
                else if (m_speedMode == 3) speedFactor = 12.0f;
                m_gameTimeHours += deltaTime * speedFactor;

                if (m_gameTimeHours >= 24.0f) {
                    m_gameTimeHours = fmod(m_gameTimeHours, 24.0f);
                    m_day++;
                    if (m_day > 30) {
                        m_day = 1;
                        m_month++;
                        if (m_month > 12) {
                            m_month = 1;
                            m_year++;
                        }
                    }
                    std::string calendarText = "Year " + std::to_string(m_year) +
                                            " Month " + std::to_string(m_month) +
                                            " Day " + std::to_string(m_day);
                    m_calendarLabel->SetText(calendarText);
                }
            }
            if (!m_movements.empty()) {
                float speedFactor = 0.0f;
                if (m_speedMode == 1) speedFactor = 1.0f;
                else if (m_speedMode == 2) speedFactor = 6.0f;
                else if (m_speedMode == 3) speedFactor = 12.0f;

                if (speedFactor > 0.0f) {
                    for (auto it = m_movements.begin(); it != m_movements.end(); ) {
                        it->progress += deltaTime * speedFactor / MOVE_DURATION_HOURS;
                        if (it->progress >= 1.0f) {
                            it->progress = 1.0f;
                            Tile& srcTile = Map[it->fromY * N + it->fromX];
                            Tile& dstTile = Map[it->toY * N + it->toX];
                            dstTile.units = std::move(srcTile.units);
                            srcTile.units.clear();
                            dstTile.owner = srcTile.owner;
                            CreateMapTexture();
                            SDL_Log("Movement completed: (%d,%d) -> (%d,%d)", it->fromX, it->fromY, it->toX, it->toY);
                            if (m_hasSelection && m_selX == it->fromX && m_selY == it->fromY) {
                                m_hasSelection = false;
                                m_infoLabel->SetActive(false);
                                m_unitsLabel->SetActive(false);
                            }
                            it = m_movements.erase(it);
                        } else {
                            ++it;
                        }
                    }
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
        std::unique_ptr<Label> m_unitsLabel;
        std::unique_ptr<Button> m_upgradeButton;
        std::unique_ptr<Button> m_recruitButton;
        uint16_t m_playerCapitalX = 0, m_playerCapitalY = 0;
        //time
        float m_gameTimeHours;
        int m_speedMode;
        float m_progressBarX, m_progressBarY, m_progressBarW, m_progressBarH;
        //calendar
        int m_year;
        int m_month;
        int m_day;
        std::unique_ptr<Label> m_calendarLabel;
        static constexpr float MOVE_DURATION_HOURS = 12.0f;
        std::vector<SDL_Color> m_playerColors;
        int m_nextUnitId = 1;
        struct Movement {
            int fromX, fromY;
            int toX, toY;
            float progress;
        };
        std::vector<Movement> m_movements;
        bool isTileInMovementAsFrom(int x, int y) const {
            for (const auto& m : m_movements) {
                if (m.fromX == x && m.fromY == y) return true;
            }
            return false;
        }
        bool isTileInMovementAsTo(int x, int y) const {
            for (const auto& m : m_movements) {
                if (m.toX == x && m.toY == y) return true;
            }
            return false;
        }
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
        void Recruit() {
            if (!CanRecruit()) return;
            Tile& tile = Map[m_playerCapitalY * N + m_playerCapitalX];
            UnitInfo newUnit;
            newUnit.level = 1;
            newUnit.id = "U" + std::to_string(m_nextUnitId++);
            tile.units.push_back(newUnit);
            CreateMapTexture();
            if (m_hasSelection && m_selX == m_playerCapitalX && m_selY == m_playerCapitalY) {
                if (!tile.units.empty()) {
                    std::string unitsText = "Units: ";
                    for (size_t i = 0; i < tile.units.size(); ++i) {
                        if (i > 0) unitsText += ", ";
                        unitsText += tile.units[i].id;
                    }
                    m_unitsLabel->SetText(unitsText);
                    m_unitsLabel->SetActive(true);
                } else {
                    m_unitsLabel->SetActive(false);
                }
            }
        }
        SDL_FPoint getTileScreenCenter(int tx, int ty) const {
            float tileSize = (m_height / N) * zoom;
            float screenX = Otstup + (tx - panX) * tileSize + tileSize/2;
            float screenY = (ty - panY) * tileSize + tileSize/2;
            return {screenX, screenY};
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
                    if (tile.owner == -1) continue;
                    SDL_Color color = m_playerColors[tile.owner];
                    Uint8 a_src = 64;
                    Uint8 r_src = color.r;
                    Uint8 g_src = color.g;
                    Uint8 b_src = color.b;
                    int dstX0 = static_cast<int>(j * baseTileSize);
                    int dstY0 = static_cast<int>(i * baseTileSize);
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

            int cornerSize = static_cast<int>(baseTileSize / 16.0f + 0.5f);
            int tileSizePx = static_cast<int>(baseTileSize);
            const int borderThickness = 2;
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    if (tile.owner == -1) continue;
                    SDL_Color color = m_playerColors[tile.owner];
                    Uint32 borderColor = (color.r << 24) | (color.g << 16) | (color.b << 8) | 0xFF;
                    int dstX0 = static_cast<int>(j * baseTileSize);
                    int dstY0 = static_cast<int>(i * baseTileSize);
                    int rightX = dstX0 + tileSizePx - 1;
                    int bottomY = dstY0 + tileSizePx - 1;
                    int ni = (i - 1 + N) % N;
                    if (Map[ni * N + j].owner != tile.owner) {
                        for (int y = dstY0; y < dstY0 + borderThickness; ++y)
                            for (int x = dstX0; x < dstX0 + tileSizePx; ++x)
                                dstPixels[y * dstPitch + x] = borderColor;
                    }
                    ni = (i + 1) % N;
                    if (Map[ni * N + j].owner != tile.owner) {
                        for (int y = bottomY - borderThickness + 1; y <= bottomY; ++y)
                            for (int x = dstX0; x < dstX0 + tileSizePx; ++x)
                                dstPixels[y * dstPitch + x] = borderColor;
                    }
                    int nj = (j - 1 + N) % N;
                    if (Map[i * N + nj].owner != tile.owner) {
                        for (int x = dstX0; x < dstX0 + borderThickness; ++x)
                            for (int y = dstY0; y < dstY0 + tileSizePx; ++y)
                                dstPixels[y * dstPitch + x] = borderColor;
                    }
                    nj = (j + 1) % N;
                    if (Map[i * N + nj].owner != tile.owner) {
                        for (int x = rightX - borderThickness + 1; x <= rightX; ++x)
                            for (int y = dstY0; y < dstY0 + tileSizePx; ++y)
                                dstPixels[y * dstPitch + x] = borderColor;
                    }
                }
            }
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    if (!tile.units.empty()) {
                        int dstX0 = static_cast<int>(j * baseTileSize);
                        int dstY0 = static_cast<int>(i * baseTileSize);
                        int unitSrcX0 = (0 + tile.units[0].level) * atlasTileSize;
                        int unitSrcY0 = 6 * atlasTileSize;
                        for (int y = 0; y < atlasTileSize; ++y) {
                            for (int x = 0; x < atlasTileSize; ++x) {
                                Uint32 srcPixel = srcPixels[(unitSrcY0 + y) * srcPitch + (unitSrcX0 + x)];
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
            if (tile.owner != -1) {
                info += " [Player " + std::to_string(tile.owner) + "]";
            }
            if (!tile.units.empty()) {
                info += "\nUnits: ";
                for (size_t i = 0; i < tile.units.size(); ++i) {
                    if (i > 0) info += ", ";
                    info += tile.units[i].id;
                }
            }
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
        if (!tile.units.empty()) {
            std::string unitsText = "Units: ";
            for (size_t i = 0; i < tile.units.size(); ++i) {
                if (i > 0) unitsText += ", ";
                unitsText += tile.units[i].id;
            }
            m_unitsLabel->SetText(unitsText);
            m_unitsLabel->SetActive(true);
        } else {
            m_unitsLabel->SetActive(false);
        }
    }
};
