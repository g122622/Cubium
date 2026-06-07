# 天气粒子 (Weather Particles)

天气和水体相关的粒子效果，包括溅射、钓鱼涟漪等。

## 目录结构

```
weather/
├── SplashParticle.hpp/cpp    # 溅射粒子 - 雨滴落地/实体落水时向上喷射
└── FishingParticle.hpp/cpp   # 钓鱼粒子 - 水面涟漪效果
```

## 内部模块关系

- `SplashParticle`：独立的溅射粒子，由雨滴落地或实体落水触发
- `FishingParticle`：独立的钓鱼涟漪粒子，由钓鱼浮标触发

两个粒子类相互独立，均继承自 `Particle` 基类。

## 上下游外部依赖关系

**上游依赖**：
- `Particle`（`client/renderer/trident/particle/Particle.hpp`）：粒子基类
- `Random`（`common/util/math/random/Random.hpp`）：随机数生成
- `ClientWorld`（`client/ClientWorld.hpp`）：客户端世界接口

**下游调用者**：
- `ParticleManager`：通过 `ParticleRegistry` 创建和管理粒子实例
- `RainParticle`（未实现）：落地时生成 `SplashParticle`
- 钓鱼系统：生成 `FishingParticle` 用于浮标水面效果

## 容易踩的坑

1. **生命周期管理**：`tick()` 方法中必须手动增加 `m_age`，父类不会自动增加
2. **渲染类型**：天气粒子使用 `PARTICLE_SHEET_TRANSLUCENT`，需要正确的混合状态
3. **纹理位置**：纹理路径为 `minecraft:particle/splash` 和 `minecraft:particle/fishing`
