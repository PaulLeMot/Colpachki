#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <SDL3/SDL_main.h>
#include <vector>
#include "../include/ColpEngine.hpp"
class MainMenu {
    public:
    MainMenu(SDL_Renderer* renderer, TTF_Font* font, const float width, const float height) : m_renderer(renderer), m_font(font), m_width(width), m_height(height){
        InitButtons();
        for (size_t i = 0; i < m_buttons.size(); ++i) {
            m_buttonsObjects.emplace_back(
                m_renderer, m_font, m_buttons[i],
                m_width, m_height,
                m_width / 3,
                m_height / 8 + (m_height / 10 + ((m_height / 8) * i)),
                m_width / 3, m_height / 10,
                240, 240, 240
            );
        }
    }
    void Render() {
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 100, 255);
        SDL_RenderClear(m_renderer);
        for (auto& button : m_buttonsObjects) {
            button.RenderButton();
        }
    }
    void HandleEvent(const SDL_Event& event) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int mouseX = static_cast<int>(event.button.x);
            int mouseY = static_cast<int>(event.button.y);
            if (m_buttonsObjects[0].GetButtonAt(mouseX, mouseY)) {
                OnButtonPressed("Play");
            }
            else if (m_buttonsObjects[1].GetButtonAt(mouseX, mouseY)) {
                OnButtonPressed("Options");
            }
            else if (m_buttonsObjects[2].GetButtonAt(mouseX, mouseY)) {
                SDL_Event quitEvent;
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            }
        }
    }
    private:
    std::vector<std::string> m_buttons;
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    const float m_width, m_height;
    std::vector<Button> m_buttonsObjects;
    void InitButtons() {
        m_buttons = {
            "Play", "Options", "Exit"
        };
    }

    /*void HandleKeyPress(const SDL_KeyboardEvent& keyEvent) {
        SDL_Keycode key = keyEvent.key;
        int Current = 0;
        
        switch (key) {
            case SDLK_UP:
                if (Current !=0) {
                    Current-=1;
                }else{Current=2;}
                break;
                
            case SDLK_DOWN:
                if (Current !=2) {
                    Current+=1;
                }else{Current=0;}
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                OnButtonPressed(m_buttons[Current]);
                break;
        }
    }*/
    void OnButtonPressed(std::string button){

    }
};
        
int main(int argc, char *argv[]){
    if(!SDL_Init(SDL_INIT_VIDEO)){
        std::cerr<<"SDL_Init Failed: "<<SDL_GetError()<<std::endl;
    return -1;
    }
    if(!TTF_Init()){
        std::cerr<<"TTF_Init Failed: "<<SDL_GetError()<<std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Colpachki", 0, 0, SDL_WINDOW_FULLSCREEN);
    if(!window){
        std::cerr<<"Window creation failed: "<<SDL_GetError()<<std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    int width, height;
    SDL_SetWindowFullscreen(window, 1);
    if (SDL_GetWindowSizeInPixels(window, &width, &height)) {
    } else {
        std::cerr<<"problem with window size: "<<SDL_GetError()<<std::endl;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if(!renderer){
        std::cerr<<"renderer creation failed: "<<SDL_GetError()<<std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/TTF/Hack-Regular.ttf", 36);
    if(!font){
        std::cerr<<"font failed: "<<SDL_GetError()<<std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    MainMenu ui(renderer, font, width, height);
    bool d = 0;
    Uint32 lastTime = SDL_GetTicks();
    while(!d){
        Uint32 crntTime = SDL_GetTicks();
        float deltaTime = (crntTime - lastTime)/1000.0f;
        lastTime = crntTime;
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    d = true;
                }
                ui.HandleEvent(event);
        }
        ui.Render();
        SDL_RenderPresent(renderer);
        SDL_Delay(32);
    }
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
