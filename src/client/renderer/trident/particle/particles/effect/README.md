# 效果粒子 (Effect Particles)

效果粒子用于各种视觉特效，如火焰、烟雾、爆炸、传送门等。

## 目录结构

```
effect/
├── AshParticle.hpp/cpp              # 灰烬粒子（AshParticle + WhiteAshParticle）
├── CampfireParticle.hpp/cpp         # 营火烟雾粒子（Cozy/Signal 两种类型）
├── CopperFireFlameParticle.hpp/cpp  # 铜火火焰粒子（与 FlameParticle 行为相同，使用铜火纹理）
├── CritParticle.hpp/cpp             # 暴击粒子（含 EnchantedHitParticle 附魔暴击粒子）
├── DamageIndicatorParticle.hpp/cpp  # 伤害指示器粒子（实体受伤时弹出）
├── DragonBreathParticle.hpp/cpp     # 龙息粒子（含 EndRod、SweepAttack）
├── DustParticle.hpp/cpp             # 灰尘粒子（Dust 可配置颜色）+ 颜色过渡灰尘粒子（DustColorTransition）
├── DustPlumeParticle.hpp/cpp        # 灰尘羽粒子（锤击攻击产生的棕灰色烟尘）
├── EmitterParticle.hpp/cpp          # 发射器粒子基类及实现（HugeExplosion/Flame/Smoke Emitter）
├── EggCrackParticle.hpp/cpp         # 蛋壳碎裂粒子（蛋白色小粒子，随机漂移）
├── ElderGuardianParticle.hpp/cpp    # 远古守卫者外观粒子（深蓝灰色，向上漂浮淡出）
├── ElectricSparkParticle.hpp/cpp    # 电火花粒子（避雷针电火花，蓝白色发光）
├── ExplosionParticle.hpp/cpp        # 爆炸粒子（大型爆炸效果）
├── FireflyParticle.hpp/cpp          # 萤火虫粒子（暖黄绿色发光，闪烁效果）
├── FireworkParticle.hpp/cpp         # 烟花爆炸粒子（明亮白色-黄色，膨胀淡出）
├── FlashParticle.hpp/cpp            # 闪光粒子（极短生命周期，快速缩小淡出）
├── FlameParticle.hpp/cpp            # 火焰粒子（发光、向上漂浮、缩小淡出）
├── GlowParticle.hpp/cpp             # 发光地衣粒子（暖色发光，轻微上浮，缩小淡出）
├── GustParticle.hpp/cpp             # 风爆粒子（GustParticle + SmallGustParticle）
├── GustEmitterParticle.hpp/cpp      # 风爆发射器粒子（GustEmitterLarge + GustEmitterSmall，不可见）
├── InfestedParticle.hpp/cpp         # 虫蚀方块粒子（暗灰绿色云雾，漂浮扩散淡出）
├── ItemPickupParticle.hpp/cpp       # 物品拾取粒子（拾取物品时的烟雾效果）
├── LavaParticle.hpp/cpp             # 熔岩滴粒子（发光、下落）
├── LightParticle.hpp/cpp            # 光源粒子（结构方块显示用）
├── NoteParticle.hpp/cpp             # 音符粒子（音符盒产生，颜色由音高决定）
├── OmenParticle.hpp/cpp             # 预兆粒子（OminousSpawning + RaidOmen + TrialOmen）
├── PoofParticle.hpp/cpp             # 消散粒子（云雾消散效果）
├── PortalParticle.hpp/cpp           # 传送门粒子（紫色、水平摆动）+ 反向传送门粒子（绿色、反向旋转）
├── RedstoneParticle.hpp/cpp         # 红石粉尘粒子（发光、颜色可变）+ 附魔粒子 + 下落灰尘粒子
├── SculkChargeParticle.hpp/cpp      # 幽匿充能粒子（SculkCharge + SculkChargePop）
├── SculkSoulParticle.hpp/cpp        # 幽匿灵魂粒子（蓝青色发光，向上漂浮）
├── ShriekParticle.hpp/cpp           # 尖啸粒子（深蓝色，向上漂浮，延迟淡入）
├── SmallFlameParticle.hpp/cpp       # 小型火焰粒子（蜡烛等使用，FlameParticle 一半尺寸）
├── SmokeParticle.hpp/cpp            # 烟雾粒子（向上飘动、变大淡出）
├── SonicBoomParticle.hpp/cpp        # 音爆粒子（监守者音爆，蓝白色高速，无摩擦匀速）
├── SoulParticle.hpp/cpp             # 灵魂粒子（灵魂火效果）
├── SpellParticle.hpp/cpp            # 药水效果粒子（魔法效果、多种颜色）
├── TrialSpawnerParticle.hpp/cpp     # 试炼刷怪笼粒子（TrialSpawnerDetection + TrialSpawnerDetectionOminous）
└── WhiteSmokeParticle.hpp/cpp       # 白色烟雾粒子（白灰色调、方向性发射）
```

## 内部模块关系

