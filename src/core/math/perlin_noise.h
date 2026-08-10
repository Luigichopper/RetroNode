#ifndef RETRONODE_PERLIN_NOISE_H
#define RETRONODE_PERLIN_NOISE_H

#include <vector>

namespace RetroNode {

class PerlinNoise {
private:
    std::vector<int> p;

    static float fade(float t) noexcept;
    static float lerp(float t, float a, float b) noexcept;
    static float grad(int hash, float x, float y) noexcept;

public:
    PerlinNoise();
    explicit PerlinNoise(unsigned int seed);

    void set_seed(unsigned int seed);
    float get_noise_2d(float x, float y) const noexcept;
};

} // namespace RetroNode

#endif // RETRONODE_PERLIN_NOISE_H
