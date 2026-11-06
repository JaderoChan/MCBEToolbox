# MCBE Toolbox 前端

使用 Qt6.7+ 实现的前端界面，用户能够以可视化界面调整生成参数、选取 Fallback 方块（透明像素填充方块）、选择可用方块列表与 NBT 编辑器。

## 构建需求

- **C++17+**
- **Qt6.7+**
- **MCBE Toolbox 后端库**

## 构建选项

| 选项 | 作用 | 默认值 | 备注 |
| --- | --- | --- | --- |
| `KEEP_TERMINAL` | 保留程序控制台入口（用于开发调试） | `OFF` | 仅 **Windows** 平台下可用 |
| `UPDATE_TRANSLATION_MAPPING_FILE` | 自动提取程序文本翻译 ID（用于开发后提取文本翻译 ID） | `OFF` | - |
| `DEPLOYQT_AFTER_BUILD` | 构建结束后是否自动进行 Qt 部署（拉取 Qt 相关的依赖库） | `ON` | 需要保证 `*deployqt` 工具可见 |
| `CREATE_DMG` | 构建结束后是否进行 DMG 打包 | `ON` | 仅 **macOS** 平台下可用，需要保证 `create-dmg` 工具可见 |
