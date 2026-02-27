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

class Button{
    public:
        Button(SDL_Renderer* renderer, TTF_Font* font, const std::string Name, const float width, const float height, float x, float y, float w, float h, int r, int g, int b) : m_renderer(renderer), m_font(font), m_buttonName(Name), m_width(width), m_height(height), x(x),y(y),w(w),h(h),r(r),g(g),b(b){
            if(m_font){
                LoadButtonTexture();
            }
        }
        Button(const Button&) = delete;
        Button& operator=(const Button&) = delete;

        Button(Button&& other) noexcept
            : m_renderer(other.m_renderer),
              m_font(other.m_font),
              m_buttonName(std::move(other.m_buttonName)),
              m_width(other.m_width),
              m_height(other.m_height),
              x(other.x), y(other.y), w(other.w), h(other.h),
              r(other.r), g(other.g), b(other.b),
              m_buttonTexture(other.m_buttonTexture) {
            other.m_buttonTexture = nullptr;
        }

        Button& operator=(Button&& other) noexcept {
            if (this != &other) {
                FreeButtonTextures();
                m_renderer = other.m_renderer;
                m_font = other.m_font;
                m_buttonName = std::move(other.m_buttonName);
                x = other.x; y = other.y; w = other.w; h = other.h;
                r = other.r; g = other.g; b = other.b;
                m_buttonTexture = other.m_buttonTexture;
                other.m_buttonTexture = nullptr;
            }
            return *this;
        }
        ~Button(){
            FreeButtonTextures();
        }
        void RenderButton(){
 
            SDL_SetRenderDrawColor(m_renderer, r,g,b, 255);
                SDL_FRect buttonRect = {x,y,w,h};
                SDL_RenderFillRect(m_renderer, &buttonRect);
                if (m_buttonTexture) {
                    float tw, th;
                    SDL_GetTextureSize(m_buttonTexture, &tw, &th);
                    float tx = buttonRect.x + (buttonRect.w - tw) / 2.0f;
                    float ty = buttonRect.y + (buttonRect.h - th) / 2.0f;
                    SDL_FRect textRect = {tx, ty, tw, th};
                    SDL_RenderTexture(m_renderer, m_buttonTexture, nullptr, &textRect);
                }
        }
        void LoadButtonTexture() {
        SDL_Color colorText = {0, 0, 0, 255};
        SDL_Surface* surface = TTF_RenderText_Blended(m_font, m_buttonName.c_str(), 0, colorText);
            if (surface) {
                m_buttonTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
                SDL_DestroySurface(surface);
            }
        }
        bool GetButtonAt(int mouseX, int mouseY) {
            if (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h) {
                return 1;
            }
            return 0;
        }
    private:
        float x,y,w,h;
        std::string m_buttonName;
        SDL_Texture* m_buttonTexture;
        SDL_Renderer* m_renderer;
        TTF_Font* m_font;
        const float m_width, m_height;
        int r,g,b;
        
        void FreeButtonTextures() {
            if (m_buttonTexture) SDL_DestroyTexture(m_buttonTexture);
        }

};

class Input {
public:
    std::string m_inputText;
    Input(SDL_Renderer* renderer,
          TTF_Font* font,
          const std::string& placeholder,
          float width, float height,
          float x, float y, float w, float h,
          int r, int g, int b, int textlimit)
        : m_renderer(renderer), m_font(font), m_placeholder(placeholder),
          m_width(width), m_height(height), x(x), y(y), w(w), h(h),
          r(r), g(g), b(b), m_active(false), m_textTexture(nullptr) {
        if (m_font) {
            UpdateTexture(m_placeholder);
        }
    }

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    Input(Input&& other) noexcept
        : m_renderer(other.m_renderer),
          m_font(other.m_font),
          m_placeholder(std::move(other.m_placeholder)),
          m_width(other.m_width),
          m_height(other.m_height),
          m_inputText(std::move(other.m_inputText)),
          m_lastDisplayedText(std::move(other.m_lastDisplayedText)),
          m_active(other.m_active),
          x(other.x), y(other.y), w(other.w), h(other.h),
          r(other.r), g(other.g), b(other.b),
          m_textTexture(other.m_textTexture) {
        other.m_textTexture = nullptr;
    }

    Input& operator=(Input&& other) noexcept {
        if (this != &other) {
            FreeTexture();
            m_renderer = other.m_renderer;
            m_font = other.m_font;
            m_placeholder = std::move(other.m_placeholder);
            m_inputText = std::move(other.m_inputText);
            m_lastDisplayedText = std::move(other.m_lastDisplayedText);
            m_active = other.m_active;
            x = other.x; y = other.y; w = other.w; h = other.h;
            r = other.r; g = other.g; b = other.b;
            m_textTexture = other.m_textTexture;
            other.m_textTexture = nullptr;
        }
        return *this;
    }

    ~Input() {
        FreeTexture();
    }

