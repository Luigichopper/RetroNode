#include "scene_loader.h"
#include "../../core/object/class_db.h"
#include "../../servers/texture_server.h"
#include "../2d/node_2d.h"
#include "../2d/sprite_2d.h"
#include "../2d/tile_map_layer.h"
#include "../2d/cpu_particles_2d.h"
#include "../animation/animation_player.h"
#include "../audio/audio_stream_player.h"
#include "../gui/canvas_layer.h"
#include "../gui/control.h"
#include "../gui/debug_overlay.h"
#include "../gui/label.h"
#include "../gui/nine_patch_rect.h"
#include "../physics/collision_shape_2d.h"
#include "../physics/static_body_2d.h"
#include "../physics/area_2d.h"
#include "../physics/character_body_2d.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace RetroNode {

static Variant json_to_variant(const json &j) {
  if (j.is_boolean()) {
    return Variant(j.get<bool>());
  }
  if (j.is_number_integer()) {
    return Variant(j.get<int64_t>());
  }
  if (j.is_number_float()) {
    return Variant(Fixed16::from_float(j.get<float>()));
  }
  if (j.is_string()) {
    return Variant(j.get<std::string>());
  }
  if (j.is_object() && j.contains("x") && j.contains("y")) {
    float x = j.value("x", 0.0f);
    float y = j.value("y", 0.0f);
    return Variant(Vector2Fixed::from_floats(x, y));
  }
  if (j.is_array() && j.size() == 2) {
    float x = j[0].get<float>();
    float y = j[1].get<float>();
    return Variant(Vector2Fixed::from_floats(x, y));
  }
  if (j.is_array() && j.size() == 4) {
    return Variant(SDL_Color{
        static_cast<Uint8>(j[0].get<int>()),
        static_cast<Uint8>(j[1].get<int>()),
        static_cast<Uint8>(j[2].get<int>()),
        static_cast<Uint8>(j[3].get<int>())
    });
  }
  return Variant();
}

