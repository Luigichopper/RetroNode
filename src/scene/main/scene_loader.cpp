#include "scene_loader.h"
#include "../../core/object/class_db.h"
#include "../../servers/texture_server.h"
#include "../2d/node_2d.h"
#include "../2d/sprite_2d.h"
#include "../2d/tile_map_layer.h"
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

  if (j.contains("properties")) {
    const auto &props = j["properties"];

    Node2D *node2d = dynamic_cast<Node2D *>(node);
    if (node2d && props.contains("position")) {
      float px = props["position"].value("x", 0.0f);
      float py = props["position"].value("y", 0.0f);
      node2d->set_position(Vector2Fixed::from_floats(px, py));
    }

    Control *ctrl = dynamic_cast<Control *>(node);
    if (ctrl) {
      if (props.contains("position")) {
        float px = props["position"].value("x", 0.0f);
        float py = props["position"].value("y", 0.0f);
        ctrl->set_position(Vector2Fixed::from_floats(px, py));
      }
      if (props.contains("size")) {
        float sx = props["size"].value("x", 40.0f);
        float sy = props["size"].value("y", 40.0f);
        ctrl->set_size(Vector2Fixed::from_floats(sx, sy));
      }
      if (props.contains("z_index")) {
        ctrl->z_index = props.value("z_index", 100);
      }
    }

    Sprite2D *sprite = dynamic_cast<Sprite2D *>(node);
    if (sprite) {
      if (props.contains("texture")) {
        sprite->set_texture_path(props["texture"]);
      }
      if (props.contains("z_index")) {
        sprite->z_index = props.value("z_index", 10);
      }
    }

    Label *label = dynamic_cast<Label *>(node);
    if (label) {
      if (props.contains("text")) {
        label->set_text(props["text"]);
      }
    }

    NinePatchRect *nine_patch = dynamic_cast<NinePatchRect *>(node);
    if (nine_patch) {
      if (props.contains("texture")) {
        nine_patch->set_texture_path(props["texture"]);
      }
      if (props.contains("patch_margin")) {
        nine_patch->patch_margin = props.value("patch_margin", 4);
      }
    }

    AudioStreamPlayer *audio_player = dynamic_cast<AudioStreamPlayer *>(node);
    if (audio_player) {
      if (props.contains("stream")) {
        audio_player->set_stream_path(props["stream"]);
      }
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

} // namespace RetroNode
