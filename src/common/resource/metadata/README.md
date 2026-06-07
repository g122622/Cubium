# 资源元数据模块

此模块提供资源元数据的解析和管理，用于解析资源包中的`.mcmeta`文件。

## 目录结构

```text
src/common/resource/metadata/
└── AnimationMetadata.hpp/cpp   # 动画纹理元数据（.png.mcmeta解析）
```

## 内部模块关系

模块仅包含 `AnimationMetadata` 一个组件，负责从 JSON 解析动画配置（帧时间、帧尺寸、帧序列、插值设置）。

## 上下游依赖关系

**下游依赖（谁使用了这个模块）**：
- `AnimatedSprite` - 动画精灵，消费 AnimationMetadata 进行帧播放
- `TextureAtlasBuilder` - 纹理图集构建器，读取 mcmeta 数据
- `ItemTextureAtlas` - 物品纹理图集
- `ParticleTextureAtlas` - 粒子纹理图集

**上游依赖（这个模块依赖了谁）**：
- `nlohmann-json` - JSON 解析
- `common/core/Types.hpp` - 基础类型定义（i32, u32, Size 等）

## 容易踩的坑

- `width`/`height` 为 -1 表示自动检测，需要在调用 `fromMcmeta()` 时传入图像尺寸才能正确计算
- `getFrameCount()` 返回自定义帧序列长度，若无自定义帧则返回 0，实际帧数需从图像高度计算
- `getFrameIndex()` 和 `getFrameTime()` 的 position 参数会自动取模，无需调用方处理循环
- JSON 解析失败时返回空 AnimationMetadata，`isValidAnimation()` 返回 false，需调用方检查