static Node *parse_node_internal(const json &j) {
  // Support sub-scene instantiations via "instance": "res://scenes/player.json"
  if (j.contains("instance")) {
    std::string instance_path = j["instance"];
    std::string resolved_path = instance_path;
    if (resolved_path.rfind("res://", 0) == 0) {
      resolved_path = resolved_path.substr(6);
    }

    std::string proj_dir = TextureServer::get()->get_project_dir();
    std::vector<std::string> candidates = {
        proj_dir + "/" + resolved_path, "./" + resolved_path,
        "./MyRPG/" + resolved_path, "../MyRPG/" + resolved_path, instance_path};

    // Check for compiled binary .rnb counterpart first
    std::string final_path = "";
    for (auto cand : candidates) {
      if (cand.length() > 5 && cand.substr(cand.length() - 5) == ".json") {
        std::string rnb_cand = cand.substr(0, cand.length() - 5) + ".rnb";
        if (fs::exists(rnb_cand)) {
          final_path = rnb_cand;
          break;
        }
      }
      if (fs::exists(cand)) {
        final_path = cand;
        break;
      }
    }

    if (!final_path.empty()) {
      Node *instanced_node = SceneLoader::load_scene_from_file(final_path);
      if (instanced_node) {
        instanced_node->set_scene_instance_path(instance_path);
        if (j.contains("name")) {
          instanced_node->set_name(j["name"]);
        }

        if (j.contains("properties")) {
          const auto &props = j["properties"];
          Node2D *node2d = dynamic_cast<Node2D *>(instanced_node);
          if (node2d && props.contains("position")) {
            float px = props["position"].value("x", 0.0f);
            float py = props["position"].value("y", 0.0f);
            node2d->set_position(Vector2Fixed::from_floats(px, py));
          }
        }
        std::cout << "[SceneLoader] Instanced sub-scene: " << instance_path
                  << " -> " << final_path << std::endl;
        return instanced_node;
      }
    }
  }

  std::string type_name = j.value("type", "Node");
  std::string script_name = j.value("script", "");

  std::string create_type = script_name.empty() ? type_name : script_name;

  Object *obj = ClassDB::get()->instantiate(StringName(create_type));
  if (!obj && !script_name.empty()) {
    std::cout << "[SceneLoader] ClassDB fallback to base type '" << type_name
              << "' for script '" << script_name << "'" << std::endl;
    obj = ClassDB::get()->instantiate(StringName(type_name));
  }

  Node *node = dynamic_cast<Node *>(obj);
  if (!node) {
    std::cerr << "[SceneLoader] Failed to create node of type '" << create_type
              << "', creating default Node" << std::endl;
    node = new Node();
  }

  if (j.contains("name")) {
    node->set_name(j["name"]);
  }

  if (j.contains("properties") && j["properties"].is_object()) {
    const auto &props = j["properties"];
    for (auto &[key, val] : props.items()) {
      node->set(StringName(key), json_to_variant(val));
    }

    Control *ctrl = dynamic_cast<Control *>(node);
    if (ctrl && props.contains("z_index")) {
      ctrl->z_index = props.value("z_index", 100);
    }

    NinePatchRect *nine_patch = dynamic_cast<NinePatchRect *>(node);
    if (nine_patch && props.contains("patch_margin")) {
      nine_patch->patch_margin = props.value("patch_margin", 4);
    }

    AudioStreamPlayer *audio_player = dynamic_cast<AudioStreamPlayer *>(node);
    if (audio_player) {
      if (props.contains("volume")) {
        audio_player->volume = props.value("volume", 1.0f);
      }
      if (props.contains("autoplay")) {
        audio_player->autoplay = props.value("autoplay", false);
      }
    }

    AnimationPlayer *anim_player = dynamic_cast<AnimationPlayer *>(node);
    if (anim_player && props.contains("animations")) {
      const auto &anims = props["animations"];
      if (anims.is_object()) {
        for (auto &[anim_name, anim_data] : anims.items()) {
          AnimationTrack track;
          track.name = anim_name;
          track.fps = anim_data.value("fps", 8.0f);
          track.loop = anim_data.value("loop", true);

          if (anim_data.contains("frames") && anim_data["frames"].is_array()) {
            for (const auto &f : anim_data["frames"]) {
              float fx = f.value("x", 0.0f);
              float fy = f.value("y", 0.0f);
              float fw = f.value("w", 16.0f);
              float fh = f.value("h", 16.0f);
              track.frames.push_back(Rect2Fixed::from_floats(fx, fy, fw, fh));
            }
          }
          anim_player->add_track(track);
        }
      }
      if (props.contains("autoplay")) {
        anim_player->play(props["autoplay"]);
      }
    }

    TileMapLayer *tilemap = dynamic_cast<TileMapLayer *>(node);
    if (tilemap) {
      if (props.contains("z_index")) {
        tilemap->z_index = props.value("z_index", -10);
      }
      if (props.contains("tileset")) {
        tilemap->set_tileset_path(props["tileset"]);
      }

      int cols = props.value("columns", 16);
      int rows = props.value("rows", 14);
      int tile_sz = props.value("tile_size", 16);

      std::vector<int> tiles;
      std::vector<bool> collisions;

      if (props.contains("tile_data") && props["tile_data"].is_array()) {
        tiles = props["tile_data"].get<std::vector<int>>();
      }
      if (props.contains("collision_data") &&
          props["collision_data"].is_array()) {
        collisions = props["collision_data"].get<std::vector<bool>>();
      }

      tilemap->setup_map(cols, rows, tile_sz, tiles, collisions);
    }

    CollisionShape2D *col_shape = dynamic_cast<CollisionShape2D *>(node);
    if (col_shape) {
      if (props.contains("shape_type")) {
        std::string stype = props["shape_type"];
        if (stype == "CIRCLE" || stype == "circle") {
          col_shape->set_shape_type(ShapeType::CIRCLE);
        } else {
          col_shape->set_shape_type(ShapeType::RECTANGLE);
        }
      }
      if (props.contains("rect")) {
        float rx = props["rect"].value("x", 0.0f);
        float ry = props["rect"].value("y", 0.0f);
        float rw = props["rect"].value("w", 16.0f);
        float rh = props["rect"].value("h", 16.0f);
        col_shape->set_rect(Rect2Fixed::from_floats(rx, ry, rw, rh));
      }
      if (props.contains("radius")) {
        float rad = props.value("radius", 8.0f);
        col_shape->set_radius(Fixed16::from_float(rad));
      }
    }

    CPUParticles2D *particles = dynamic_cast<CPUParticles2D *>(node);
    if (particles) {
      if (props.contains("amount")) {
        particles->set_amount(props.value("amount", 16));
      }
      if (props.contains("emitting")) {
        particles->emitting = props.value("emitting", true);
      }
      if (props.contains("lifetime")) {
        particles->lifetime = Fixed16::from_float(props.value("lifetime", 1.0f));
      }
      if (props.contains("one_shot")) {
        particles->one_shot = props.value("one_shot", false);
      }
      if (props.contains("speed_scale")) {
        particles->speed_scale = Fixed16::from_float(props.value("speed_scale", 1.0f));
      }
      if (props.contains("explosiveness")) {
        particles->explosiveness = Fixed16::from_float(props.value("explosiveness", 0.0f));
      }
      if (props.contains("randomness")) {
        particles->randomness = Fixed16::from_float(props.value("randomness", 0.0f));
      }
      if (props.contains("local_coords")) {
        particles->local_coords = props.value("local_coords", true);
      }
      if (props.contains("z_index")) {
        particles->z_index = props.value("z_index", 10);
      }
      if (props.contains("texture")) {
        particles->set_texture_path(props["texture"]);
      }
      if (props.contains("gravity")) {
        float gx = props["gravity"].value("x", 0.0f);
        float gy = props["gravity"].value("y", 98.0f);
        particles->gravity = Vector2Fixed::from_floats(gx, gy);
      }
      if (props.contains("direction")) {
        float dx = props["direction"].value("x", 1.0f);
        float dy = props["direction"].value("y", 0.0f);
        particles->direction = Vector2Fixed::from_floats(dx, dy);
      }
      if (props.contains("spread")) {
        particles->spread = Fixed16::from_float(props.value("spread", 45.0f));
      }
      if (props.contains("initial_velocity_min")) {
        particles->initial_velocity_min = Fixed16::from_float(props.value("initial_velocity_min", 16.0f));
      }
      if (props.contains("initial_velocity_max")) {
        particles->initial_velocity_max = Fixed16::from_float(props.value("initial_velocity_max", 32.0f));
      }
      if (props.contains("scale_amount_min")) {
        particles->scale_amount_min = Fixed16::from_float(props.value("scale_amount_min", 4.0f));
      }
      if (props.contains("scale_amount_max")) {
        particles->scale_amount_max = Fixed16::from_float(props.value("scale_amount_max", 4.0f));
      }
      if (props.contains("color")) {
        uint8_t cr = props["color"].value("r", 255);
        uint8_t cg = props["color"].value("g", 255);
        uint8_t cb = props["color"].value("b", 255);
        uint8_t ca = props["color"].value("a", 255);
        particles->color = {cr, cg, cb, ca};
      }
      if (props.contains("color_ramp") && props["color_ramp"].is_array()) {
        if (!particles->color_ramp) {
          particles->color_ramp = new Gradient();
          particles->color_ramp->clear();
        }
        for (const auto &pt : props["color_ramp"]) {
          float offset = pt.value("offset", 0.0f);
          uint8_t r = pt.value("r", 255);
          uint8_t g = pt.value("g", 255);
          uint8_t b = pt.value("b", 255);
          uint8_t a = pt.value("a", 255);
          particles->color_ramp->add_point(offset, {r, g, b, a});
        }
      }
      if (props.contains("scale_curve") && props["scale_curve"].is_array()) {
        if (!particles->scale_amount_curve) {
          particles->scale_amount_curve = new Curve();
          particles->scale_amount_curve->clear();
        }
        for (const auto &pt : props["scale_curve"]) {
          float pos = pt.value("position", 0.0f);
          float val = pt.value("val", 1.0f);
          particles->scale_amount_curve->add_point(pos, val);
        }
      }
    }
  }

  if (j.contains("children") && j["children"].is_array()) {
    for (const auto &child_j : j["children"]) {
      Node *child_node = parse_node_internal(child_j);
      if (child_node) {
        node->add_child(child_node);
      }
    }
  }

  return node;
}