    void RenderInput() {
        std::string displayText;
        if (m_active || !m_inputText.empty()) {
            displayText = m_inputText;
        } else {
            displayText = m_placeholder;
        }

        if (displayText != m_lastDisplayedText) {
            UpdateTexture(displayText);
            m_lastDisplayedText = displayText;
        }

        // Рисуем фон
        SDL_SetRenderDrawColor(m_renderer, r, g, b, 255);
        SDL_FRect inputRect = {x, y, w, h};
        SDL_RenderFillRect(m_renderer, &inputRect);

        // Рисуем текст
        if (m_textTexture) {
            float tw, th;
            SDL_GetTextureSize(m_textTexture, &tw, &th);
            float tx = x + 5;
            float ty = y + (h - th) / 2.0f;
            SDL_FRect textRect = {tx, ty, tw, th};
            SDL_RenderTexture(m_renderer, m_textTexture, nullptr, &textRect);
        }

        // Если поле активно, рисуем рамку (визуальный курсор)
        if (m_active) {
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255); // белая рамка
            SDL_FRect borderRect = {x - 1, y - 1, w + 2, h + 2};
            SDL_RenderRect(m_renderer, &borderRect);
        }
    }

    void HandleEvent(const SDL_Event& event) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int mouseX = static_cast<int>(event.button.x);
            int mouseY = static_cast<int>(event.button.y);
            bool inside = (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);
            SetActive(inside);
        }
        else if (event.type == SDL_EVENT_KEY_DOWN && m_active) {
            SDL_Keycode key = event.key.key;
            if (key == SDLK_BACKSPACE) {
                if (!m_inputText.empty()) {
                    m_inputText.pop_back();
                }
            }
            else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                SetActive(false);
            }
        }
        else if (event.type == SDL_EVENT_TEXT_INPUT && m_active &&m_inputText.size()<textlimit) {  //maybe fix tho not that important
            m_inputText += event.text.text;
        }
    }

    const std::string& GetText() const { return m_inputText; }

    void Clear() {
        m_inputText.clear();
    }

private:
    float x, y, w, h;
    std::string m_placeholder;
    std::string m_lastDisplayedText;
    bool m_active;
    SDL_Texture* m_textTexture;
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    const float m_width, m_height;
    int r, g, b;
    int textlimit;

    void UpdateTexture(const std::string& text) {
        FreeTexture();
        if (text.empty()) return;

        SDL_Color color = {255,255,255,255};
        SDL_Surface* surface = TTF_RenderText_Blended(m_font, text.c_str(), 0, color);
        if (surface) {
            m_textTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
            SDL_DestroySurface(surface);
        }
    }

    void FreeTexture() {
        if (m_textTexture) {
            SDL_DestroyTexture(m_textTexture);
            m_textTexture = nullptr;
        }
    }

    void SetActive(bool active) {
        if (m_active != active) {
            m_active = active;
            if (!m_active) {
                m_lastDisplayedText.clear();
            }
        }
    }
};

class Label {
public:
    Label(SDL_Renderer* renderer,
          TTF_Font* font,
          const std::string& text,
          float x, float y, float w, float h,
          int r, int g, int b)
        : m_renderer(renderer), m_font(font), m_text(text),
          m_x(x), m_y(y), m_w(w), m_h(h),
          m_r(r), m_g(g), m_b(b),
          m_textTexture(nullptr), m_active(false) {
        UpdateTexture(m_text);
    }

    Label(const Label&) = delete;
    Label& operator=(const Label&) = delete;

    Label(Label&& other) noexcept
        : m_renderer(other.m_renderer),
          m_font(other.m_font),
          m_text(std::move(other.m_text)),
          m_x(other.m_x), m_y(other.m_y), m_w(other.m_w), m_h(other.m_h),
          m_r(other.m_r), m_g(other.m_g), m_b(other.m_b),
          m_textTexture(other.m_textTexture),
          m_active(other.m_active) {
        other.m_textTexture = nullptr;
    }

    Label& operator=(Label&& other) noexcept {
        if (this != &other) {
            FreeTexture();
            m_renderer = other.m_renderer;
            m_font = other.m_font;
            m_text = std::move(other.m_text);
            m_x = other.m_x; m_y = other.m_y; m_w = other.m_w; m_h = other.m_h;
            m_r = other.m_r; m_g = other.m_g; m_b = other.m_b;
            m_textTexture = other.m_textTexture;
            m_active = other.m_active;
            other.m_textTexture = nullptr;
        }
        return *this;
    }

    ~Label() {
        FreeTexture();
    }

    void Render() {
        if (!m_active) return;
        SDL_SetRenderDrawColor(m_renderer, m_r, m_g, m_b, 255);
        SDL_FRect bgRect = {m_x, m_y, m_w, m_h};
        SDL_RenderFillRect(m_renderer, &bgRect);
        if (m_textTexture) {
            // Растягиваем текстуру на всю плашку
            SDL_RenderTexture(m_renderer, m_textTexture, nullptr, &bgRect);
        }
    }
    void SetText(const std::string& newText) {
        if (m_text != newText) {
            m_text = newText;
            UpdateTexture(m_text);
        }
    }
    const std::string& GetText() const {
        return m_text;
    }
    void SetActive(bool active) {
        m_active = active;
    }
    bool IsActive() const {
        return m_active;
    }

private:
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    std::string m_text;
    float m_x, m_y, m_w, m_h;
    int m_r, m_g, m_b;
    SDL_Texture* m_textTexture;
    bool m_active;

    void UpdateTexture(const std::string& text) {
        FreeTexture();
        if (text.empty()) return;

        SDL_Color color = {0, 0, 110, 255};
        SDL_Surface* surface = TTF_RenderText_Blended(m_font, text.c_str(), 0, color);
        if (surface) {
            m_textTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
            SDL_DestroySurface(surface);
        }
    }

    void FreeTexture() {
        if (m_textTexture) {
            SDL_DestroyTexture(m_textTexture);
            m_textTexture = nullptr;
        }
    }
};
