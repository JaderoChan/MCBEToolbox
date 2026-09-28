#!/usr/bin/env python3
"""清理 macOS App Bundle 内所有 Mach-O 文件中构建期残留的绝对路径 RPATH
（如 Homebrew 的 /opt/homebrew/...），统一改写为 bundle 内部相对路径。

背景：macdeployqt 只会拷贝依赖库文件，不会清理这些库自身原本携带的 RPATH（例如
Homebrew 编译 OpenCV 时写入的 /opt/homebrew/Cellar/opencv/.../lib）。如果不清理，
`@rpath/xxx.dylib` 可能被解析到系统里的原始文件而不是 bundle 内的副本，导致同一个库
（如 libomp）从两个不同路径被加载，进而触发 "OMP: Error #15" 之类的重复初始化问题。
"""
import os
import re
import subprocess
import sys


def is_macho(path: str) -> bool:
    out = subprocess.run(["file", "-b", path], capture_output=True, text=True).stdout
    return "Mach-O" in out


def get_rpaths(path: str) -> list[str]:
    out = subprocess.run(["otool", "-l", path], capture_output=True, text=True, check=True).stdout
    lines = out.splitlines()
    rpaths = []
    for i, line in enumerate(lines):
        if line.strip() == "cmd LC_RPATH":
            m = re.search(r"path (.+) \(offset", lines[i + 2])
            if m and m.group(1) not in rpaths:
                rpaths.append(m.group(1))
    return rpaths


def desired_rpath(bundle_dir: str, path: str) -> str:
    macos_dir = os.path.join(bundle_dir, "Contents", "MacOS")
    if os.path.dirname(path) == macos_dir:
        return "@executable_path/../Frameworks"
    frameworks_dir = os.path.join(bundle_dir, "Contents", "Frameworks")
    rel = os.path.relpath(frameworks_dir, os.path.dirname(path))
    return "@loader_path" if rel == "." else f"@loader_path/{rel}"


def main() -> None:
    bundle_dir = os.path.abspath(sys.argv[1])

    for root, _dirs, files in os.walk(bundle_dir):
        for name in files:
            path = os.path.join(root, name)
            if os.path.islink(path) or not is_macho(path):
                continue

            target = desired_rpath(bundle_dir, path)
            current = get_rpaths(path)

            for rp in current:
                if rp != target:
                    subprocess.run(["install_name_tool", "-delete_rpath", rp, path], check=False)

            if target not in current:
                subprocess.run(["install_name_tool", "-add_rpath", target, path], check=False)

    subprocess.run(["codesign", "--force", "--deep", "--sign", "-", bundle_dir], check=True)


if __name__ == "__main__":
    main()