static std::string read_string_binary(std::ifstream &in) {
  uint32_t len = 0;
  in.read(reinterpret_cast<char *>(&len), sizeof(len));
  if (len == 0)
    return "";
  if (len > 10485760) { // 10MB safety cap
    in.setstate(std::ios::failbit);
    return "";
  }
  std::string str(len, '\0');
  in.read(&str[0], len);
  if (!in)
    return "";
  return str;
}

static Node *parse_node_binary_internal(std::ifstream &in) {
  std::string name = read_string_binary(in);
  std::string type_name = read_string_binary(in);
  std::string script_name = read_string_binary(in);
  std::string instance_name = read_string_binary(in);
  std::string props_json_str = read_string_binary(in);

  uint32_t num_children = 0;
  in.read(reinterpret_cast<char *>(&num_children), sizeof(num_children));

  json j_node;
  j_node["name"] = name;
  j_node["type"] = type_name;
  if (!script_name.empty()) {
    j_node["script"] = script_name;
  }
  if (!instance_name.empty()) {
    j_node["instance"] = instance_name;
  }

  if (!props_json_str.empty()) {
    try {
      j_node["properties"] = json::parse(props_json_str);
    } catch (...) {
    }
  }

  Node *node = parse_node_internal(j_node);

  for (uint32_t i = 0; i < num_children; ++i) {
    Node *child = parse_node_binary_internal(in);
    if (child) {
      node->add_child(child);
    }
  }

  return node;
}

