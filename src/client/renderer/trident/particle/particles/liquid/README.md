# 液体粒子 (Liquid Particles)

液体粒子用于实现水滴、熔岩滴、蜂蜜滴等液体滴落效果。

## 目录结构

```
liquid/
├── DripParticle.hpp/cpp          # 液体滴落粒子基类，实现三阶段状态机（Hanging → Falling → Landed）
├── DripWaterParticle.hpp/cpp     # 水滴粒子，从含水方块下方滴落
├── DripstoneDripParticle.hpp/cpp # 滴水石专用粒子（水/熔岩），落地时播放滴水音效
└── README.md
```

## 内部模块关系

```
DripParticle (基类)
    │
    ├── DripWaterParticle (水滴粒子)
    │       └── 重写 onLand() 委托给父类
    │
    ├── DripstoneWaterDripParticle (滴水石水滴粒子)
    │       └── 重写 onLand() 播放滴水音效 + 生成落地粒子
    │
    └── DripstoneLavaDripParticle (滴水石熔岩滴粒子)
            └── 重写 onLand() 播放滴熔岩音效 + 生成落地粒子
```

`DripParticle` 实现了液体滴落的核心状态机，子类只需重写关键方法即可实现不同液体类型。

`DripstoneWaterDripParticle` 和 `DripstoneLavaDripParticle` 是滴水石（钟乳石）专用的滴落粒子，
与普通水滴/熔岩滴粒子的区别在于：落地时会播放 `block.pointed_dripstone.drip_water` 或
`block.pointed_dripstone.drip_lava` 音效，对齐 MC Java 版的 `DripstoneFallAndLandParticle`。

粒子生命周期链：
- **水滴石**: DRIPSTONE_WATER(悬挂) → FALLING_DRIPSTONE_WATER(下落) → SPLASH(落地) + 音效
- **熔岩滴石**: DRIPSTONE_LAVA(悬挂) → FALLING_DRIPSTONE_LAVA(下落) → LANDING_LAVA(落地) + 音效

## 上下游外部依赖关系

**上游依赖（本目录依赖）：**
- `Particle` 基类 (`particle/Particle.hpp`) - 所有粒子的基类（含 `m_random` 随机源）
- `ClientWorld` (`client/world/ClientWorld.hpp`) - 世界状态查询、本地音效播放
- `Fluid`/`FluidTags` (`common/world/fluid/`) - 流体检测
- `PhysicsConstants` (`common/physics/`) - 物理常量
- `SoundEvents` (`common/sound/SoundEvents.hpp`) - 音效事件 ID
- `SoundCategory` (`common/sound/SoundCategory.hpp`) - 音效分类

**下游依赖（被依赖）：**
- `ParticleFactories.cpp` - 注册液体粒子工厂方法
- `ParticleRegistry` - 粒子类型注册表

## 容易踩的坑

1. **落地粒子生成**：`onLand()` 方法通过 `m_emitCallback` 生成落地粒子（Splash/LandingLava 等），滴水石粒子额外调用 `playLocalSound` 播放音效

2. **流体检测**：使用 `FluidTags::WATER()` 和 `FluidTags::LAVA()` 而非直接判断方块类型

3. **发光粒子亮度**：熔岩滴是发光粒子，`getLightColor()` 返回固定高亮度，而水滴使用世界光照

4. **状态机重力值变化**：Hanging 状态重力 0.02，Falling 状态根据类型不同（水 0.06，蜂蜜 0.01）

5. **滴水石粒子颜色**：水滴石粒子颜色 (0.2, 0.3, 1.0) 与普通水滴 (0.7, 0.7, 1.0) 不同，对齐 MC Java 版 DripParticle.DripstoneWaterHangProvider
