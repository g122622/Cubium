# 环境粒子 (Ambient Particles)

## 概述

环境粒子用于水下、气泡、孢子花等环境效果。

## 文件

| 文件 | 描述 |
|------|------|
| BubbleParticle.hpp/cpp | 气泡粒子 - 在水中向上升起 |
| UnderwaterParticle.hpp/cpp | 水下悬浮粒子 - 水下环境效果 |
| SporeBlossomParticle.hpp/cpp | 孢子花粒子 - 掉落孢子和空气漂浮效果 |

## 特性

### BubbleParticle（气泡粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT
- **生命周期**：约 2 秒
- **行为**：
  - 负重力（向上升起）
  - 随机水平漂移
  - 到达水面后消失
- **颜色**：淡蓝色半透明

### UnderwaterParticle（水下悬浮粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT
- **生命周期**：约 3 秒
- **行为**：
  - 无重力
  - 缓慢随机漂移
  - 淡出消失
- **颜色**：淡蓝色半透明

### FallingSporeBlossomParticle（孢子花掉落粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT
- **生命周期**：约 3-4 秒（随机化）
- **行为**：
  - 微弱重力（缓慢下落）
  - 轻微水平漂移
  - 生命周期后期淡出
- **颜色**：绿色调半透明 (0.32, 0.50, 0.22)
- **纹理**：minecraft:particle/spore_blossom

### SporeBlossomAirParticle（孢子花空气粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT
- **生命周期**：约 5-7 秒（随机化）
- **行为**：
  - 无重力（漂浮）
  - 缓慢三轴随机漂移
  - 渐入淡出效果
- **颜色**：绿色调半透明 (0.32, 0.50, 0.22)
- **纹理**：minecraft:particle/spore_blossom_air

## 用法

```cpp
// 创建气泡粒子
auto bubble = std::make_unique<BubbleParticle>(position, velocity);
particleManager.addParticle(std::move(bubble));

// 创建水下悬浮粒子
auto underwater = std::make_unique<UnderwaterParticle>(position, velocity);
particleManager.addParticle(std::move(underwater));

// 创建孢子花掉落粒子
auto falling = std::make_unique<FallingSporeBlossomParticle>(position, velocity);
particleManager.addParticle(std::move(falling));

// 创建孢子花空气粒子
auto air = std::make_unique<SporeBlossomAirParticle>(position, velocity);
particleManager.addParticle(std::move(air));
```

## 参考

- Minecraft Java 1.16.5 `net.minecraft.client.particle.BubbleParticle`
- Minecraft Java 1.16.5 `net.minecraft.client.particle.UnderwaterParticle`
- Minecraft Java 1.17+ `net.minecraft.client.particle.SuspendedParticle` (spore_blossom_air)
- Minecraft Java 1.17+ `net.minecraft.client.particle.FallingSporeBlossomParticle`
