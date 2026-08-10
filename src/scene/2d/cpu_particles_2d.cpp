#include "cpu_particles_2d.h"
#include "../../servers/visual_server.h"
#include "../../servers/texture_server.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace RetroNode {

RN_REGISTER_CLASS(CPUParticles2D);

CPUParticles2D::CPUParticles2D() {
    set_amount(amount);
}

void CPUParticles2D::set_amount(int p_amount) {
    if (p_amount < 1) p_amount = 1;
    amount = p_amount;
    particles.resize(amount);
    for (auto& p : particles) {
        p.active = false;
    }
}

void CPUParticles2D::set_emitting(bool p_emitting) {
    if (emitting != p_emitting) {
        emitting = p_emitting;
        if (emitting && one_shot) {
            restart();
        }
    }
}

void CPUParticles2D::set_texture_path(const std::string& path) {
    texture_path = path;
    if (!path.empty()) {
        texture_id = TextureServer::get()->load_texture(path);
        texture_size = TextureServer::get()->get_texture_size(texture_id);
    }
}

void CPUParticles2D::restart() {
    time_accumulator = Fixed16::from_float(0.0f);
    for (auto& p : particles) {
        p.active = false;
    }
    if (emitting && explosiveness.to_float() >= 0.5f) {
        for (auto& p : particles) {
            spawn_particle(p);
        }
    }
}

float CPUParticles2D::randf() {
    // Simple fast linear congruential generator for deterministic PRNG
    prng_seed = prng_seed * 1664525u + 1013904223u;
    return static_cast<float>(prng_seed & 0x00FFFFFF) / 16777215.0f;
}

float CPUParticles2D::randf_range(float min_val, float max_val) {
    return min_val + (max_val - min_val) * randf();
}

void CPUParticles2D::spawn_particle(Particle& p) {
    p.active = true;
    p.life = Fixed16::from_float(0.0f);
    
    float rand_life_mod = 1.0f - randf() * randomness.to_float();
    float actual_lifetime = std::max(0.05f, lifetime.to_float() * rand_life_mod);
    p.lifetime = Fixed16::from_float(actual_lifetime);

    // Initial position based on emission shape
    Vector2Fixed spawn_pos = Vector2Fixed::zero();

    switch (emission_shape) {
        case ParticleEmissionShape::POINT:
            spawn_pos = Vector2Fixed::zero();
            break;
        case ParticleEmissionShape::SPHERE: {
            float r = std::sqrt(randf()) * emission_sphere_radius.to_float();
            float theta = randf() * 2.0f * static_cast<float>(M_PI);
            spawn_pos = Vector2Fixed::from_floats(r * std::cos(theta), r * std::sin(theta));
            break;
        }
        case ParticleEmissionShape::RECTANGLE: {
            float ex = emission_rect_extents.x.to_float();
            float ey = emission_rect_extents.y.to_float();
            spawn_pos = Vector2Fixed::from_floats(randf_range(-ex, ex), randf_range(-ey, ey));
            break;
        }
        case ParticleEmissionShape::POINTS:
        case ParticleEmissionShape::DIRECTED_POINTS: {
            if (!emission_points.empty()) {
                size_t idx = static_cast<size_t>(randf() * emission_points.size()) % emission_points.size();
                spawn_pos = emission_points[idx];
            }
            break;
        }
    }

    if (local_coords) {
        p.position = spawn_pos;
    } else {
        p.position = get_global_position() + spawn_pos;
    }
    p.previous_position = p.position;

    // Direction & Spread
    float base_angle = std::atan2(direction.y.to_float(), direction.x.to_float());
    float half_spread = (spread.to_float() * static_cast<float>(M_PI) / 180.0f) * 0.5f;
    float final_angle = base_angle + randf_range(-half_spread, half_spread);

    float init_speed = randf_range(initial_velocity_min.to_float(), initial_velocity_max.to_float());
    p.velocity = Vector2Fixed::from_floats(std::cos(final_angle) * init_speed, std::sin(final_angle) * init_speed);

    // Rotation & Rot velocity
    p.rotation = Fixed16::from_float(0.0f);
    p.rot_velocity = Fixed16::from_float(randf_range(angular_velocity_min.to_float(), angular_velocity_max.to_float()));

    // Base color & initial scale
    p.base_color = color;
    p.color = color;
    float base_scale = randf_range(scale_amount_min.to_float(), scale_amount_max.to_float());
    p.scale = Vector2Fixed::from_floats(base_scale, base_scale);
}

