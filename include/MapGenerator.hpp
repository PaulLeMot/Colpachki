#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <queue>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>

struct UnitInfo {
    uint8_t level;
    std::string id;
};
struct Tile {
    uint16_t x, y;
    uint8_t biome;
    int8_t zone;
    bool forest;
    bool mountain;
    uint8_t buildingLevel;
    SDL_Color capitalColor;
    std::vector<UnitInfo> units;
    int8_t owner;
};

class MapGenerator {
public:
    MapGenerator(std::mt19937& rng) : m_rng(rng) {}

    void CreateMap(std::vector<Tile>& Map, std::vector<float>& heightMap,
                std::vector<int>& perm, int N) {
        std::vector<int> permutation(256);
        for (int i = 0; i < 256; ++i) permutation[i] = i;
        std::shuffle(permutation.begin(), permutation.end(), m_rng);

        std::vector<int> p(512);
        for (int i = 0; i < 512; ++i) p[i] = permutation[i % 256];

        auto fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };
        auto lerp = [](float a, float b, float t) { return a + t * (b - a); };
        auto grad = [](int hash, float x, float y) {
            int h = hash & 15;
            float u = h < 8 ? x : y;
            float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
            return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
        };

        auto noise = [&](float x, float y) {
            int X = (int)std::floor(x) & 255;
            int Y = (int)std::floor(y) & 255;
            x -= std::floor(x);
            y -= std::floor(y);
            float u = fade(x);
            float v = fade(y);
            int aa = p[p[X] + Y];
            int ab = p[p[X] + Y + 1];
            int ba = p[p[X + 1] + Y];
            int bb = p[p[X + 1] + Y + 1];
            float g1 = grad(aa, x, y);
            float g2 = grad(ba, x - 1, y);
            float g3 = grad(ab, x, y - 1);
            float g4 = grad(bb, x - 1, y - 1);
            float x1 = lerp(g1, g2, u);
            float x2 = lerp(g3, g4, u);
            return lerp(x1, x2, v);
        };

        const int octaves = 4;
        const float persistence = 0.5f;
        const float lacunarity = 2.7f;
        const float scale = 0.02f / (N / 256.0f);

        const int gridSize = 2 * N;
        std::vector<float> grid(gridSize * gridSize);

        #pragma omp parallel for
        for (int gi = 0; gi < gridSize; ++gi) {
            for (int gj = 0; gj < gridSize; ++gj) {
                int ix = gi - N;
                int iy = gj - N;
                float x = ix * scale;
                float y = iy * scale;

                float amp = 1.0f;
                float freq = 1.0f;
                float value = 0.0f;
                float maxAmp = 0.0f;
                for (int o = 0; o < octaves; ++o) {
                    value += amp * noise(x * freq, y * freq);
                    maxAmp += amp;
                    amp *= persistence;
                    freq *= lacunarity;
                }
                grid[gi * gridSize + gj] = value / maxAmp;
            }
        }

        std::vector<float> rawValues(N * N);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                float wx = (float)i / N;
                float wy = (float)j / N;

                float v00 = grid[(i + N) * gridSize + (j + N)];
                float v10 = grid[i * gridSize + (j + N)];
                float v01 = grid[(i + N) * gridSize + j];
                float v11 = grid[i * gridSize + j];

                float raw = (1.0f - wx) * (1.0f - wy) * v00
                        + wx * (1.0f - wy) * v10
                        + (1.0f - wx) * wy * v01
                        + wx * wy * v11;

