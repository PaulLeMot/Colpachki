#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <SDL3/SDL_main.h>
class Colpachki {
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    public:
    Colpachki(SDL_Renderer* renderer, TTF_Font* font) : m_renderer(renderer), m_font(font){}
    


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
    const float windowWidth = 680;
    const float windowHeight = 800;
    SDL_Window* window = SDL_CreateWindow("Colpachki", windowWidth, windowHeight, 0);
    if(!window){
        std::cerr<<"Window creation failed: "<<SDL_GetError()<<std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
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

    Colpachki ui(renderer, font);
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

        ui.Update(deltaTime);
            
        SDL_SetRenderDrawColor(renderer,0,0,0, 255);
        SDL_RenderClear(renderer);
        ui.Render(windowWidth, windowHeight);
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
