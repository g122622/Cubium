# 精灵动画系统 (Sprite Animation System)

## 概述

精灵系统用于管理粒子纹理的 UV 坐标和动画帧。支持静态精灵和动画精灵。

## 目录结构

```
sprite/
├── ISprite.hpp          # 精灵接口
├── SimpleSprite.hpp/cpp # 简单精灵（单帧）
├── AnimatedSprite.hpp/cpp # 动画精灵（多帧）
└── README.md
```

## 类设计

### ISprite（接口）

```cpp
class ISprite {
public:
    virtual ~ISprite() = default;

    // 获取 UV 坐标
    [[nodiscard]] virtual glm::vec4 getFrameUV(f32 age, f32 maxAge) const = 0;
    [[nodiscard]] virtual glm::vec4 getRandomFrameUV(u32 seed) const = 0;

    // 属性查询
    [[nodiscard]] virtual bool isAnimated() const = 0;
    [[nodiscard]] virtual u32 frameCount() const = 0;
    [[nodiscard]] virtual f32 frameTime() const = 0;
};
```

### SimpleSprite（简单精灵）

单帧静态纹理精灵：

```cpp
// 创建简单精灵
SimpleSprite sprite(
    glm::vec2(0.0f, 0.0f),  // UV 左上角
    glm::vec2(0.125f, 0.125f) // UV 右下角
);

// 获取 UV（对于静态精灵，age 和 maxAge 被忽略）
glm::vec4 uv = sprite.getFrameUV(age, maxAge);
```

### AnimatedSprite（动画精灵）

多帧动画纹理精灵，帧按垂直方向排列：

```cpp
// 创建动画精灵（8 帧，每帧 0.1 秒）
AnimatedSprite sprite(
    glm::vec2(0.0f, 0.0f),   // UV 左上角
    glm::vec2(0.125f, 1.0f), // UV 右下角（高度包含所有帧）
    8,                       // 帧数
    0.1f                     // 每帧时间
);

// 基于年龄获取当前帧 UV
glm::vec4 uv = sprite.getFrameUV(age, maxAge);

// 随机选择帧
glm::vec4 randomUv = sprite.getRandomFrameUV(seed);
```

## 纹理布局

### 静态纹理

```
┌──────────┐
│   帧 1   │  UV: (u0, v0) - (u1, v1)
│          │
└──────────┘
```

### 动画纹理（垂直帧条）

```
┌──────────┐
│   帧 1   │  V: v0 - v0+height
├──────────┤
│   帧 2   │  V: v0+height - v0+2*height
├──────────┤
│   帧 3   │
├──────────┤
│   帧 4   │
├──────────┤
│   ...    │
├──────────┤
│   帧 N   │  V: v0+(N-1)*height - v1
└──────────┘
```

## 与 ParticleTextureAtlas 集成

```cpp
// 从 ParticleTextureAtlas 获取精灵信息
const SpriteInfo* info = atlas.getSprite(location);
if (info) {
    if (info->isAnimated()) {
        auto sprite = std::make_unique<AnimatedSprite>(
            info->uvMin, info->uvMax,
            info->frameCount, info->frameTime
        );
    } else {
        auto sprite = std::make_unique<SimpleSprite>(
            info->uvMin, info->uvMax
        );
    }
}
```

## 动画模式

### 基于生命周期的动画

粒子从出生到死亡，帧从第一帧到最后一帧：

```cpp
glm::vec4 uv = sprite->getFrameUV(particle.age(), particle.maxAge());
```

### 随机帧

粒子随机选择一帧并保持：

```cpp
u32 seed = static_cast<u32>(particle.age() * 1000);
glm::vec4 uv = sprite->getRandomFrameUV(seed);
```

### 基于时间的循环动画

基于游戏时间循环播放：

```cpp
f32 time = GameTime::ticks() / 20.0f;  // 秒
u32 frame = static_cast<u32>(time / sprite->frameTime()) % sprite->frameCount();
```

## 参考

- Minecraft Java 1.16.5 `net.minecraft.client.renderer.texture.TextureAtlasSprite`
- Minecraft 资源包动画元数据格式（.mcmeta）
