# 特殊粒子 (Special Particles)

## 目录结构树

```
special/
├── NautilusParticle.hpp/cpp   # 鹦鹉螺粒子（发光、三阶段缩放、无重力）
└── README.md
```

## 内部模块关系

```
Particle (基类)
    ↑
    └── NautilusParticle   # 独立粒子类
```

所有粒子类继承自 `Particle` 基类 (`client/renderer/trident/particle/Particle.hpp`)。

## 上下游外部依赖关系

**依赖方（上游）**：
- `Particle` 基类 - 提供生命周期、渲染接口
- `ParticleRegistry` - 粒子类型注册
- `ParticleTextureAtlas` - 纹理图集
- `mc::math::Random` - 随机数生成

**被依赖方（下游）**：
- 实体系统 - 潮涌核心效果
- 网络同步 - 通过 `ParticlePacket` 接收服务端粒子事件

## 容易踩的坑

1. **getScale() 返回值是乘数而非绝对尺寸**：`getScale()` 返回的值会与 `m_size` 相乘（渲染管线：`halfSize = m_size * scale * 0.5`），因此 getScale() 应仅返回乘数（如 0~1 范围），不要乘以 m_initialSize 或 size()，否则渲染尺寸会被平方放大。
