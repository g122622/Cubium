# 特殊粒子 (Special Particles)

## 目录结构树

```
special/
├── NautilusParticle.hpp/cpp          # 鹦鹉螺粒子（发光、三阶段缩放、无重力）
├── TrailParticle.hpp/cpp             # 轨迹粒子（飞向目标位置、自定义颜色、加速缓动）
├── VaultConnectionParticle.hpp/cpp   # 宝库连接粒子（从源位置飞向目标位置）
└── VibrationSignalParticle.hpp/cpp   # 振动信号粒子（从源位置飞向目标位置）
```

## 内部模块关系

```
Particle (基类)
    ↑
    ├── NautilusParticle           # 独立粒子类
    ├── TrailParticle              # 轨迹粒子（飞向目标位置、ARGB 颜色）
    ├── VaultConnectionParticle    # 宝库连接粒子（与 VibrationSignalParticle 类似的定向粒子）
    └── VibrationSignalParticle    # 振动信号粒子类
```

所有粒子类继承自 `Particle` 基类 (`client/renderer/trident/particle/Particle.hpp`)。

## 数据管线

| 粒子类型 | 数据类 | 网络回调 | 数据工厂 | 说明 |
|---|---|---|---|---|
| VibrationSignalParticle | `VibrationParticleData` | `onVibrationParticle` | ✅ 已注册 | 目标位置 + 到达时间 |
| VaultConnectionParticle | `VibrationParticleData` | 无独立回调（MC Java 中为 SimpleParticleType） | ✅ 已注册 | 编程方式创建时使用 VibrationParticleData |
| TrailParticle | `TrailParticleData` | `onTrailParticle` | ✅ 已注册 | 目标位置 + ARGB 颜色 + 持续时间 |

- **VibrationSignalParticle**：通过网络回调 `onVibrationParticle` 接收 `ir::play::LevelParticles` 中解码的目标位置和到达时间，创建 `VibrationParticleData` 并通过数据管线传递给 `createWithTarget()`。
- **VaultConnectionParticle**：MC Java 中为 SimpleParticleType，velocity 字段传递目标偏移，`create()` 工厂方法直接处理。编程方式创建可通过 `addParticleWithData()` 使用 `VibrationParticleData`。
- **TrailParticle**：通过网络回调 `onTrailParticle` 接收 `ir::play::LevelParticles` 中解码的目标位置、颜色和持续时间，创建 `TrailParticleData` 并通过数据管线传递给 `createWithTarget()`。

## 上下游外部依赖关系

**依赖方（上游）**：
- `Particle` 基类 - 提供生命周期、渲染接口
- `ParticleRegistry` - 粒子类型注册
- `ParticleTextureAtlas` - 纹理图集
- `mc::math::Random` - 随机数生成
- `VibrationParticleData` - 振动/宝库连接粒子数据（目标位置 + 到达时间）
- `TrailParticleData` - 轨迹粒子数据（目标位置 + ARGB 颜色 + 持续时间）

**被依赖方（下游）**：
- 实体系统 - 潮涌核心效果（NautilusParticle）
- 振动系统 - 幽匿感测体、幽匿尖啸体、监守者振动信号（VibrationSignalParticle）
- 宝库系统 - 宝库解锁连接光束（VaultConnectionParticle）
- 诡异橡树/嘎枝 - 眼球花和心声方块轨迹效果（TrailParticle）
- 网络同步 - 通过 `ir::play::LevelParticles` 接收服务端粒子事件
- `ClientApplicationNetwork` - 振动/轨迹粒子回调处理

## 容易踩的坑

1. **getScale() 返回值是乘数而非绝对尺寸**：`getScale()` 返回的值会与 `m_size` 相乘（渲染管线：`halfSize = m_size * scale * 0.5`），因此 getScale() 应仅返回乘数（如 0~1 范围），不要乘以 m_initialSize 或 size()，否则渲染尺寸会被平方放大。

2. **VibrationSignalParticle / VaultConnectionParticle / TrailParticle 的运动完全由目标位置驱动**：粒子的速度 (velocity) 不用于定位，而是通过 `m_targetPosition` 和 `m_arrivalInTicks`/`m_durationInTicks` 计算插值移动。不要尝试通过设置速度来控制这些粒子的运动。

3. **VaultConnectionParticle 的网络层不使用 ParticleData**：MC Java 中 vault_connection 是 SimpleParticleType，目标位置通过 velocity 字段传递，不需要网络层 ParticleData。数据工厂仅用于编程方式创建（如通过 `addParticleWithData()`）。
