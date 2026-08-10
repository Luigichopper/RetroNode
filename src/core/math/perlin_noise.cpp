#include "perlin_noise.h"
#include <numeric>
#include <random>
#include <algorithm>
#include <cmath>

namespace RetroNode {

PerlinNoise::PerlinNoise() {
    set_seed(2026);
}

PerlinNoise::PerlinNoise(unsigned int seed) {
    set_seed(seed);
}

void PerlinNoise::set_seed(unsigned int seed) {
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);
    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.end(), engine);
    p.insert(p.end(), p.begin(), p.end());
}

float PerlinNoise::fade(float t) noexcept {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoise::lerp(float t, float a, float b) noexcept {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y) noexcept {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float PerlinNoise::get_noise_2d(float x, float y) const noexcept {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);

    float u = fade(x);
    float v = fade(y);

    int A = p[X] + Y;
    int B = p[X + 1] + Y;

    return lerp(v, lerp(u, grad(p[A], x, y), grad(p[B], x - 1.0f, y)),
                   lerp(u, grad(p[A + 1], x, y - 1.0f), grad(p[B + 1], x - 1.0f, y - 1.0f)));
}

} // namespace RetroNode
