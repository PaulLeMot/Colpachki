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
        std::string GetButtonAt(int mouseX, int mouseY) {
            if (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h) {
                return m_buttonName;
            }
            return "";
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
