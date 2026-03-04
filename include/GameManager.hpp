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
#include <cstdint>
#include <fstream>

class GameManager{
    public:
        GameManager(const std::string& Name,
                    const std::string& SeedName)
                    : Name(Name), SeedName(SeedName){
            Seed = str2hash(SeedName);
        }

        std::string Name, SeedName;
        uint64_t Seed{};

        std::string NewGame(){
            std::string filepath = "../saves/" + Name + ".bin";
            std::ofstream file(filepath, std::ios::binary);
            
            file.write(reinterpret_cast<const char*>(&Seed), sizeof(Seed));
            
            uint32_t nameLen = static_cast<uint32_t>(Name.size());
            file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
            file.write(Name.data(), nameLen);
            
            uint32_t seedNameLen = static_cast<uint32_t>(SeedName.size());
            file.write(reinterpret_cast<const char*>(&seedNameLen), sizeof(seedNameLen));
            file.write(SeedName.data(), seedNameLen);
            return filepath;
        }
        void LoadGame(){

        }

    private:

        uint64_t str2hash(const std::string& s) {
            uint64_t h = 14695981039346656037ULL;
            for (unsigned char c : s) h = (h ^ c) * 1099511628211ULL;
            return h;
        }
};
