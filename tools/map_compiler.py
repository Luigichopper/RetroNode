#!/usr/bin/env python3
import os
import sys
import json
import struct

MAGIC_SIGNATURE = b"RNB1" # RetroNode Binary v1

def compile_node(node_dict):
    name_bytes = node_dict.get("name", "Node").encode("utf-8")
    type_bytes = node_dict.get("type", "Node").encode("utf-8")
    script_bytes = node_dict.get("script", "").encode("utf-8")
    instance_bytes = node_dict.get("instance", "").encode("utf-8")

    props_bytes = json.dumps(node_dict.get("properties", {})).encode("utf-8")
    children = node_dict.get("children", [])

    data = bytearray()
    data.extend(struct.pack("<I", len(name_bytes)))
    data.extend(name_bytes)
    data.extend(struct.pack("<I", len(type_bytes)))
    data.extend(type_bytes)
    data.extend(struct.pack("<I", len(script_bytes)))
    data.extend(script_bytes)
    data.extend(struct.pack("<I", len(instance_bytes)))
    data.extend(instance_bytes)

    data.extend(struct.pack("<I", len(props_bytes)))
    data.extend(props_bytes)

    data.extend(struct.pack("<I", len(children)))
    for child in children:
        data.extend(compile_node(child))

    return data

def compile_map(input_json, output_rnb):
    if not os.path.exists(input_json):
        print(f"[map_compiler] Input JSON not found: {input_json}")
        return

    with open(input_json, "r") as f:
        data = json.load(f)

    compiled_bytes = bytearray()
    compiled_bytes.extend(MAGIC_SIGNATURE)
    compiled_bytes.extend(compile_node(data))

    os.makedirs(os.path.dirname(output_rnb), exist_ok=True)
    with open(output_rnb, "wb") as f:
        f.write(compiled_bytes)

    print(f"[map_compiler] Compiled binary map: {input_json} -> {output_rnb} ({len(compiled_bytes)} bytes)")

if __name__ == "__main__":
    input_file = sys.argv[1] if len(sys.argv) > 1 else "./MyRPG/scenes/overworld.json"
    output_file = sys.argv[2] if len(sys.argv) > 2 else "./MyRPG/scenes/overworld.rnb"
    compile_map(input_file, output_file)
