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
#include <filesystem>
#include <vector>
#include <algorithm>

class GameManagerNew{
    public:
        GameManagerNew(const std::string& Name,
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

    private:

        uint64_t str2hash(const std::string& s) {
            uint64_t h = 14695981039346656037ULL;
            for (unsigned char c : s) h = (h ^ c) * 1099511628211ULL;
            return h;
        }
};

class GameLoader {
public:
    GameLoader(uint16_t& Current) : Current(Current) {}

    uint16_t CountSaves() {
        uint16_t count = 0;
        for (const auto& entry : std::filesystem::directory_iterator("../saves/")) {
            if (std::filesystem::is_regular_file(entry.status())) {
                std::string ext = entry.path().extension().string();
                for (char& c : ext) c = std::tolower(c);
                if (ext == ".bin") ++count;
            }
        }
        return count;
    }

    void GetSaves(std::vector<std::string>& SaveNames) {
        SaveNames.clear();
        namespace fs = std::filesystem;
        fs::path savesDir = "../saves/";
        if (!fs::exists(savesDir) || !fs::is_directory(savesDir)) return;

        std::vector<std::string> allBins;
        for (const auto& entry : fs::directory_iterator(savesDir)) {
            if (fs::is_regular_file(entry.status())) {
                std::string ext = entry.path().extension().string();
                for (char& c : ext) c = std::tolower(c);
                if (ext == ".bin") {
                    allBins.push_back(entry.path().filename().string());
                }
            }
        }
        std::sort(allBins.begin(), allBins.end());

        uint16_t page = Current;
        uint16_t start = page * 8;
        uint16_t total = static_cast<uint16_t>(allBins.size());
        if (start >= total) return;
        uint16_t end = std::min<uint16_t>(start + 8, total);
        for (uint16_t i = start; i < end; ++i) {
            SaveNames.push_back(allBins[i]);
        }
    }

private:
    uint16_t& Current;
};