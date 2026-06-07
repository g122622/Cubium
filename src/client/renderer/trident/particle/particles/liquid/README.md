# 液体粒子 (Liquid Particles)

液体粒子用于实现水滴、熔岩滴、蜂蜜滴等液体滴落效果。

## 目录结构

```
liquid/
├── DripParticle.hpp/cpp      # 液体滴落粒子基类，实现三阶段状态机（Hanging → Falling → Landed）
├── DripWaterParticle.hpp/cpp # 水滴粒子，从含水方块下方滴落
└── README.md
```

## 内部模块关系

```
DripParticle (基类)
    │
    └── DripWaterParticle (水滴粒子)
            └── 重写 onLand() 实现落地逻辑
```

`DripParticle` 实现了液体滴落的核心状态机，子类只需重写关键方法即可实现不同液体类型。

## 上下游外部依赖关系

**上游依赖（本目录依赖）：**
- `Particle` 基类 (`particle/Particle.hpp`) - 所有粒子的基类
- `ClientWorld` (`client/world/ClientWorld.hpp`) - 世界状态查询
- `Fluid`/`FluidTags` (`common/world/fluid/`) - 流体检测
- `Random` (`common/util/math/random/`) - 随机数生成
- `PhysicsConstants` (`common/physics/`) - 物理常量

**下游依赖（被依赖）：**
- `ParticleFactories.cpp` - 注册液体粒子工厂方法
- `ParticleRegistry` - 粒子类型注册表

## 容易踩的坑

1. **每次 tick 创建 Random 对象**：`DripParticle::tickHanging()` 中每次 tick 都创建新的 `Random` 对象，应该使用粒子自身的随机源（TODO 待修复）

2. **落地粒子生成未实现**：`onLand()` 方法中的落地粒子效果（SplashParticle 等）等待 ParticleManager 支持粒子生成后实现

3. **流体检测**：使用 `FluidTags::WATER()` 和 `FluidTags::LAVA()` 而非直接判断方块类型

4. **发光粒子亮度**：熔岩滴是发光粒子，`getLightColor()` 返回固定高亮度，而水滴使用世界光照

5. **状态机重力值变化**：Hanging 状态重力 0.02，Falling 状态根据类型不同（水 0.06，蜂蜜 0.01）
