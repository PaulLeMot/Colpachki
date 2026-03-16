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
                int zone = -1;

                if (biome == 0) {
                    float pos = (float)i / N;
                    float p = pos <= 0.5f ? pos : 1.0f - pos;

                    const float borders[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.47f};
                    const float blend_width = 0.02f;

                    int main_zone;
                    if (p <= borders[0]) main_zone = 0;
                    else if (p <= borders[1]) main_zone = 1;
                    else if (p <= borders[2]) main_zone = 2;
                    else if (p <= borders[3]) main_zone = 3;
                    else if (p <= borders[4]) main_zone = 4;
                    else main_zone = 5;

                    int selected_zone = main_zone;

                    for (int b = 0; b < 5; ++b) {
                        if (p >= borders[b] - blend_width && p <= borders[b] + blend_width) {
                            float t = (p - (borders[b] - blend_width)) / (2.0f * blend_width);
                            std::uniform_real_distribution<float> dis(0.0f, 1.0f);
                            float rnd = dis(m_rng);
                            selected_zone = (rnd < t) ? b + 1 : b;
                            break;
                        }
                    }
                    zone = selected_zone;
                }

                Map[i * N + j] = {static_cast<uint16_t>(i), static_cast<uint16_t>(j),
                  static_cast<uint8_t>(biome), static_cast<int8_t>(zone),
                  false, false, 0, {0,0,0,0}, {}, -1};
            }
        }
        perm = p;
    }

    void SmoothClimate(std::vector<Tile>& Map, int N, int iterations) {
        std::vector<Tile> newMap = Map;
        for (int iter = 0; iter < iterations; ++iter) {
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    if (tile.biome == 1) continue;

                    int votes[6] = {0};

                    for (int di = -1; di <= 1; ++di) {
                        for (int dj = -1; dj <= 1; ++dj) {
                            if (di == 0 && dj == 0) continue;
                            int ni = (i + di + N) % N;
                            int nj = (j + dj + N) % N;
                            const Tile& neighbor = Map[ni * N + nj];
                            if (neighbor.biome == 1) continue;
                            if (neighbor.zone >= 0 && neighbor.zone < 6)
                                votes[neighbor.zone]++;
                        }
                    }

                    int bestZone = tile.zone;
                    int bestCount = 0;
                    for (int z = 0; z < 6; ++z) {
                        if (votes[z] > bestCount) {
                            bestCount = votes[z];
                            bestZone = z;
                        }
                    }

                    if (bestCount > 2) {
                        newMap[i * N + j].zone = bestZone;
                    } else {
                        newMap[i * N + j].zone = tile.zone;
                    }
                }
            }
            std::swap(Map, newMap);
        }
    }

    void ApplyCoastalInfluence(std::vector<Tile>& Map, int N, int iterations) {
        std::vector<Tile> newMap = Map;
        for (int iter = 0; iter < iterations; ++iter) {
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    const Tile& tile = Map[i * N + j];
                    if (tile.biome == 1) {
                        newMap[i * N + j].zone = -1;
                        continue;
                    }

                    float pos = (float)i / N;
                    float p = pos <= 0.5f ? pos : 1.0f - pos;
                    int base_zone;
                    if (p <= 0.1f) base_zone = 0;
                    else if (p <= 0.2f) base_zone = 1;
                    else if (p <= 0.3f) base_zone = 2;
                    else if (p <= 0.4f) base_zone = 3;
                    else if (p <= 0.47f) base_zone = 4;
                    else base_zone = 5;

                    int newZone = tile.zone;

                    bool waterNear = false;
                    bool tropicalNear = false;
                    bool temperateNear = false;
                    bool desertNear = false;
                    for (int di = -1; di <= 1; ++di) {
                        for (int dj = -1; dj <= 1; ++dj) {
                            if (di == 0 && dj == 0) continue;
                            int ni = (i + di + N) % N;
                            int nj = (j + dj + N) % N;
                            const Tile& nb = Map[ni * N + nj];
                            if (nb.biome == 1)
                                waterNear = true;
                            else if (nb.zone == 5)
                                tropicalNear = true;
                            else if (nb.zone == 3)
                                temperateNear = true;
                            else if (nb.zone == 4)
                                desertNear = true;
                        }
                    }

                    if (tile.zone == 4) {
                        if (base_zone >= 4 && (waterNear || tropicalNear)) {
                            std::uniform_int_distribution<int> dis(0, 99);
                            if (dis(m_rng) < 95) {
                                newZone = 5;
                            }
                        }
                        if (newZone != 5) {
                            if (base_zone <= 3 && waterNear && !tropicalNear) {
                                newZone = 3;
                            }
                            else if (temperateNear && !tropicalNear && !waterNear) {
                                newZone = 3;
                            }
                        }
                    }

                    newMap[i * N + j].zone = newZone;
                }
            }
            std::swap(Map, newMap);
        }
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

    void AdjustMountainZones(std::vector<Tile>& Map, int N) {
        std::vector<Tile> newMap = Map;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                Tile& tile = newMap[i * N + j];
                if (!tile.mountain) continue;

                bool allMountain = true;

                int ni = (i - 1 + N) % N;
                int nj = j;
                if (!Map[ni * N + nj].mountain) allMountain = false;

                ni = (i + 1) % N;
                if (!Map[ni * N + nj].mountain) allMountain = false;

                ni = i;
                nj = (j - 1 + N) % N;
                if (!Map[ni * N + nj].mountain) allMountain = false;

                nj = (j + 1) % N;
                if (!Map[ni * N + nj].mountain) allMountain = false;

                if (allMountain && tile.zone > 0 && tile.zone != 5) {
                    tile.zone -= 1;
                }
                if (allMountain && tile.zone == 5) {
                    tile.zone -= 2;
                }
            }
        }
        Map = std::move(newMap);
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

    void GenerateRivers(std::vector<Tile>& Map, std::vector<float>& heightMap,
                        std::vector<std::pair<SDL_Point, SDL_Point>>& riverSegments,
                        int N) {
        const int maxSteps = 150;
        const int minLandNeighbors = 2;

        struct RiverGroup { int count; int minDist; int maxDist; };
        std::vector<RiverGroup> groups = {
            {150, 8, 20},
            {150, 2, 8}
        };
        std::vector<int> multipliers = {3000, 50};

        auto nodeHeight = [&](int nodeX, int nodeY) -> float {
            float sum = 0.0f;
            int count = 0;
            for (int dx = -1; dx <= 0; ++dx) {
                for (int dy = -1; dy <= 0; ++dy) {
                    int tx = nodeX + dx;
                    int ty = nodeY + dy;
                    int txw = (tx + N) % N;
                    int tyw = (ty + N) % N;
                    sum += heightMap[tyw * N + txw];
                    count++;
                }
            }
            return (count == 0) ? 0.0f : sum / count;
        };

        std::vector<int> distToWater(N * N, -1);
        std::queue<std::pair<int,int>> q;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (Map[i * N + j].biome == 1) {
                    distToWater[i * N + j] = 0;
                    q.push({i, j});
                }
            }
        }
        int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        while (!q.empty()) {
            auto [y, x] = q.front(); q.pop();
            int curDist = distToWater[y * N + x];
            for (auto& d : dirs) {
                int ny = (y + d[0] + N) % N;
                int nx = (x + d[1] + N) % N;
                if (distToWater[ny * N + nx] == -1) {
                    distToWater[ny * N + nx] = curDist + 1;
                    q.push({ny, nx});
                }
            }
        }

        riverSegments.clear();
        std::vector<std::vector<bool>> nodeUsed(N+1, std::vector<bool>(N+1, false));
        std::uniform_int_distribution<int> dist(0, N-1);

        const int bigRiverCount = 10;
        int bigRiversCreated = 0;

        std::vector<std::pair<int,int>> candidates;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (Map[i * N + j].biome != 0) continue;
                if (Map[i * N + j].zone == 4) continue;
                int d = distToWater[i * N + j];
                if (d < 20) continue;
                candidates.emplace_back(i, j);
            }
        }
        std::shuffle(candidates.begin(), candidates.end(), m_rng);

        for (const auto& [ty, tx] : candidates) {
            if (bigRiversCreated >= bigRiverCount) break;

            bool tooClose = false;
            for (int dy = -3; dy <= 3 && !tooClose; ++dy) {
                for (int dx = -3; dx <= 3; ++dx) {
                    int ny = (ty + dy + N+1) % (N+1);
                    int nx = (tx + dx + N+1) % (N+1);
                    if (nodeUsed[ny][nx]) {
                        tooClose = true;
                        break;
                    }
                }
            }
            if (tooClose) continue;

            int curX = tx, curY = ty;
            std::vector<std::pair<SDL_Point, SDL_Point>> segments;
            bool reachedWaterOrRiver = false;
            int totalSteps = 0;
            const int maxTotalSteps = 500;
            int mainDir = m_rng() % 4;

            auto step = [&](int dir, int& x, int& y) {
                int nx = x, ny = y;
                switch (dir) {
                    case 0: ny = (y == 0) ? N : y-1; break;
                    case 1: ny = (y == N) ? 0 : y+1; break;
                    case 2: nx = (x == 0) ? N : x-1; break;
                    case 3: nx = (x == N) ? 0 : x+1; break;
                }
                x = nx;
                y = ny;
            };

            while (!reachedWaterOrRiver && totalSteps < maxTotalSteps) {
                int straightLen = 3 + (m_rng() % 5);
                for (int s = 0; s < straightLen; ++s) {
                    if (totalSteps >= maxTotalSteps) break;
                    int prevX = curX, prevY = curY;
                    step(mainDir, curX, curY);
                    totalSteps++;

                    if (nodeUsed[curY][curX]) {
                        segments.emplace_back(SDL_Point{prevX, prevY}, SDL_Point{curX, curY});
                        reachedWaterOrRiver = true;
                        break;
                    }

                    segments.emplace_back(SDL_Point{prevX, prevY}, SDL_Point{curX, curY});

                    bool waterNow = false;
                    for (int dx = -1; dx <= 0 && !waterNow; ++dx) {
                        for (int dy = -1; dy <= 0; ++dy) {
                            int wx = (curX + dx + N) % N;
                            int wy = (curY + dy + N) % N;
                            if (Map[wy * N + wx].biome == 1) {
                                waterNow = true;
                                break;
                            }
                        }
                    }
                    if (waterNow) {
                        reachedWaterOrRiver = true;
                        break;
                    }
                }
                if (reachedWaterOrRiver) break;

                int sideDir;
                if (mainDir == 0 || mainDir == 1) {
                    sideDir = (m_rng() % 2) ? 2 : 3;
                } else {
                    sideDir = (m_rng() % 2) ? 0 : 1;
                }
                int sideLen = 1 + (m_rng() % 2);
                for (int s = 0; s < sideLen; ++s) {
                    if (totalSteps >= maxTotalSteps) break;
                    int prevX = curX, prevY = curY;
                    step(sideDir, curX, curY);
                    totalSteps++;

                    if (nodeUsed[curY][curX]) {
                        segments.emplace_back(SDL_Point{prevX, prevY}, SDL_Point{curX, curY});
                        reachedWaterOrRiver = true;
                        break;
                    }

                    segments.emplace_back(SDL_Point{prevX, prevY}, SDL_Point{curX, curY});

                    bool waterNow = false;
                    for (int dx = -1; dx <= 0 && !waterNow; ++dx) {
                        for (int dy = -1; dy <= 0; ++dy) {
                            int wx = (curX + dx + N) % N;
                            int wy = (curY + dy + N) % N;
                            if (Map[wy * N + wx].biome == 1) {
                                waterNow = true;
                                break;
                            }
                        }
                    }
                    if (waterNow) {
                        reachedWaterOrRiver = true;
                        break;
                    }
                }
                if (reachedWaterOrRiver) break;
            }

            if (reachedWaterOrRiver && !segments.empty()) {
                riverSegments.insert(riverSegments.end(), segments.begin(), segments.end());
                for (const auto& seg : segments) {
                    nodeUsed[seg.first.y][seg.first.x] = true;
                    nodeUsed[seg.second.y][seg.second.x] = true;
                }
                bigRiversCreated++;
            }
        }
        SDL_Log("Big rivers (snake) created: %d", bigRiversCreated);

        for (size_t g = 0; g < groups.size(); ++g) {
            const auto& group = groups[g];
            int riversCreatedInGroup = 0;
            int attempts = 0;
            const int maxAttempts = group.count * multipliers[g];

            while (riversCreatedInGroup < group.count && attempts < maxAttempts) {
                attempts++;
                int tx = dist(m_rng);
                int ty = dist(m_rng);

                if (Map[ty * N + tx].biome != 0) continue;
                if (Map[ty * N + tx].zone == 4) continue;

                int landNeighbors = 0;
                if (ty > 0 && Map[(ty - 1) * N + tx].biome == 0) landNeighbors++;
                if (ty < N - 1 && Map[(ty + 1) * N + tx].biome == 0) landNeighbors++;
                if (tx > 0 && Map[ty * N + (tx - 1)].biome == 0) landNeighbors++;
                if (tx < N - 1 && Map[ty * N + (tx + 1)].biome == 0) landNeighbors++;

                if (landNeighbors < minLandNeighbors) continue;

                int d = distToWater[ty * N + tx];
                if (d < group.minDist || d > group.maxDist) continue;

                bool tooClose = false;
                for (int dy = -2; dy <= 2 && !tooClose; ++dy) {
                    for (int dx = -2; dx <= 2; ++dx) {
                        int ny = ty + dy;
                        int nx = tx + dx;
                        if (ny < 0) ny += N+1;
                        if (ny > N) ny -= N+1;
                        if (nx < 0) nx += N+1;
                        if (nx > N) nx -= N+1;
                        if (nodeUsed[ny][nx]) {
                            tooClose = true;
                            break;
                        }
                    }
                }
                if (tooClose) continue;

                int curX = tx, curY = ty;
                std::vector<std::pair<SDL_Point, SDL_Point>> segments;
                bool reachedWaterOrRiver = false;
                int steps = 0;

                while (steps < maxSteps) {
                    std::vector<int> dirs = {0,1,2,3};

                    float curH = nodeHeight(curX, curY);
                    float bestH = curH;
                    int bestDir = -1;

                    for (int d : dirs) {
                        int nx = curX, ny = curY;
                        if (d == 0) ny = (curY == 0) ? N : curY-1;
                        else if (d == 1) ny = (curY == N) ? 0 : curY+1;
                        else if (d == 2) nx = (curX == 0) ? N : curX-1;
                        else if (d == 3) nx = (curX == N) ? 0 : curX+1;

                        float nh = nodeHeight(nx, ny);
                        if (nh > bestH) {
                            bestH = nh;
                            bestDir = d;
                        }
                    }

                    if (bestDir == -1) break;

                    int nx = curX, ny = curY;
                    if (bestDir == 0) ny = (curY == 0) ? N : curY-1;
                    else if (bestDir == 1) ny = (curY == N) ? 0 : curY+1;
                    else if (bestDir == 2) nx = (curX == 0) ? N : curX-1;
                    else if (bestDir == 3) nx = (curX == N) ? 0 : curX+1;

                    if (nodeUsed[ny][nx]) {
                        segments.emplace_back(SDL_Point{curX, curY}, SDL_Point{nx, ny});
                        reachedWaterOrRiver = true;
                        break;
                    }

                    segments.emplace_back(SDL_Point{curX, curY}, SDL_Point{nx, ny});

                    bool waterFound = false;
                    for (int dx = -1; dx <= 0 && !waterFound; ++dx) {
                        for (int dy = -1; dy <= 0; ++dy) {
                            int wx = (nx + dx + N) % N;
                            int wy = (ny + dy + N) % N;
                            if (Map[wy * N + wx].biome == 1) {
                                waterFound = true;
                                break;
                            }
                        }
                    }

                    if (waterFound) {
                        reachedWaterOrRiver = true;
                        break;
                    }

                    curX = nx;
                    curY = ny;
                    steps++;
                }

                if (reachedWaterOrRiver && !segments.empty()) {
                    riverSegments.insert(riverSegments.end(), segments.begin(), segments.end());
                    for (const auto& seg : segments) {
                        nodeUsed[seg.first.y][seg.first.x] = true;
                        nodeUsed[seg.second.y][seg.second.x] = true;
                    }
                    riversCreatedInGroup++;
                }
            }
            SDL_Log("River group [%zu]: minDist=%d maxDist=%d, created=%d/%d, attempts=%d (max=%d), efficiency=%.2f%%",
                    g, group.minDist, group.maxDist, riversCreatedInGroup, group.count, attempts, maxAttempts,
                    (attempts > 0 ? (100.0f * riversCreatedInGroup / attempts) : 0.0f));
        }
        SDL_Log("Total river segments: %zu", riverSegments.size());
    }

    void DesrtifyJungles(std::vector<Tile>& Map,
                         const std::vector<std::pair<SDL_Point, SDL_Point>>& riverSegments,
                         int N) {
        std::vector<std::vector<bool>> riverNode(N+1, std::vector<bool>(N+1, false));
        for (const auto& seg : riverSegments) {
            riverNode[seg.first.y][seg.first.x] = true;
            riverNode[seg.second.y][seg.second.x] = true;
        }

        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                Tile& tile = Map[i * N + j];
                if (tile.biome != 0 || (tile.zone != 5 && tile.zone != 3)) continue;

                int desertCount = 0;
                for (int di = -1; di <= 1; ++di) {
                    for (int dj = -1; dj <= 1; ++dj) {
                        if (di == 0 && dj == 0) continue;
                        int ni = (i + di + N) % N;
                        int nj = (j + dj + N) % N;
                        const Tile& nb = Map[ni * N + nj];
                        if (nb.biome == 0 && nb.zone == 4) desertCount++;
                    }
                }
                if (desertCount < 3) continue;

                bool waterNear = false;
                for (int di = -2; di <= 2 && !waterNear; ++di) {
                    for (int dj = -2; dj <= 2; ++dj) {
                        if (di == 0 && dj == 0) continue;
                        int ni = (i + di + N) % N;
                        int nj = (j + dj + N) % N;
                        if (Map[ni * N + nj].biome == 1) {
                            waterNear = true;
                            break;
                        }
                    }
                }
                if (waterNear) continue;

                bool riverNear = false;
                for (int dy = -1; dy <= 2 && !riverNear; ++dy) {
                    for (int dx = -1; dx <= 2; ++dx) {
                        int ny = (i + dy + (N+1)) % (N+1);
                        int nx = (j + dx + (N+1)) % (N+1);
                        if (riverNode[ny][nx]) {
                            riverNear = true;
                            break;
                        }
                    }
                }
                if (riverNear) continue;

                tile.zone = 4;
            }
        }
    }

    void JungleifyDeserts(std::vector<Tile>& Map, int N) {
        const int INF = 1e9;
        const int MAX_DIST = 1;
        std::vector<int> distToPlains(N * N, INF);
        std::vector<int> distToJungles(N * N, INF);
        std::queue<std::pair<int, int>> q;
        int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (Map[i * N + j].biome == 0 && Map[i * N + j].zone == 3) {
                    distToPlains[i * N + j] = 0;
                    q.push({i, j});
                }
            }
        }
        while (!q.empty()) {
            auto [y, x] = q.front(); q.pop();
            int cur = distToPlains[y * N + x];
            for (auto& d : dirs) {
                int ny = (y + d[0] + N) % N;
                int nx = (x + d[1] + N) % N;
                if (Map[ny * N + nx].biome == 0 && distToPlains[ny * N + nx] == INF) {
                    distToPlains[ny * N + nx] = cur + 1;
                    q.push({ny, nx});
                }
            }
        }

        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (Map[i * N + j].biome == 0 && Map[i * N + j].zone == 5) {
                    distToJungles[i * N + j] = 0;
                    q.push({i, j});
                }
            }
        }
        while (!q.empty()) {
            auto [y, x] = q.front(); q.pop();
            int cur = distToJungles[y * N + x];
            for (auto& d : dirs) {
                int ny = (y + d[0] + N) % N;
                int nx = (x + d[1] + N) % N;
                if (Map[ny * N + nx].biome == 0 && distToJungles[ny * N + nx] == INF) {
                    distToJungles[ny * N + nx] = cur + 1;
                    q.push({ny, nx});
                }
            }
        }

        std::vector<Tile> newMap = Map;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                const Tile& tile = Map[i * N + j];
                if (tile.biome != 0 || tile.zone != 4) continue;

                int dP = distToPlains[i * N + j];
                int dJ = distToJungles[i * N + j];
                if (dP > MAX_DIST && dJ > MAX_DIST) continue;
                if (dP < dJ) {
                    newMap[i * N + j].zone = 3;
                } else if (dJ < dP) {
                    newMap[i * N + j].zone = 5;
                }
            }
        }
        Map = std::move(newMap);
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