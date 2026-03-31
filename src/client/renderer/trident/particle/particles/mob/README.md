# 生物粒子 (Mob Particles)

## 概述

生物粒子用于与生物相关的视觉效果，如爱心、愤怒、开心等。

## 文件

| 文件 | 描述 |
|------|------|
| HeartParticle.hpp/cpp | 爱心粒子 - 生物繁殖或驯服时显示 |

## 特性

### HeartParticle（爱心粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT
- **生命周期**：约 1 秒
- **行为**：
  - 向上飘动（无重力）
  - 速度逐渐减慢
  - 后半生命周期淡出
- **颜色**：红色心形
- **用途**：
  - 生物繁殖时
  - 驯服动物时
  - 玩家喂食动物时

## 用法

```cpp
// 创建爱心粒子
auto heart = std::make_unique<HeartParticle>(
    position,
    glm::vec3(0.0f, 0.02f, 0.0f)
);
particleManager.addParticle(std::move(heart));
```

## 扩展粒子

后续可添加的其他生物粒子：
- **AngryVillagerParticle**：愤怒村民（灰色烟雾）
- **HappyVillagerParticle**：开心村民（绿色星星）
- **DamageIndicatorParticle**：伤害指示器

## 参考

- Minecraft Java 1.16.5 `net.minecraft.client.particle.HeartParticle`
