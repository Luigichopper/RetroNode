#!/usr/bin/env python3
import os
import sys
import json
from PIL import Image

def pack_textures(project_dir):
    assets_dir = os.path.join(project_dir, "assets")
    if not os.path.exists(assets_dir):
        print(f"[texture_packer] Assets dir does not exist: {assets_dir}")
        return

    images_to_pack = []
    
    # Scan for PNG files in assets
    for root, dirs, files in os.walk(assets_dir):
        for file in files:
            if file.endswith(".png") and file != "atlas.png":
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, project_dir).replace("\\", "/")
                images_to_pack.append((rel_path, full_path))

    if not images_to_pack:
        print("[texture_packer] No PNG images found to pack.")
        return

    loaded_images = []
    for rel_path, full_path in images_to_pack:
        try:
            im = Image.open(full_path).convert("RGBA")
            region_name = rel_path
            if region_name.startswith("assets/"):
                region_name = "res://" + region_name
            loaded_images.append((region_name, im))
        except Exception as e:
            print(f"[texture_packer] Failed to load {full_path}: {e}")

    # Sort images by height descending for shelf packing algorithm
    loaded_images.sort(key=lambda x: x[1].height, reverse=True)

    atlas_width = 256
    current_x = 0
    current_y = 0
    shelf_height = 0

    manifest = {"regions": {}}
    placed_images = []

    for name, img in loaded_images:
        w, h = img.size
        if current_x + w > atlas_width:
            current_x = 0
            current_y += shelf_height
            shelf_height = 0

        placed_images.append((img, current_x, current_y))
        manifest["regions"][name] = {
            "x": current_x,
            "y": current_y,
            "w": w,
            "h": h
        }

        current_x += w
        if h > shelf_height:
            shelf_height = h

    atlas_height = max(16, current_y + shelf_height)
    
    # Power of 2 height padding
    p2_height = 16
    while p2_height < atlas_height:
        p2_height *= 2
    atlas_height = p2_height

    atlas_img = Image.new("RGBA", (atlas_width, atlas_height), (0, 0, 0, 0))
    for img, x, y in placed_images:
        atlas_img.paste(img, (x, y))

    atlas_path = os.path.join(assets_dir, "atlas.png")
    manifest_path = os.path.join(assets_dir, "atlas.json")

    atlas_img.save(atlas_path, "PNG")
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=2)

    print(f"[texture_packer] Successfully generated atlas: {atlas_path} ({atlas_width}x{atlas_height}) and manifest: {manifest_path}")

if __name__ == "__main__":
    proj_dir = sys.argv[1] if len(sys.argv) > 1 else "./MyRPG"
    pack_textures(proj_dir)
