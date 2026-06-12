# 效果粒子 (Effect Particles)

效果粒子用于各种视觉特效，如火焰、烟雾、爆炸、传送门等。参考 MC 1.16.5 `net.minecraft.client.particle.*` 实现。

## 目录结构

```
effect/
├── FlameParticle.hpp/cpp         # 火焰粒子（发光、向上漂浮、缩小淡出）
├── SmokeParticle.hpp/cpp         # 烟雾粒子（向上飘动、变大淡出）
├── LavaParticle.hpp/cpp          # 熔岩滴粒子（发光、下落）
├── PortalParticle.hpp/cpp        # 传送门粒子（紫色、水平摆动）
├── CritParticle.hpp/cpp          # 暴击粒子（发光、快速下落）
├── ExplosionParticle.hpp/cpp     # 爆炸粒子（大型爆炸效果）
├── PoofParticle.hpp/cpp          # 消散粒子（云雾消散效果）
├── SpellParticle.hpp/cpp         # 药水效果粒子（魔法效果、多种颜色）
├── DragonBreathParticle.hpp/cpp  # 龙息粒子（含 EndRod、SweepAttack）
├── SoulParticle.hpp/cpp          # 灵魂粒子（灵魂火效果）
├── RedstoneParticle.hpp/cpp      # 红石粉尘粒子（发光、颜色可变）
├── CampfireParticle.hpp/cpp      # 营火烟雾粒子（Cozy/Signal 两种类型）
└── EmitterParticle.hpp/cpp       # 发射器粒子基类及实现（HugeExplosion/Flame/Smoke Emitter）
```

## 内部模块关系

```
Particle (基类，在 particle/ 目录)
    ↑
    ├── FlameParticle ────────────┐
    ├── SmokeParticle             │
    ├── LavaParticle              │
    ├── PortalParticle            │  普通效果粒子
    ├── CritParticle              │  （直接继承 Particle）
    ├── ExplosionParticle         │
    ├── PoofParticle              │
    ├── SpellParticle             │
    ├── DragonBreathParticle      │
    ├── SoulParticle              │
    ├── RedstoneParticle          │
    ├── CampfireParticle ─────────┘
    │
    └── EmitterParticle ──────────┐
              ↑                   │  发射器粒子
              ├── HugeExplosionEmitterParticle  │  （生成其他粒子）
              ├── FlameEmitterParticle ─────────┘
              └── SmokeEmitterParticle
```

## 上下游依赖关系

**上游依赖（本目录依赖）：**
- `Particle.hpp` - 粒子基类
- `ParticleTypes.hpp` - 粒子类型 ID 枚举
- `ParticleRenderType.hpp` - 渲染类型枚举
- `ParticleRegistry.hpp` - 粒子注册表（用于发射器粒子创建子粒子）
- `glm` - 数学库

**下游依赖（依赖本目录）：**
- `ParticleRegistry` - 注册所有粒子类型
- `ParticleManager` - 管理和渲染粒子
- `ClientWorld` - 世界中的粒子效果触发
- 数据包处理 - 服务端粒子广播

## 容易踩的坑

1. **发光粒子亮度值**：MC 1.16.5 使用 `0xF0`（blockLight=15, skyLight=0）作为固定亮度，而非 15728880。参考 `FlameParticle::getLightColor()`。

2. **发射器粒子不渲染**：`EmitterParticle::getRenderType()` 返回 `NO_RENDER`，切勿误以为渲染出错。

3. **发射回调必须在 tick 中调用**：通过 `emit()` 发射子粒子时，`emitCallback` 由 `ParticleManager::tick()` 设置，确保回调已正确初始化。

4. **生命周期单位**：粒子生命周期单位是 **ticks**（1秒=20ticks），而非秒。如 `DEFAULT_LIFETIME = 30.0f` 表示约 1.5 秒。

5. **SpellParticle 多种类型**：SpellParticle 支持多种药水效果颜色（Instant、Mob、Wizard 等），创建时需正确设置颜色。

6. **DragonBreathParticle 复合功能**：该文件同时包含 EndRodParticle 和 SweepAttackParticle 的实现，因它们共享相似逻辑。

7. **getScale() 返回值是乘数而非绝对尺寸**：`getScale()` 返回的值会与 `m_size` 相乘（渲染管线：`halfSize = m_size * scale * 0.5`），因此 getScale() 应仅返回 0~1 范围的乘数。不要在 getScale() 中乘以 m_initialSize 或 size()，否则渲染尺寸会被平方放大。如需基于初始大小做动画，应在 tick() 中用 `setSize(m_initialSize * factor)` 更新 m_size，让 getScale() 返回 1.0；或让 getScale() 仅返回乘数。
