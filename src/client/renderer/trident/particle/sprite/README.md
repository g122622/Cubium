# 精灵动画系统 (Sprite Animation System)

## 目录结构

```
sprite/
├── ISprite.hpp          # 精灵接口（静态/动画精灵的统一抽象）
├── SimpleSprite.hpp/cpp # 简单精灵（单帧静态纹理）
├── AnimatedSprite.hpp/cpp # 动画精灵（多帧动画纹理，帧按垂直方向排列）
└── README.md
```

## 内部模块关系

```
ISprite（接口）
    │
    ├── SimpleSprite      # 单帧实现，age/maxAge/seed 被忽略
    │
    └── AnimatedSprite    # 多帧实现，基于 age/maxAge 计算当前帧
```

## 上下游外部依赖关系

### 上游依赖

| 依赖模块 | 用途 |
|---------|------|
| `common/core/Types.hpp` | 基础类型（f64, u32） |
| `glm` | glm::vec2、glm::vec4 向量类型 |

### 下游依赖

| 使用模块 | 用途 |
|---------|------|
| `ParticleTextureAtlas` | 创建 ISprite 实例（根据 SpriteInfo 创建 SimpleSprite 或 AnimatedSprite） |
| `TextureAtlasTicker` | 纹理图集动画驱动 |

## 容易踩的坑

1. **帧排列方向**：AnimatedSprite 要求帧在纹理中按**垂直方向**排列（从上到下），水平排列的帧条无法正确工作。

2. **UV 坐标语义**：`uvMin` 是左上角，`uvMax` 是右下角。对于 AnimatedSprite，`uvMax.y` 是整个动画条底部的 V 坐标，不是单帧底部。

3. **帧时间单位**：AnimatedSprite 的 `frameTime` 参数单位是**秒**，不是 ticks。

4. **getFrameUV 的帧选择**：基于 `age / maxAge` 的进度来选择帧，粒子生命周期结束时播放到最后一帧。如需循环动画，应使用 `getRandomFrameUV` 或自行计算帧索引。

5. **帧数保护**：AnimatedSprite 构造函数会自动将 `frameCount < 1` 修正为 1，不会崩溃但行为可能不符合预期。
