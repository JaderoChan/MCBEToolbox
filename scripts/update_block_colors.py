#!/usr/bin/env python3
"""Recalculate colors in block_colors.json from a user-selected texture directory."""

import json
from collections import Counter
from pathlib import Path

from PIL import Image


KMEANS_CLUSTERS = 3
KMEANS_ITERATIONS = 30
MINIMUM_ALPHA = 16


def dominant_color(image_path: Path) -> str:
    with Image.open(image_path) as image:
        pixels = image.convert("RGBA").getdata()

    weighted_colors = Counter(
        (red, green, blue)
        for red, green, blue, alpha in pixels
        if alpha >= MINIMUM_ALPHA
    )
    if not weighted_colors:
        return "#000000"

    points = list(weighted_colors)
    cluster_count = min(KMEANS_CLUSTERS, len(points))
    centers = [points[index * len(points) // cluster_count] for index in range(cluster_count)]

    for _ in range(KMEANS_ITERATIONS):
        clusters = [[] for _ in centers]
        for point, weight in weighted_colors.items():
            center_index = min(
                range(len(centers)),
                key=lambda index: sum(
                    (point[channel] - centers[index][channel]) ** 2 for channel in range(3)
                ),
            )
            clusters[center_index].append((point, weight))

        updated_centers = []
        for center, cluster in zip(centers, clusters):
            if not cluster:
                updated_centers.append(center)
                continue
            total_weight = sum(weight for _, weight in cluster)
            updated_centers.append(
                tuple(
                    sum(point[channel] * weight for point, weight in cluster) // total_weight
                    for channel in range(3)
                )
            )
        if updated_centers == centers:
            break
        centers = updated_centers

    largest_cluster = max(clusters, key=lambda cluster: sum(weight for _, weight in cluster))
    total_weight = sum(weight for _, weight in largest_cluster)
    color = tuple(
        sum(point[channel] * weight for point, weight in largest_cluster) // total_weight
        for channel in range(3)
    )
    return "#{:02X}{:02X}{:02X}".format(*color)


def update_surface_colors(
    textures: object,
    textures_directory: Path,
    missing_textures: set[str],
) -> object | None:
    if isinstance(textures, str):
        image_path = textures_directory / textures.replace("/", "\\")
        if not image_path.is_file():
            missing_textures.add(textures)
            return None
        return dominant_color(image_path)

    if isinstance(textures, dict):
        colors = {}
        for face, texture_path in textures.items():
            if not isinstance(texture_path, str):
                continue
            color = update_surface_colors(texture_path, textures_directory, missing_textures)
            if color is not None:
                colors[face] = color
        return colors

    return None


def main() -> None:
    textures_text = input("请输入 textures 目录路径: ").strip().strip('"')
    block_colors_text = input("请输入 block_colors.json 文件路径: ").strip().strip('"')
    textures_directory = Path(textures_text).expanduser()
    block_colors_path = Path(block_colors_text).expanduser()

    if not textures_directory.is_dir():
        print(f"错误: textures 目录不存在: {textures_directory}")
        return
    if not block_colors_path.is_file():
        print(f"错误: block_colors.json 文件不存在: {block_colors_path}")
        return

    try:
        block_colors = json.loads(block_colors_path.read_text(encoding="utf-8"))
        if not isinstance(block_colors, dict):
            raise ValueError("block_colors.json 的根节点必须为 object")

        missing_textures: set[str] = set()
        updated_count = 0
        for block_data in block_colors.values():
            if not isinstance(block_data, dict):
                continue
            for version_data in block_data.values():
                if not isinstance(version_data, dict) or "textures" not in version_data:
                    continue
                colors = update_surface_colors(
                    version_data["textures"], textures_directory, missing_textures
                )
                if colors is not None:
                    version_data["colors"] = colors
                    updated_count += 1

        block_colors_path.write_text(
            json.dumps(block_colors, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"更新失败: {error}")
        return

    print(f"更新完成: {block_colors_path}")
    print(f"已更新 {updated_count} 组材质颜色。")
    if missing_textures:
        print("以下材质文件不存在:")
        for texture_path in sorted(missing_textures):
            print(f"  {texture_path}")


if __name__ == "__main__":
    main()
