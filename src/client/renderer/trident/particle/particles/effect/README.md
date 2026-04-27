# 效果粒子 (Effect Particles)

## 概述

效果粒子用于各种视觉特效，如火焰、烟雾、爆炸、传送门等。

## 文件

| 文件 | 描述 |
|------|------|
| FlameParticle.hpp/cpp | 火焰粒子 - 发光、向上飘动、随年龄缩小 |
| SmokeParticle.hpp/cpp | 烟雾粒子 - 灰色、向上飘动、变大淡出 |
| LavaParticle.hpp/cpp | 熔岩滴粒子 - 发光、橙红色、下落 |
| PortalParticle.hpp/cpp | 传送门粒子 - 紫色、向下飘落、水平摆动 |
| CritParticle.hpp/cpp | 暴击粒子 - 发光、快速下落 |
| ExplosionParticle.hpp/cpp | 爆炸粒子 - 大型爆炸效果 |
| PoofParticle.hpp/cpp | 消散粒子 - 云雾消散效果 |
| SpellParticle.hpp/cpp | 药水效果粒子 - 魔法效果 |
| DragonBreathParticle.hpp/cpp | 龙息粒子 - 末影龙吐息效果 |
| SoulParticle.hpp/cpp | 灵魂粒子 - 灵魂火效果 |
| RedstoneParticle.hpp/cpp | 红石粉尘粒子 - 发光、颜色可变 |
| CampfireParticle.hpp/cpp | 营火烟雾粒子 - 营火烟雾效果 |
| **EmitterParticle.hpp/cpp** | **发射器粒子基类及实现** |

## 特性

### FlameParticle（火焰粒子）

- **渲染类型**：PARTICLE_SHEET_LIT（发光，不受光照影响）
- **生命周期**：约 1.5 秒
- **行为**：
  - 向上漂浮（无重力）
  - 随机水平漂移
  - 随年龄缩小
  - 后半生命周期淡出
- **颜色**：橙黄色（带随机变化）

### SmokeParticle（烟雾粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT（半透明）
- **生命周期**：约 2 秒
- **行为**：
  - 向上缓慢飘动
  - 随机水平漂移
  - 随年龄变大
  - 后半生命周期淡出
- **颜色**：灰色（带随机变化）

### LavaParticle（熔岩滴粒子）

- **渲染类型**：PARTICLE_SHEET_LIT（发光）
- **生命周期**：约 1.5 秒
- **行为**：
  - 受重力影响下落
  - 随机水平漂移
  - 后 40% 生命周期淡出
- **颜色**：橙红色

### PortalParticle（传送门粒子）

- **渲染类型**：PARTICLE_SHEET_TRANSLUCENT
- **生命周期**：约 2.5 秒
- **行为**：
  - 向下飘落
  - 水平方向正弦摆动
  - 旋转效果
  - 后半生命周期淡出
- **颜色**：紫色

## 用法

```cpp
// 创建火焰粒子
auto flame = std::make_unique<FlameParticle>(
    glm::vec3(x, y, z),
    glm::vec3(0.0f, 0.05f, 0.0f)
);
particleManager.addParticle(std::move(flame));

// 通过注册表创建
auto particle = ParticleRegistry::instance().createParticle(
    ParticleTypeId::Flame,
    position,
    velocity
);
```

## 参考

- Minecraft Java 1.16.5 `net.minecraft.client.particle.*`