                rawValues[i * N + j] = raw;
            }
        }
        float minVal = rawValues[0], maxVal = rawValues[0];
        for (float v : rawValues) {
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }
        for (float& v : rawValues) {
            v = (v - minVal) / (maxVal - minVal);
        }
        heightMap = rawValues;

        std::vector<float> sorted = rawValues;
        std::sort(sorted.begin(), sorted.end());
        size_t thresholdIndex = static_cast<size_t>(sorted.size() * 0.33f);
        float threshold = sorted[thresholdIndex];

        Map.resize(N * N);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                float raw = rawValues[i * N + j];
                int biome = (raw > threshold) ? 1 : 0;
                int zone = (biome == 0) ? 3 : -1; // все сухие тайлы получают один биом (зона 3)

                Map[i * N + j] = {static_cast<uint16_t>(i), static_cast<uint16_t>(j),
                                static_cast<uint8_t>(biome), static_cast<int8_t>(zone),
                                false, false, 0, {0,0,0,0}, {}, -1};
            }
        }
        perm = p;
    }
    void GenerateForest(std::vector<Tile>& Map, const std::vector<int>& perm, int N) {
        if (perm.empty()) return;

        auto fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };
        auto lerp = [](float a, float b, float t) { return a + t * (b - a); };
        auto grad = [](int hash, float x, float y) {
            int h = hash & 15;
            float u = h < 8 ? x : y;
            float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
            return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
        };

        auto noise = [&](float x, float y) {
            int X = (int)std::floor(x) & 255;
            int Y = (int)std::floor(y) & 255;
            x -= std::floor(x);
            y -= std::floor(y);
            float u = fade(x);
            float v = fade(y);
            int aa = perm[perm[X] + Y];
            int ab = perm[perm[X] + Y + 1];
            int ba = perm[perm[X + 1] + Y];
            int bb = perm[perm[X + 1] + Y + 1];
            float g1 = grad(aa, x, y);
            float g2 = grad(ba, x - 1, y);
            float g3 = grad(ab, x, y - 1);
            float g4 = grad(bb, x - 1, y - 1);
            float x1 = lerp(g1, g2, u);
            float x2 = lerp(g3, g4, u);
            return lerp(x1, x2, v);
        };

        const float baseScale = 0.2f;
        const float offset = 1000.0f;
        const int octaves = 3;
        const float persistence = 0.5f;
        const float lacunarity = 2.0f;

        std::vector<float> forestNoise(N * N);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                float value = 0.0f;
                float amp = 1.0f;
                float freq = 1.0f;
                float maxAmp = 0.0f;
                for (int o = 0; o < octaves; ++o) {
                    float x = (i + offset) * baseScale * freq;
                    float y = (j + offset) * baseScale * freq;
                    value += amp * noise(x, y);
                    maxAmp += amp;
                    amp *= persistence;
                    freq *= lacunarity;
                }
                forestNoise[i * N + j] = (value / maxAmp + 1.0f) / 2.0f;
            }
        }

        for (auto& tile : Map) {
            tile.forest = false;
        }

        std::vector<int> group1 = {0,1,2,3};
        int group2 = 4;
        int group3 = 5;

        auto processGroup = [&](const std::vector<int>& zones, float targetPercent) {
            std::vector<float> values;
            std::vector<int> indices;
            for (int idx = 0; idx < N * N; ++idx) {
                const Tile& tile = Map[idx];
                if (tile.biome == 0 && !tile.mountain) {
                    for (int z : zones) {
                        if (tile.zone == z) {
                            values.push_back(forestNoise[idx]);
                            indices.push_back(idx);
                            break;
                        }
                    }
                }
            }
            if (values.empty()) return;

            std::sort(values.begin(), values.end());
            size_t thresholdIndex = static_cast<size_t>(values.size() * (1.0f - targetPercent));
            float threshold = values[thresholdIndex];

            for (size_t k = 0; k < indices.size(); ++k) {
                int idx = indices[k];
                if (forestNoise[idx] > threshold) {
                    Map[idx].forest = true;
                }
            }
        };

        processGroup(group1, 0.4f);
        processGroup({group2}, 0.1f);
        processGroup({group3}, 0.7f);
    }

    void GenerateMountains(std::vector<Tile>& Map, int N) {

        const int numPoints = 160;
        std::uniform_real_distribution<float> dist(0.0f, N);

        struct Point { float x, y; };
        std::vector<Point> points;
        for (int k = 0; k < numPoints; ++k) {
            points.push_back({dist(m_rng), dist(m_rng)});
        }

        const float thresholdRatio = 0.96f;

        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                Tile& tile = Map[i * N + j];
                if (tile.biome != 0) continue;

                float px = i + 0.5f;
                float py = j + 0.5f;

                float best1 = std::numeric_limits<float>::max();
                float best2 = std::numeric_limits<float>::max();

                for (const auto& pt : points) {
                    float dx = px - pt.x;
                    float dy = py - pt.y;
                    float dist2 = dx*dx + dy*dy;
                    if (dist2 < best1) {
                        best2 = best1;
                        best1 = dist2;
                    } else if (dist2 < best2) {
                        best2 = dist2;
                    }
                }

                if (best2 > 0 && (best1 / best2) > thresholdRatio) {
                    tile.mountain = true;
                    if (tile.forest) tile.forest = false;
                } else {
                    tile.mountain = false;
                }
            }
        }
    }

