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
class Colpachki {
    public:
    Colpachki(SDL_Renderer* renderer, TTF_Font* font) : m_renderer(renderer), m_font(font){
    InitButtons();
        if(m_font){
            LoadButtonTextures();
        }
    }
    ~Colpachki() {
        FreeButtonTextures();
    }
    void Render(const float width, const float height){
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 100, 255);
    SDL_RenderClear(m_renderer);
    SDL_SetRenderDrawColor(m_renderer, 240, 240, 240, 21);
        for(int i=0;i<3;++i){
        SDL_FRect buttonRect = {width/3, height/8+(height/10+((height/8)*i)), width/3, height/10};
        SDL_RenderFillRect(m_renderer, &buttonRect);
            if (i < static_cast<int>(m_buttonTextures.size()) && m_buttonTextures[i]) {
                float tw, th;
                SDL_GetTextureSize(m_buttonTextures[i], &tw, &th);
                float tx = buttonRect.x + (buttonRect.w - tw) / 2.0f;
                float ty = buttonRect.y + (buttonRect.h - th) / 2.0f;
                SDL_FRect textRect = {tx, ty, tw, th};
                SDL_RenderTexture(m_renderer, m_buttonTextures[i], nullptr, &textRect);
            }
        }
    }
 
    /*void HandleEvent(const SDL_Event& event) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int mouseX = static_cast<int>(event.button.x);
            int mouseY = static_cast<int>(event.button.y);
            int buttonIndex = GetButtonAt(mouseX, mouseY);
            if (buttonIndex != -1) {
                OnButtonPressed(m_buttons[buttonIndex]);
            }
        }
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            HandleKeyPress(event.key);
        }
    }*/
    private:
    std::vector<std::string> m_buttons;
    std::vector<SDL_Texture*> m_buttonTextures;
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    void InitButtons() {
        m_buttons = {
            "Play", "Options", "Exit"
        };
    }
    void LoadButtonTextures() {
        m_buttonTextures.clear();
        m_buttonTextures.resize(m_buttons.size(), nullptr);
        SDL_Color color = {0, 0, 0, 255};
        for (size_t i = 0; i < m_buttons.size(); ++i) {
            SDL_Surface* surface = TTF_RenderText_Blended(m_font, m_buttons[i].c_str(), 0, color);
            if (surface) {
                m_buttonTextures[i] = SDL_CreateTextureFromSurface(m_renderer, surface);
                SDL_DestroySurface(surface);
            }
        }
    }
    void FreeButtonTextures() {
        for (SDL_Texture* tex : m_buttonTextures) {
            if (tex) SDL_DestroyTexture(tex);
        }
        m_buttonTextures.clear();
    }
    /*
    int GetButtonAt(int mouseX, int mouseY) {
        int totalButtons = static_cast<int>(m_buttons.size());
        int columns = 6;
        int rows = 5;

        const float windowWidth = 680;
        const float windowHeight = 800;
        const float buttonHeight = windowHeight / (rows + 1);
        const float buttonWidth = windowWidth / columns;

        for (int i = 0; i < totalButtons; ++i) {
            int row = i / columns;
            int col = i % columns;

            float x = col * buttonWidth + 10;
            float y = (row + 1) * buttonHeight + 10;
            float w = buttonWidth - 20;
            float h = buttonHeight - 20;

            if (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h) {
                return i;
            }
        }
        return -1;
    }

    void HandleKeyPress(const SDL_KeyboardEvent& keyEvent) {
        SDL_Keycode key = keyEvent.key;
        
        switch (key) {
            case SDLK_UP:
                if (m_cursorPos > 0) {
                    m_cursorPos--;
                    ResetCursorBlink();
                }
                break;
                
            case SDLK_DOWN:
                if (m_cursorPos < m_display.length()) {
                    m_cursorPos++;
                    ResetCursorBlink();
                }
                break;
                
            case SDLK_HOME:
                m_cursorPos = 0;
                ResetCursorBlink();
                break;
                
            case SDLK_END:
                m_cursorPos = m_display.length();
                ResetCursorBlink();
                break;
                
            case SDLK_BACKSPACE:
                if (m_cursorPos > 0 && m_cursorPos <= m_display.length()) {
                    m_display.erase(m_cursorPos - 1, 1);
                    m_cursorPos--;
                    ResetCursorBlink();
                    if (m_display.empty()) {
                        m_display = "0";
                        m_cursorPos = 1;
                    }
                }
                break;
                
            case SDLK_DELETE:
                if (m_cursorPos < m_display.length()) {
                    m_display.erase(m_cursorPos, 1);
                    ResetCursorBlink();
                    if (m_display.empty()) {
                        m_display = "0";
                        m_cursorPos = 1;
                    }
                }
                break;
                
            case SDLK_ESCAPE:
                m_displayHasFocus = false;
                break;
                
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                OnButtonPressed("=");
                break;
        }
    }*/
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
                //ui.HandleEvent(event);
        }
        ui.Render(width, height);
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
