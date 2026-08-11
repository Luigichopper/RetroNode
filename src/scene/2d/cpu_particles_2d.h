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
    float initial_scale = 1.0f;
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
    virtual ~CPUParticles2D() override;

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    void set_amount(int p_amount);
    int get_amount() const { return amount; }

    void set_emitting(bool p_emitting);
    bool get_emitting() const { return emitting; }

    void set_lifetime(Fixed16 p_life) { lifetime = p_life; }
    Fixed16 get_lifetime() const { return lifetime; }

    void set_one_shot(bool p_os) { one_shot = p_os; }
    bool get_one_shot() const { return one_shot; }

    void set_speed_scale(Fixed16 p_sc) { speed_scale = p_sc; }
    Fixed16 get_speed_scale() const { return speed_scale; }

    void set_explosiveness(Fixed16 p_exp) { explosiveness = p_exp; }
    Fixed16 get_explosiveness() const { return explosiveness; }

    void set_direction(const Vector2Fixed& p_dir) { direction = p_dir; }
    Vector2Fixed get_direction() const { return direction; }

    void set_spread(Fixed16 p_spread) { spread = p_spread; }
    Fixed16 get_spread() const { return spread; }

    void set_gravity(const Vector2Fixed& p_grav) { gravity = p_grav; }
    Vector2Fixed get_gravity() const { return gravity; }

    void set_initial_velocity_min(Fixed16 p_val) { initial_velocity_min = p_val; }
    Fixed16 get_initial_velocity_min() const { return initial_velocity_min; }

    void set_initial_velocity_max(Fixed16 p_val) { initial_velocity_max = p_val; }
    Fixed16 get_initial_velocity_max() const { return initial_velocity_max; }

    void set_angular_velocity_min(Fixed16 p_val) { angular_velocity_min = p_val; }
    Fixed16 get_angular_velocity_min() const { return angular_velocity_min; }

    void set_angular_velocity_max(Fixed16 p_val) { angular_velocity_max = p_val; }
    Fixed16 get_angular_velocity_max() const { return angular_velocity_max; }

    void set_scale_amount_min(Fixed16 p_val) { scale_amount_min = p_val; }
    Fixed16 get_scale_amount_min() const { return scale_amount_min; }

    void set_scale_amount_max(Fixed16 p_val) { scale_amount_max = p_val; }
    Fixed16 get_scale_amount_max() const { return scale_amount_max; }

    void set_color(const SDL_Color& p_col) { color = p_col; }
    SDL_Color get_color() const { return color; }

    void set_texture_path(const std::string& path);
    const std::string& get_texture_path() const { return texture_path; }

    void set_z_index(int p_z) { z_index = p_z; }
    int get_z_index() const { return z_index; }

    void restart();

    virtual void _physics_process(Fixed16 delta) override;
    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CPU_PARTICLES_2D_H