void GenerateCapitals(std::vector<Tile>& Map, int N) {
    const int targetCount = 100;
    const float minDist = 10.0f;
    std::vector<std::pair<int,int>> positions;
    std::uniform_int_distribution<int> dist(0, N-1);

    auto distance = [&](int x1, int y1, int x2, int y2) {
        int dx = std::abs(x1 - x2);
        int dy = std::abs(y1 - y2);
        dx = std::min(dx, N - dx);
        dy = std::min(dy, N - dy);
        return std::sqrt(dx*dx + dy*dy);
    };

    int attempts = 0;
    const int maxAttempts = 10000;
    while (positions.size() < targetCount && attempts < maxAttempts) {
        int x = dist(m_rng);
        int y = dist(m_rng);
        int idx = y * N + x;
        if (Map[idx].biome != 0) {
            attempts++;
            continue;
        }

        bool ok = true;
        for (auto& pos : positions) {
            if (distance(x, y, pos.first, pos.second) < minDist) {
                ok = false;
                break;
            }
        }

        if (ok) {
            positions.emplace_back(x, y);
            m_capitals.emplace_back(x, y);
            int colorIndex = positions.size() - 1;

            float hue = (float)colorIndex / (float)targetCount;
            float r=0, g=0, b=0;
            float h = hue * 6.0f;
            int sector = (int)h;
            float f = h - sector;
            float p = 0.0f;
            float q = 1.0f - f;
            float t = f;

            switch (sector % 6) {
                case 0: r = 1.0f; g = t; b = p; break;
                case 1: r = q; g = 1.0f; b = p; break;
                case 2: r = p; g = 1.0f; b = t; break;
                case 3: r = p; g = q; b = 1.0f; break;
                case 4: r = t; g = p; b = 1.0f; break;
                case 5: r = 1.0f; g = p; b = q; break;
            }

            SDL_Color color = {
                static_cast<Uint8>(r * 255),
                static_cast<Uint8>(g * 255),
                static_cast<Uint8>(b * 255),
                255
            };
            Map[idx].capitalColor = color;
            Map[idx].buildingLevel = 1;
        }
        attempts++;
    }
}
    void RemoveIsolatedMountains(std::vector<Tile>& Map, int N) {
        std::vector<Tile> newMap = Map;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (!Map[i * N + j].mountain) continue;

                int mountainNeighbors = 0;
                for (int di = -1; di <= 1; ++di) {
                    for (int dj = -1; dj <= 1; ++dj) {
                        if (di == 0 && dj == 0) continue;
                        int ni = (i + di + N) % N;
                        int nj = (j + dj + N) % N;
                        if (Map[ni * N + nj].mountain) ++mountainNeighbors;
                    }
                }

                if (mountainNeighbors >= 8) {
                    newMap[i * N + j].mountain = false;
                }
            }
        }
        Map = std::move(newMap);
    }
    const std::vector<std::pair<int,int>>& GetCapitals() const { return m_capitals; }
private:
    std::mt19937& m_rng;
    std::vector<std::pair<int,int>> m_capitals;
};