# 特殊粒子 (Special Particles)

## 目录结构树

```
special/
├── NautilusParticle.hpp/cpp          # 鹦鹉螺粒子（发光、三阶段缩放、无重力）
├── VaultConnectionParticle.hpp/cpp   # 宝库连接粒子（从源位置飞向目标位置）
└── VibrationSignalParticle.hpp/cpp   # 振动信号粒子（从源位置飞向目标位置）
```

## 内部模块关系

```
Particle (基类)
    ↑
    ├── NautilusParticle           # 独立粒子类
    ├── VaultConnectionParticle    # 宝库连接粒子（与 VibrationSignalParticle 类似的定向粒子）
    └── VibrationSignalParticle    # 振动信号粒子类
```

所有粒子类继承自 `Particle` 基类 (`client/renderer/trident/particle/Particle.hpp`)。

## 上下游外部依赖关系

**依赖方（上游）**：
- `Particle` 基类 - 提供生命周期、渲染接口
- `ParticleRegistry` - 粒子类型注册
- `ParticleTextureAtlas` - 纹理图集
- `mc::math::Random` - 随机数生成
- `VibrationParticleData` - 振动粒子数据（目标位置 + 到达时间）

**被依赖方（下游）**：
- 实体系统 - 潮涌核心效果（NautilusParticle）
- 振动系统 - 幽匿感测体、幽匿尖啸体、监守者振动信号（VibrationSignalParticle）
- 宝库系统 - 宝库解锁连接光束（VaultConnectionParticle）
- 网络同步 - 通过 `ParticlePacket` 接收服务端粒子事件
- `ClientApplicationNetwork` - 振动粒子回调处理

## 容易踩的坑

1. **getScale() 返回值是乘数而非绝对尺寸**：`getScale()` 返回的值会与 `m_size` 相乘（渲染管线：`halfSize = m_size * scale * 0.5`），因此 getScale() 应仅返回乘数（如 0~1 范围），不要乘以 m_initialSize 或 size()，否则渲染尺寸会被平方放大。

2. **VibrationSignalParticle 的运动完全由目标位置驱动**：粒子的速度 (velocity) 不用于定位，而是通过 `m_targetPosition` 和 `m_arrivalInTicks` 计算插值移动。不要尝试通过设置速度来控制振动粒子的运动。

3. **VaultConnectionParticle 与 VibrationSignalParticle 共享运动逻辑**：两者均使用 `glm::mix` 插值向目标位置飞行，具有相同的指数缓动效果。VaultConnectionParticle 使用橙金色 (0.9, 0.7, 0.3) 并在 70% 生命周期后淡出。
