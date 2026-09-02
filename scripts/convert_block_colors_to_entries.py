#!/usr/bin/env python3
"""Convert a block_colors JSON/JSONC file into a block_entries JSON file."""

import json
from pathlib import Path


def strip_json_comments(content: str) -> str:
    """Remove JSONC comments while preserving comment-like text in strings."""
    result: list[str] = []
    index = 0
    in_string = False
    escaped = False

    while index < len(content):
        character = content[index]

        if in_string:
            result.append(character)
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                in_string = False
            index += 1
            continue

        if character == '"':
            in_string = True
            result.append(character)
        elif character == "/" and index + 1 < len(content) and content[index + 1] == "/":
            index += 2
            while index < len(content) and content[index] not in "\r\n":
                index += 1
            continue
        elif character == "/" and index + 1 < len(content) and content[index + 1] == "*":
            end_index = content.find("*/", index + 2)
            if end_index == -1:
                raise ValueError("存在未闭合的块注释")
            index = end_index + 2
            continue
        else:
            result.append(character)

        index += 1

    return "".join(result)


def remove_trailing_commas(content: str) -> str:
    """Remove trailing commas from JSONC content while preserving strings."""
    result: list[str] = []
    index = 0
    in_string = False
    escaped = False

    while index < len(content):
        character = content[index]

        if in_string:
            result.append(character)
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                in_string = False
            index += 1
            continue

        if character == '"':
            in_string = True
            result.append(character)
        elif character == ",":
            next_index = index + 1
            while next_index < len(content) and content[next_index].isspace():
                next_index += 1
            if next_index >= len(content) or content[next_index] not in "}]":
                result.append(character)
        else:
            result.append(character)

        index += 1

    return "".join(result)


def create_entry(block_data: dict, block_id: str) -> dict:
    default_data = block_data.get("default")
    if not isinstance(default_data, dict):
        raise ValueError(f"方块 '{block_id}' 缺少 object 类型的 default 字段")

    for field in ("textures", "colors"):
        if field not in default_data:
            raise ValueError(f"方块 '{block_id}' 的 default 缺少 '{field}' 字段")

    entry = {
        "name": "",
        "added_version": "",
        "default": {
            "structure_nbt_id": "",
            "command_id": "",
            "textures": default_data["textures"],
            "colors": default_data["colors"],
            "attributes": {},
        },
    }

    variants = {
        version: variant_data
        for version, variant_data in block_data.items()
        if version != "default"
    }
    if variants:
        if not all(isinstance(variant_data, dict) for variant_data in variants.values()):
            raise ValueError(f"方块 '{block_id}' 的版本数据必须为 object")
        entry["variants"] = variants

    return entry


def update_entry(entry: dict, block_data: dict, block_id: str) -> None:
    """Update only textures and colors, preserving manually maintained entry fields."""
    default_data = block_data.get("default")
    if not isinstance(default_data, dict):
        raise ValueError(f"方块 '{block_id}' 缺少 object 类型的 default 字段")

    default_entry = entry.setdefault("default", {})
    if not isinstance(default_entry, dict):
        raise ValueError(f"目标条目 '{block_id}' 的 default 字段必须为 object")
    for field in ("textures", "colors"):
        if field not in default_data:
            raise ValueError(f"方块 '{block_id}' 的 default 缺少 '{field}' 字段")
        default_entry[field] = default_data[field]

    source_variants = {
        version: variant_data
        for version, variant_data in block_data.items()
        if version != "default"
    }
    if not source_variants:
        return
    if not all(isinstance(variant_data, dict) for variant_data in source_variants.values()):
        raise ValueError(f"方块 '{block_id}' 的版本数据必须为 object")

    target_variants = entry.setdefault("variants", {})
    if not isinstance(target_variants, dict):
        raise ValueError(f"目标条目 '{block_id}' 的 variants 字段必须为 object")
    for version, source_variant in source_variants.items():
        target_variant = target_variants.setdefault(version, {})
        if not isinstance(target_variant, dict):
            raise ValueError(f"目标条目 '{block_id}' 的版本 '{version}' 必须为 object")
        for field in ("textures", "colors"):
            if field in source_variant:
                target_variant[field] = source_variant[field]


def main() -> None:
    input_text = input("请输入 block_colors 文件路径: ").strip().strip('"')
    input_path = Path(input_text).expanduser()

    if not input_path.is_file():
        print(f"错误: 文件不存在或不是普通文件: {input_path}")
        return

    try:
        content = input_path.read_text(encoding="utf-8")
        colors_data = json.loads(remove_trailing_commas(strip_json_comments(content)))
        if not isinstance(colors_data, dict):
            raise ValueError("根节点必须为 object")

        output_path = input_path.with_name("block_entries.json")
        if output_path.is_file():
            entries_content = output_path.read_text(encoding="utf-8")
            entries_data = json.loads(remove_trailing_commas(strip_json_comments(entries_content)))
            if not isinstance(entries_data, dict):
                raise ValueError("已有 block_entries.json 的根节点必须为 object")
        else:
            entries_data = {}

        for block_id, block_data in colors_data.items():
            if not isinstance(block_data, dict):
                raise ValueError(f"方块 '{block_id}' 的数据必须为 object")
            existing_entry = entries_data.get(block_id)
            if existing_entry is None:
                entries_data[block_id] = create_entry(block_data, block_id)
            elif isinstance(existing_entry, dict):
                update_entry(existing_entry, block_data, block_id)
            else:
                raise ValueError(f"目标条目 '{block_id}' 必须为 object")

        output_path.write_text(
            json.dumps(entries_data, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"转换失败: {error}")
        return

    print(f"转换完成: {output_path}")


if __name__ == "__main__":
    main()