void CPUParticles2D::_physics_process(Fixed16 delta) {
    Node2D::_physics_process(delta);

    float dt_float = delta.to_float() * speed_scale.to_float();
    Fixed16 dt = Fixed16::from_float(dt_float);

    if (dt_float <= 0.00001f) return;

    // Handle Spawning logic
    if (emitting) {
        float emission_rate = lifetime.to_float() / static_cast<float>(amount);
        time_accumulator += dt;

        while (time_accumulator.to_float() >= emission_rate) {
            time_accumulator -= Fixed16::from_float(emission_rate);

            // Find an inactive particle slot to spawn
            bool spawned = false;
            for (auto& p : particles) {
                if (!p.active) {
                    spawn_particle(p);
                    spawned = true;
                    break;
                }
            }

            if (!spawned && one_shot) {
                // Check if all particles finished life
                bool any_active = false;
                for (const auto& p : particles) {
                    if (p.active) {
                        any_active = true;
                        break;
                    }
                }
                if (!any_active) {
                    emitting = false;
                }
            }
        }
    }

    // Particle dynamics simulation
    for (auto& p : particles) {
        if (!p.active) continue;

        p.previous_position = p.position;
        p.life += dt;

        float t = std::clamp(p.life.to_float() / p.lifetime.to_float(), 0.0f, 1.0f);

        if (t >= 1.0f) {
            p.active = false;
            continue;
        }

        // Apply Linear Acceleration
        float lin_accel = randf_range(linear_accel_min.to_float(), linear_accel_max.to_float());
        if (std::abs(lin_accel) > 0.001f) {
            float vel_len = std::sqrt(p.velocity.x.to_float() * p.velocity.x.to_float() + p.velocity.y.to_float() * p.velocity.y.to_float());
            if (vel_len > 0.001f) {
                p.velocity += (p.velocity / Fixed16::from_float(vel_len)) * Fixed16::from_float(lin_accel * dt_float);
            }
        }

        // Apply Gravity
        p.velocity += gravity * dt;

        // Apply Damping
        float damp = randf_range(damping_min.to_float(), damping_max.to_float());
        if (damp > 0.001f) {
            float damp_factor = std::max(0.0f, 1.0f - damp * dt_float);
            p.velocity *= Fixed16::from_float(damp_factor);
        }

        // Integrate Position & Rotation
        p.position += p.velocity * dt;
        p.rotation += p.rot_velocity * dt;

        // Evaluate Color Ramp
        if (color_ramp) {
            SDL_Color ramp_color = color_ramp->sample(t);
            p.color.r = static_cast<Uint8>((ramp_color.r * p.base_color.r) / 255);
            p.color.g = static_cast<Uint8>((ramp_color.g * p.base_color.g) / 255);
            p.color.b = static_cast<Uint8>((ramp_color.b * p.base_color.b) / 255);
            p.color.a = static_cast<Uint8>((ramp_color.a * p.base_color.a) / 255);
        } else {
            p.color = p.base_color;
        }

        // Evaluate Scale Curve
        float current_scale = randf_range(scale_amount_min.to_float(), scale_amount_max.to_float());
        if (scale_amount_curve) {
            current_scale *= scale_amount_curve->sample(t);
        }
        p.scale = Vector2Fixed::from_floats(current_scale, current_scale);
    }
}

void CPUParticles2D::_process(float delta) {
    (void)delta;

    Vector2Fixed node_global_pos = get_global_position();

    for (const auto& p : particles) {
        if (!p.active) continue;

        Vector2Fixed p_curr_pos = local_coords ? (node_global_pos + p.position) : p.position;
        Vector2Fixed p_prev_pos = local_coords ? (node_global_pos + p.previous_position) : p.previous_position;

        Vector2Fixed draw_size = (texture_id != 0) ? texture_size : p.scale;
        Rect2Fixed src_rect = Rect2Fixed::from_floats(0.0f, 0.0f, draw_size.x.to_float(), draw_size.y.to_float());

        VisualServer::get()->submit_draw_sprite(
            p_curr_pos,
            p_prev_pos,
            draw_size,
            src_rect,
            texture_id,
            z_index,
            p.color
        );
    }
}

} // namespace RetroNode
