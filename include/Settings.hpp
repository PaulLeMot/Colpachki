#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <memory>
#include "ColpEngine.hpp"

extern std::vector<uint16_t> btnRGB;

struct SettingsData {
    float zoomSensitivity = 1.0f;
};

inline void SaveSettings(const SettingsData& data) {
    std::filesystem::create_directories("../settings");
    std::ofstream file("../settings/settings.bin", std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&data), sizeof(data));
    }
}

inline SettingsData LoadSettings() {
    SettingsData data;
    std::filesystem::path path = "../settings/settings.bin";
    if (!std::filesystem::exists(path)) {
        SaveSettings(data);
        return data;
    }
    std::ifstream file(path, std::ios::binary);
    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(&data), sizeof(data));
    }
    return data;
}

class SettingsMenu {
public:
    SettingsMenu(SDL_Renderer* renderer, TTF_Font* font,
                 const float width, const float height, uint8_t* StatePtr)
        : m_renderer(renderer), m_font(font),
          m_width(width), m_height(height),
          m_state(StatePtr), m_returnState(0)
    {
        m_settings = LoadSettings();
        m_buttons = { "Back", "+", "-" };
        CreateUI();
    }

    void SetReturnState(uint8_t state) { m_returnState = state; }
    float GetZoomSensitivity() const { return m_settings.zoomSensitivity; }

    void Render() {

        SDL_SetRenderDrawColor(m_renderer, 0, 0, 100, 255);
        SDL_FRect background = {0, 0, m_width, m_height};
        SDL_RenderFillRect(m_renderer, &background);
        for (auto& btn : m_buttonsObjects) btn.RenderButton();
        m_sensitivityLabel->Render();
        m_titleLabel->Render();
    }

    void HandleEvent(const SDL_Event& event) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int mouseX = static_cast<int>(event.button.x);
            int mouseY = static_cast<int>(event.button.y);

            if (m_buttonsObjects[0].GetButtonAt(mouseX, mouseY)) {
                *m_state = m_returnState;
                return;
            }
            if (m_buttonsObjects[1].GetButtonAt(mouseX, mouseY)) {
                m_settings.zoomSensitivity *= 1.1f;
                if (m_settings.zoomSensitivity > 5.0f) m_settings.zoomSensitivity = 5.0f;
                SaveSettings(m_settings);
                UpdateLabel();
                return;
            }
            if (m_buttonsObjects[2].GetButtonAt(mouseX, mouseY)) {
                m_settings.zoomSensitivity /= 1.1f;
                if (m_settings.zoomSensitivity < 0.2f) m_settings.zoomSensitivity = 0.2f;
                SaveSettings(m_settings);
                UpdateLabel();
                return;
            }
        }
    }

private:
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    const float m_width, m_height;
    uint8_t* m_state;
    uint8_t m_returnState;
    SettingsData m_settings;
    std::vector<std::string> m_buttons;
    std::vector<Button> m_buttonsObjects;
    std::unique_ptr<Label> m_sensitivityLabel;
    std::unique_ptr<Label> m_titleLabel;

    void CreateUI() {
        m_buttonsObjects.clear();

        m_buttonsObjects.emplace_back(
        m_renderer, m_font, m_buttons[0],
        m_width, m_height,
        (m_width/10*9), 0,
        m_width/10, m_height/8,
        btnRGB[0], btnRGB[1], btnRGB[2]
        );

        for (uint8_t i = 0; i < 2; ++i) {
            m_buttonsObjects.emplace_back(
                m_renderer, m_font, m_buttons[i+1],
                m_width, m_height,
                (m_width/3)+((m_width/6)*i), m_height/8*2,
                m_width/6-2, m_height/8,
                btnRGB[0], btnRGB[1], btnRGB[2]
            );
        }

        std::string text = "Sensitivity: " + std::to_string(m_settings.zoomSensitivity).substr(0,4);
        m_sensitivityLabel = std::make_unique<Label>(
            m_renderer, m_font, text,
            m_width/3*2, m_height/8*2,
            m_width/4, m_height/8,
            btnRGB[0], btnRGB[1], btnRGB[2]
        );
        m_titleLabel = std::make_unique<Label>(
            m_renderer, m_font, "Zoom Sensitivity",
            m_width/12, m_height/8*2,
            m_width/4-2, m_height/8,
            btnRGB[0], btnRGB[1], btnRGB[2]
        );
        m_sensitivityLabel->SetActive(true);
        m_titleLabel->SetActive(true);
    }

    void UpdateLabel() {
        if (m_sensitivityLabel) {
            std::string text = "Sensitivity: " + std::to_string(m_settings.zoomSensitivity).substr(0,4);
            m_sensitivityLabel->SetText(text);
        }
    }
};