Node *SceneLoader::load_scene_from_binary(const std::string &filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "[SceneLoader] Failed to open binary scene file: " << filepath
              << std::endl;
    return nullptr;
  }

  char magic[4];
  file.read(magic, 4);
  if (std::string(magic, 4) != "RNB1") {
    std::cerr << "[SceneLoader] Invalid binary map signature in " << filepath
              << std::endl;
    return nullptr;
  }

  Node *root = parse_node_binary_internal(file);
  if (root) {
    std::cout << "[SceneLoader] Loaded binary scene file (.rnb): " << filepath
              << std::endl;
  }
  return root;
}

Node *SceneLoader::load_scene_from_file(const std::string &filepath) {
  if (filepath.length() > 4 &&
      filepath.substr(filepath.length() - 4) == ".rnb") {
    return load_scene_from_binary(filepath);
  }

  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "[SceneLoader] Failed to open scene file: " << filepath
              << std::endl;
    return nullptr;
  }

  try {
    json j;
    file >> j;
    return parse_node_internal(j);
  } catch (const std::exception &e) {
    std::cerr << "[SceneLoader] JSON Parse Error: " << e.what() << std::endl;
    return nullptr;
  }
}

static json serialize_node_internal(const Node* node) {
    if (!node) return json::object();
    json j = json::object();
    j["name"] = node->get_name();
    j["type"] = node->get_class_name().as_string();

    json props = json::object();
    std::vector<PropertyInfo> prop_list = node->get_property_list();
    for (const auto& pinfo : prop_list) {
        if (pinfo.name == StringName("name")) continue; // Handled separately
        Variant val = node->get(pinfo.name);
        if (val.is_nil()) continue;
        switch (pinfo.type) {
            case VariantType::BOOL:
                props[pinfo.name.as_string()] = val.as_bool();
                break;
            case VariantType::INT:
                props[pinfo.name.as_string()] = val.as_int();
                break;
            case VariantType::FLOAT16:
                props[pinfo.name.as_string()] = val.as_fixed16().to_float();
                break;
            case VariantType::VECTOR2: {
                Vector2Fixed v = val.as_vector2();
                props[pinfo.name.as_string()] = { {"x", v.x.to_float()}, {"y", v.y.to_float()} };
                break;
            }
            case VariantType::RECT2: {
                Rect2Fixed r = val.as_rect2();
                props[pinfo.name.as_string()] = {
                    {"x", r.position.x.to_float()},
                    {"y", r.position.y.to_float()},
                    {"w", r.size.x.to_float()},
                    {"h", r.size.y.to_float()}
                };
                break;
            }
            case VariantType::COLOR: {
                SDL_Color c = val.as_color();
                props[pinfo.name.as_string()] = { c.r, c.g, c.b, c.a };
                break;
            }
            case VariantType::STRING:
            case VariantType::STRING_NAME:
                props[pinfo.name.as_string()] = val.as_string();
                break;
            default:
                break;
        }
    }
    if (!props.empty()) {
        j["properties"] = props;
    }

    json children = json::array();
    const auto& node_children = node->get_children();
    for (size_t i = 0; i < node_children.size(); ++i) {
        Node* child = node_children[i];
        if (child) {
            children.push_back(serialize_node_internal(child));
        }
    }
    if (!children.empty()) {
        j["children"] = children;
    }

    return j;
}

Node* SceneLoader::load_scene_from_json_string(const std::string& json_str) {
    try {
        json j = json::parse(json_str);
        return parse_node_internal(j);
    } catch (const std::exception& e) {
        std::cerr << "[SceneLoader] JSON Parse Error from string: " << e.what() << std::endl;
        return nullptr;
    }
}

std::string SceneLoader::serialize_node_to_json_string(const Node* node) {
    if (!node) return "";
    json j = serialize_node_internal(node);
    return j.dump(4);
}

bool SceneLoader::save_scene_to_file(const Node* node, const std::string& filepath) {
    if (!node || filepath.empty()) return false;
    std::string content = serialize_node_to_json_string(node);
    if (content.empty()) return false;
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[SceneLoader] Failed to open file for writing: " << filepath << std::endl;
        return false;
    }
    file << content;
    file.close();
    std::cout << "[SceneLoader] Saved scene to: " << filepath << std::endl;
    return true;
}

} // namespace RetroNode
