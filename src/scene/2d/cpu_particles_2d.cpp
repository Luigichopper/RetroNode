#include "cpu_particles_2d.h"
#include "../../servers/visual_server.h"
#include "../../servers/texture_server.h"
#ifdef RN_BUILD_EDITOR
#include "../../editor/editor_state.h"
#endif
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

void CPUParticles2D::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Node2D::get_property_list(out_list);
    out_list.push_back({ StringName("emitting"), VariantType::BOOL });
    out_list.push_back({ StringName("amount"), VariantType::INT });
    out_list.push_back({ StringName("lifetime"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("one_shot"), VariantType::BOOL });
    out_list.push_back({ StringName("speed_scale"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("explosiveness"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("direction"), VariantType::VECTOR2 });
    out_list.push_back({ StringName("spread"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("gravity"), VariantType::VECTOR2 });
    out_list.push_back({ StringName("initial_velocity_min"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("initial_velocity_max"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("angular_velocity_min"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("angular_velocity_max"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("scale_amount_min"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("scale_amount_max"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("color"), VariantType::COLOR });
    out_list.push_back({ StringName("texture"), VariantType::STRING, PropertyHint::FILE_PATH, "*.png" });
    out_list.push_back({ StringName("z_index"), VariantType::INT });
}

Variant CPUParticles2D::get(const StringName& p_name) const {
    static const StringName s_emitting("emitting");
    static const StringName s_amount("amount");
    static const StringName s_lifetime("lifetime");
    static const StringName s_one_shot("one_shot");
    static const StringName s_speed_scale("speed_scale");
    static const StringName s_explosiveness("explosiveness");
    static const StringName s_direction("direction");
    static const StringName s_spread("spread");
    static const StringName s_gravity("gravity");
    static const StringName s_initial_velocity_min("initial_velocity_min");
    static const StringName s_initial_velocity_max("initial_velocity_max");
    static const StringName s_angular_velocity_min("angular_velocity_min");
    static const StringName s_angular_velocity_max("angular_velocity_max");
    static const StringName s_scale_amount_min("scale_amount_min");
    static const StringName s_scale_amount_max("scale_amount_max");
    static const StringName s_color("color");
    static const StringName s_texture("texture");
    static const StringName s_texture_path("texture_path");
    static const StringName s_z_index("z_index");

    if (p_name == s_emitting) return Variant(emitting);
    if (p_name == s_amount) return Variant((int64_t)amount);
    if (p_name == s_lifetime) return Variant(lifetime);
    if (p_name == s_one_shot) return Variant(one_shot);
    if (p_name == s_speed_scale) return Variant(speed_scale);
    if (p_name == s_explosiveness) return Variant(explosiveness);
    if (p_name == s_direction) return Variant(direction);
    if (p_name == s_spread) return Variant(spread);
    if (p_name == s_gravity) return Variant(gravity);
    if (p_name == s_initial_velocity_min) return Variant(initial_velocity_min);
    if (p_name == s_initial_velocity_max) return Variant(initial_velocity_max);
    if (p_name == s_angular_velocity_min) return Variant(angular_velocity_min);
    if (p_name == s_angular_velocity_max) return Variant(angular_velocity_max);
    if (p_name == s_scale_amount_min) return Variant(scale_amount_min);
    if (p_name == s_scale_amount_max) return Variant(scale_amount_max);
    if (p_name == s_color) return Variant(color);
    if (p_name == s_texture || p_name == s_texture_path) return Variant(texture_path);
    if (p_name == s_z_index) return Variant((int64_t)z_index);
    return Node2D::get(p_name);
}

bool CPUParticles2D::set(const StringName& p_name, const Variant& p_value) {
    static const StringName s_emitting("emitting");
    static const StringName s_amount("amount");
    static const StringName s_lifetime("lifetime");
    static const StringName s_one_shot("one_shot");
    static const StringName s_speed_scale("speed_scale");
    static const StringName s_explosiveness("explosiveness");
    static const StringName s_direction("direction");
    static const StringName s_spread("spread");
    static const StringName s_gravity("gravity");
    static const StringName s_initial_velocity_min("initial_velocity_min");
    static const StringName s_initial_velocity_max("initial_velocity_max");
    static const StringName s_angular_velocity_min("angular_velocity_min");
    static const StringName s_angular_velocity_max("angular_velocity_max");
    static const StringName s_scale_amount_min("scale_amount_min");
    static const StringName s_scale_amount_max("scale_amount_max");
    static const StringName s_color("color");
    static const StringName s_texture("texture");
    static const StringName s_texture_path("texture_path");
    static const StringName s_z_index("z_index");

    if (p_name == s_emitting) { set_emitting(p_value.as_bool()); return true; }
    if (p_name == s_amount) { set_amount(static_cast<int>(p_value.as_int())); return true; }
    if (p_name == s_lifetime) { lifetime = p_value.as_fixed16(); return true; }
    if (p_name == s_one_shot) { one_shot = p_value.as_bool(); return true; }
    if (p_name == s_speed_scale) { speed_scale = p_value.as_fixed16(); return true; }
    if (p_name == s_explosiveness) { explosiveness = p_value.as_fixed16(); return true; }
    if (p_name == s_direction) { direction = p_value.as_vector2(); return true; }
    if (p_name == s_spread) { spread = p_value.as_fixed16(); return true; }
    if (p_name == s_gravity) { gravity = p_value.as_vector2(); return true; }
    if (p_name == s_initial_velocity_min) { initial_velocity_min = p_value.as_fixed16(); return true; }
    if (p_name == s_initial_velocity_max) { initial_velocity_max = p_value.as_fixed16(); return true; }
    if (p_name == s_angular_velocity_min) { angular_velocity_min = p_value.as_fixed16(); return true; }
    if (p_name == s_angular_velocity_max) { angular_velocity_max = p_value.as_fixed16(); return true; }
    if (p_name == s_scale_amount_min) { scale_amount_min = p_value.as_fixed16(); return true; }
    if (p_name == s_scale_amount_max) { scale_amount_max = p_value.as_fixed16(); return true; }
    if (p_name == s_color) { color = p_value.as_color(); return true; }
    if (p_name == s_texture || p_name == s_texture_path) { set_texture_path(p_value.as_string()); return true; }
    if (p_name == s_z_index) { z_index = static_cast<int>(p_value.as_int()); return true; }
    return Node2D::set(p_name, p_value);
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
    prng_seed = prng_seed * 1664525u + 1013904223u;
    return static_cast<float>(prng_seed & 0x00FFFFFF) / 16777215.0f;
}

float CPUParticles2D::randf_range(float min_val, float max_val) {
    return min_val + randf() * (max_val - min_val);
}

void CPUParticles2D::spawn_particle(Particle& p) {
    p.active = true;
    p.life = Fixed16::from_float(0.0f);
    float ltime = lifetime.to_float() * (1.0f - randf_range(0.0f, randomness.to_float()));
    p.lifetime = Fixed16::from_float(std::max(0.05f, ltime));

    Vector2Fixed spawn_pos = Vector2Fixed::zero();
    switch (emission_shape) {
        case ParticleEmissionShape::POINT:
            spawn_pos = Vector2Fixed::zero();
            break;
        case ParticleEmissionShape::SPHERE: {
            float angle = randf_range(0.0f, 2.0f * static_cast<float>(M_PI));
            float r = randf_range(0.0f, emission_sphere_radius.to_float());
            spawn_pos = Vector2Fixed::from_floats(std::cos(angle) * r, std::sin(angle) * r);
            break;
        }
        case ParticleEmissionShape::RECTANGLE: {
            float rx = randf_range(-emission_rect_extents.x.to_float(), emission_rect_extents.x.to_float());
            float ry = randf_range(-emission_rect_extents.y.to_float(), emission_rect_extents.y.to_float());
            spawn_pos = Vector2Fixed::from_floats(rx, ry);
            break;
        }
        default:
            spawn_pos = Vector2Fixed::zero();
            break;
    }

    p.position = spawn_pos;
    p.previous_position = spawn_pos;

    // Calculate initial velocity with spread angle
    float base_angle = std::atan2(direction.y.to_float(), direction.x.to_float());
    float half_spread_rad = (spread.to_float() * 0.5f) * (static_cast<float>(M_PI) / 180.0f);
    float p_angle = base_angle + randf_range(-half_spread_rad, half_spread_rad);
    float speed = randf_range(initial_velocity_min.to_float(), initial_velocity_max.to_float());

    p.velocity = Vector2Fixed::from_floats(std::cos(p_angle) * speed, std::sin(p_angle) * speed);

    p.rotation = Fixed16::from_float(0.0f);
    p.rot_velocity = Fixed16::from_float(randf_range(angular_velocity_min.to_float(), angular_velocity_max.to_float()));

    float init_scale = randf_range(scale_amount_min.to_float(), scale_amount_max.to_float());
    p.scale = Vector2Fixed::from_floats(init_scale, init_scale);

    p.base_color = color;
    p.color = color;
}

void CPUParticles2D::_physics_process(Fixed16 delta) {
    if (!emitting && std::all_of(particles.begin(), particles.end(), [](const Particle& p) { return !p.active; })) {
        return;
    }

    float dt_float = delta.to_float() * speed_scale.to_float();
    Fixed16 dt = Fixed16::from_float(dt_float);

    time_accumulator += dt;

    // Spawn new particles based on emission rate
    if (emitting) {
        float emission_rate = static_cast<float>(amount) / std::max(0.01f, lifetime.to_float());
        float spawn_interval = (emission_rate > 0.0f) ? (1.0f / emission_rate) : 9999.0f;

        while (time_accumulator.to_float() >= spawn_interval) {
            time_accumulator -= Fixed16::from_float(spawn_interval);

            // Find first inactive particle slot to spawn
            auto it = std::find_if(particles.begin(), particles.end(), [](const Particle& p) { return !p.active; });
            if (it != particles.end()) {
                spawn_particle(*it);
            } else if (!one_shot) {
                // Recycle oldest particle if max capacity reached
                spawn_particle(particles[0]);
            }
        }
    }

    // Update active particles
    for (auto& p : particles) {
        if (!p.active) continue;

        p.previous_position = p.position;
        p.life += dt;

        if (p.life >= p.lifetime) {
            p.active = false;
            continue;
        }

        float t = p.life.to_float() / std::max(0.01f, p.lifetime.to_float());

        // Apply Forces (Gravity, Accelerations, Damping)
        p.velocity += gravity * dt;

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

#ifdef RN_BUILD_EDITOR
    if (!EditorState::get()->get_is_play_mode()) {
        _physics_process(Fixed16::from_float(delta));
    }
#endif

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
