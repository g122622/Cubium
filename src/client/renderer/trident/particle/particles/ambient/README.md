# 环境粒子 (Ambient Particles)

## 概述

环境粒子用于水下、气泡等环境效果。

## 文件

| 文件 | 描述 |
|------|------|
| BubbleParticle.hpp/cpp | 气泡粒子 - 在水中向上升起 |
| UnderwaterParticle.hpp/cpp | 水下悬浮粒子 - 水下环境效果 |

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

## 用法

```cpp
// 创建气泡粒子
auto bubble = std::make_unique<BubbleParticle>(position, velocity);
particleManager.addParticle(std::move(bubble));

// 创建水下悬浮粒子
auto underwater = std::make_unique<UnderwaterParticle>(position, velocity);
particleManager.addParticle(std::move(underwater));
```

## 参考

- Minecraft Java 1.16.5 `net.minecraft.client.particle.BubbleParticle`
- Minecraft Java 1.16.5 `net.minecraft.client.particle.UnderwaterParticle`
