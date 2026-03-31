# 天气粒子 (Weather Particles)

## 概述

天气粒子用于雨、雪、溅射等天气效果。

## 文件

| 文件 | 描述 |
|------|------|
| RainParticle.hpp/cpp | 雨滴粒子 - 雨天效果 |
| SnowParticle.hpp/cpp | 雪花粒子 - 雪天效果 |
| SplashParticle.hpp/cpp | 溅射粒子 - 雨滴落地效果 |

## 特性

### RainParticle（雨滴粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT
- **生命周期**：约 8 ticks
- **行为**：
  - 受重力影响快速下落
  - 有终端速度限制
  - 落地后有概率消失
- **颜色**：淡蓝色半透明

### SnowParticle（雪花粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT
- **生命周期**：约 200 ticks
- **行为**：
  - 受重力缓慢下落
  - 水平摇摆效果
  - 淡出消失
- **颜色**：白色几乎不透明

### SplashParticle（溅射粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT
- **生命周期**：约 15 ticks
- **行为**：
  - 受重力影响
  - 从碰撞点向上喷射
  - 淡出消失
- **颜色**：淡蓝色半透明

## 用法

```cpp
// 创建溅射粒子
auto splash = std::make_unique<SplashParticle>(
    position,
    glm::vec3(0.0f, 0.1f, 0.0f)  // 初始向上速度
);
particleManager.addParticle(std::move(splash));
```

## 参考

- Minecraft Java 1.16.5 `net.minecraft.client.particle.RainParticle`
- Minecraft Java 1.16.5 `net.minecraft.client.particle.SnowParticle`
- Minecraft Java 1.16.5 `net.minecraft.client.particle.SplashParticle`