```
Particle (基类，在 particle/ 目录)
    ↑
    ├── FlameParticle ────────────┐
    ├── SmokeParticle             │
    ├── LavaParticle              │
    ├── PortalParticle            │  普通效果粒子
    │   (含 ReversePortalParticle)│
    ├── CritParticle ──────────────┐
    │   (含 EnchantedHitParticle)  │  暴击/附魔暴击粒子
    ├── ExplosionParticle         │
    ├── PoofParticle              │
    ├── SpellParticle             │
    ├── DragonBreathParticle      │
    ├── SoulParticle              │
    ├── RedstoneParticle          │
    ├── WhiteSmokeParticle        │
    ├── CampfireParticle ─────────┘
    ├── AshParticle               # 灰烬粒子（Ash + WhiteAsh）
    ├── CopperFireFlameParticle   # 铜火火焰粒子
    ├── DamageIndicatorParticle   # 伤害指示器粒子
    ├── DustParticle              # 灰尘粒子（Dust + DustColorTransition）
    ├── DustPlumeParticle         # 灰尘羽粒子
    ├── EggCrackParticle          # 蛋壳碎裂粒子
    ├── ElderGuardianParticle     # 远古守卫者外观粒子
    ├── ElectricSparkParticle     # 电火花粒子
    ├── FireflyParticle           # 萤火虫粒子
    ├── FireworkParticle          # 烟花爆炸粒子
    ├── FlashParticle             # 闪光粒子
    ├── GlowParticle              # 发光地衣粒子
    ├── GustParticle              # 风爆粒子（Gust + SmallGust）
    ├── InfestedParticle          # 虫蚀方块粒子
    ├── ItemPickupParticle        # 物品拾取粒子
    ├── LightParticle             # 光源粒子
    ├── NoteParticle              # 音符粒子
    ├── OmenParticle              # 预兆粒子（OminousSpawning + RaidOmen + TrialOmen）
    ├── SculkChargeParticle       # 幽匿充能粒子（SculkCharge + SculkChargePop）
    ├── SculkSoulParticle         # 幽匿灵魂粒子
    ├── ShriekParticle            # 尖啸粒子
    ├── SmallFlameParticle        # 小型火焰粒子
    ├── SonicBoomParticle         # 音爆粒子
    ├── TrialSpawnerParticle      # 试炼刷怪笼粒子（Detection + DetectionOminous）
    │
    └── EmitterParticle ──────────┐
              ↑                   │  发射器粒子
              ├── HugeExplosionEmitterParticle  │  （生成其他粒子）
              ├── FlameEmitterParticle ─────────┘
              ├── SmokeEmitterParticle
              ├── GustEmitterLargeParticle     # 大型风爆发射器
              └── GustEmitterSmallParticle     # 小型风爆发射器
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

8. **PortalParticle.hpp 包含两个粒子类**：`PortalParticle`（紫色）和 `ReversePortalParticle`（绿色），共享水平摆动逻辑但旋转方向相反。

9. **AshParticle.hpp 包含两个粒子类**：`AshParticle`（灰橙色，重力 0.003）和 `WhiteAshParticle`（白灰色，重力 0.002），均使用 LIT 渲染类型和自发光。

10. **GustParticle.hpp 包含两个粒子类**：`GustParticle`（大型风爆，生命周期 10 tick）和 `SmallGustParticle`（小型风爆，生命周期 6 tick），均为 TRANSLUCENT 渲染。

11. **GustEmitterParticle.hpp 包含两个发射器类**：`GustEmitterLargeParticle`（每 tick 发射 2~3 个 GustParticle）和 `GustEmitterSmallParticle`（每 tick 发射 1~2 个 SmallGustParticle），均为 NO_RENDER 渲染类型。

12. **OmenParticle.hpp 包含三个粒子类**：`OminousSpawningParticle`（深紫蓝色）、`RaidOmenParticle`（深红色）和 `TrialOmenParticle`（深蓝色），行为相同仅颜色不同，均使用 entity_effect 纹理。

13. **SculkChargeParticle.hpp 包含两个粒子类**：`SculkChargeParticle`（青色发光，旋转并变大淡出）和 `SculkChargePopParticle`（明亮青色闪光，旋转并缩小淡出），均使用 LIT 渲染类型。

14. **TrialSpawnerParticle.hpp 包含两个粒子类**：`TrialSpawnerDetectionParticle`（橙黄色发光）和 `TrialSpawnerDetectionOminousParticle`（蓝色-青色发光），行为相同仅颜色和纹理不同，均使用 LIT 渲染类型。

15. **DustParticle.hpp 包含两个粒子类**：`DustParticle`（可配置颜色）和 `DustColorTransitionParticle`（生命周期内颜色渐变），均使用 OPAQUE 渲染和自发光。通过 DustParticleData 和 DustColorTransitionParticleData 传递颜色和缩放数据，数据工厂已在 ParticleFactories.cpp 中注册。

16. **DamageIndicatorParticle 与 CritParticle 的区别**：两者使用相同纹理（critical_hit），但 DamageIndicatorParticle 向上弹出并减速（微弱负重力 -0.04），而 CritParticle 沿攻击方向飞行。
