#ifndef RETRONODE_CPU_PARTICLES_2D_H
#define RETRONODE_CPU_PARTICLES_2D_H

#include "node_2d.h"
#include "../../core/resource/gradient.h"
#include "../../core/resource/curve.h"
#include <SDL3/SDL.h>
#include <vector>
#include <string>

namespace RetroNode {

enum class ParticleEmissionShape {
    POINT = 0,
    SPHERE = 1,
    RECTANGLE = 2,
    POINTS = 3,
    DIRECTED_POINTS = 4
};

enum class ParticleDrawOrder {
    INDEX = 0,
    LIFETIME = 1
};

struct Particle {
    Vector2Fixed position;
    Vector2Fixed previous_position;
    Vector2Fixed velocity;
    Fixed16 rotation;
    Fixed16 rot_velocity;
    Vector2Fixed scale;
    SDL_Color color;
    SDL_Color base_color;
    Fixed16 life;
    Fixed16 lifetime;
    bool active = false;
};

class RN_API CPUParticles2D : public Node2D {
    RN_CLASS(CPUParticles2D, Node2D)

private:
    std::vector<Particle> particles;
    uint32_t prng_seed = 12345;

    Fixed16 time_accumulator;
    Fixed16 frame_remainder;

    float randf();
    float randf_range(float min_val, float max_val);

    void spawn_particle(Particle& p);

public:
    // Emission controls
    bool emitting = true;
    int amount = 16;
    Fixed16 lifetime = Fixed16::from_float(1.0f);
    bool one_shot = false;
    Fixed16 preprocess = Fixed16::from_float(0.0f);
    Fixed16 speed_scale = Fixed16::from_float(1.0f);
    Fixed16 explosiveness = Fixed16::from_float(0.0f);
    Fixed16 randomness = Fixed16::from_float(0.0f);
    bool local_coords = true;

    // Emission shape
    ParticleEmissionShape emission_shape = ParticleEmissionShape::POINT;
    Fixed16 emission_sphere_radius = Fixed16::from_float(1.0f);
    Vector2Fixed emission_rect_extents = Vector2Fixed::from_floats(1.0f, 1.0f);
    std::vector<Vector2Fixed> emission_points;
    std::vector<Vector2Fixed> emission_normals;

    // Dynamics & direction
    Vector2Fixed direction = Vector2Fixed::from_floats(1.0f, 0.0f);
    Fixed16 spread = Fixed16::from_float(45.0f); // degrees
    Vector2Fixed gravity = Vector2Fixed::from_floats(0.0f, 98.0f);

    Fixed16 initial_velocity_min = Fixed16::from_float(16.0f);
    Fixed16 initial_velocity_max = Fixed16::from_float(32.0f);

    Fixed16 angular_velocity_min = Fixed16::from_float(0.0f);
    Fixed16 angular_velocity_max = Fixed16::from_float(0.0f);

    Fixed16 linear_accel_min = Fixed16::from_float(0.0f);
    Fixed16 linear_accel_max = Fixed16::from_float(0.0f);

    Fixed16 radial_accel_min = Fixed16::from_float(0.0f);
    Fixed16 radial_accel_max = Fixed16::from_float(0.0f);

    Fixed16 tangential_accel_min = Fixed16::from_float(0.0f);
    Fixed16 tangential_accel_max = Fixed16::from_float(0.0f);

    Fixed16 damping_min = Fixed16::from_float(0.0f);
    Fixed16 damping_max = Fixed16::from_float(0.0f);

    Fixed16 scale_amount_min = Fixed16::from_float(4.0f);
    Fixed16 scale_amount_max = Fixed16::from_float(4.0f);

    // Visuals & resources
    SDL_Color color = {255, 255, 255, 255};
    Gradient* color_ramp = nullptr;
    Curve* scale_amount_curve = nullptr;

    uint32_t texture_id = 0;
    std::string texture_path;
    Vector2Fixed texture_size = Vector2Fixed::from_floats(4.0f, 4.0f);
    int z_index = 10;
    ParticleDrawOrder draw_order = ParticleDrawOrder::INDEX;

    CPUParticles2D();
    virtual ~CPUParticles2D() = default;

    void set_amount(int p_amount);
    void set_emitting(bool p_emitting);
    void set_texture_path(const std::string& path);
    void restart();

    virtual void _physics_process(Fixed16 delta) override;
    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CPU_PARTICLES_2D_H
