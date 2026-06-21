# 环境粒子 (Ambient Particles)

## 目录结构

```
ambient/
├── BubbleParticle.hpp/cpp        # 水下气泡粒子
├── BubblePopParticle.hpp/cpp     # 气泡破裂粒子（BubbleParticle 离开水面时生成）
├── CloudParticle.hpp/cpp         # 云朵、屏障、水花、海豚粒子
├── SporeBlossomParticle.hpp/cpp  # 孢子花粒子（掉落+空气漂浮）
└── UnderwaterParticle.hpp/cpp    # 水下悬浮粒子
```

**CloudParticle.hpp 包含的粒子类：**
- `CloudParticle` - 云朵粒子
- `BarrierParticle` - 屏障粒子
- `WaterWakeParticle` - 水花粒子
- `DolphinParticle` - 海豚粒子

**SporeBlossomParticle.hpp 包含的粒子类：**
- `FallingSporeBlossomParticle` - 孢子花掉落粒子
- `SporeBlossomAirParticle` - 孢子花空气粒子

**BubblePopParticle 特性：**
- 生命周期 4 tick，微弱重力 0.008，有碰撞检测
- 5 帧动画纹理（bubble_pop_0 ~ bubble_pop_4），根据生命周期进度切换
- 通过 `getTextureLocation()` 动态返回当前帧纹理路径
- 渲染类型 `PARTICLE_SHEET_OPAQUE`，白色不透明
- 由 `BubbleParticle` 在离开水面时通过 `emitCallback` 生成

## 内部模块关系

本目录下的粒子相互独立，无内部依赖关系。

## 上下游外部依赖关系

**依赖：**
- `Particle` 基类 (`client/renderer/trident/particle/Particle.hpp`)
- `ClientWorld` (`client/world/ClientWorld.hpp`)
- `Random` (`common/util/math/random/Random.hpp`)
- `FluidTags` (`common/world/fluid/FluidTags.hpp`) - BubbleParticle 用于水面检测

**被依赖：**
- `ParticleRegistry` - 注册和创建粒子实例
- `ParticleManager` - 管理粒子生命周期

## 容易踩的坑

1. **BubbleParticle 水面检测**：必须使用 `FluidTags::WATER()` 而非 `isWaterAt()`，因为后者可能不识别流体标签。

2. **BubblePop 粒子生成**：BubbleParticle 在 `tick()` 中检测到离开水面时，通过 `m_emitCallback(ParticleTypeId::BubblePop, ...)` 生成 BubblePop 粒子。必须先检查 `m_emitCallback` 是否存在（为空则跳过），避免在非 ParticleManager 管理的场景中崩溃。

3. **BubblePop 帧动画**：BubblePopParticle 使用5帧动画纹理（bubble_pop_0 ~ bubble_pop_4），通过 `getTextureLocation()` 根据年龄进度动态返回帧路径。ParticleTextureAtlas 中需注册所有5帧纹理名称。

4. **SporeBlossomAirParticle 生成区域**：孢子花空气粒子在孢子花下方 21x10x21 区域内随机生成，不是直接从方块位置生成。

5. **淡出计算**：多个粒子使用 `FADE_START_RATIO` 和 `FADE_RANGE` 计算淡出，需确保 alpha 值在 0-1 范围内。
