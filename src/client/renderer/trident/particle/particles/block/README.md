# 方块粒子 (Block Particles)

方块粒子用于方块相关的视觉效果，使用 TERRAIN_SHEET 渲染类型从方块纹理图集获取纹理。

## 目录结构

```
block/
├── DiggingParticle.hpp     # 挖掘粒子（破坏方块时产生）
├── DiggingParticle.cpp     # 挖掘粒子实现
├── DustPillarParticle.hpp  # 尘柱粒子（重锤砸地攻击产生，使用方块纹理）
└── DustPillarParticle.cpp  # 尘柱粒子实现
```

## 内部模块关系

- `DustPillarParticle` 继承自 `DiggingParticle`，复用方块纹理渲染逻辑（`_initializeBlockTexture()`、`buildVertices()` 等）。
- `DustPillarParticle` 重写重力（1.0 vs DiggingParticle 的 0.03）和生命周期（20-40 tick vs 16-24 tick），以匹配 MC Java 的 DustPillarProvider 行为。
- `DustPillarParticle` 在构造函数中重写速度：X/Z 替换为 `nextGaussian() / 30.0`（极低水平扩散），Y 保留传入值并叠加 `nextGaussian() / 2.0`（先扬后抑的抛物线），匹配 MC Java 的 `DustPillarProvider.setParticleSpeed()` 行为。

## 上下游外部依赖关系

### 依赖的上游模块

- `mc::client::renderer::trident::particle::Particle` - 粒子基类
- `mc::client::renderer::trident::chunk::ChunkMesher` - 获取全局 `BlockModelCache`
- `mc::client::resource::BlockModelCache` - 获取方块纹理和粒子纹理
- `mc::client::resource::ResourceManager` - 通过 `BlockAppearance::particleTexture` 提供模型定义的粒子纹理
- `mc::world::block::BlockState` - 方块状态
- `mc::math::Random` - 随机数生成

### 被下游模块依赖

- `ParticleRegistry` - 通过 `create()` / `createWithBlock()` 工厂方法创建粒子实例
- `ParticleManager` - 管理粒子生命周期和渲染
- 方块破坏逻辑 - 通过 `DiggingParticle::createWithBlock()` 创建粒子

## 容易踩的坑

1. **纹理获取失败**：`BlockModelCache` 不可用或方块状态没有对应外观时，会使用默认全纹理 UV (0,0)-(1,1)，可能导致粒子显示不正确。应确保在创建粒子前 `ChunkMesher::modelCache()` 已初始化。

2. **粒子纹理选择**：`_initializeBlockTexture()` 优先使用 `BlockAppearance::particleTexture`（模型 JSON 中 `textures.particle` 指定的纹理），若模型未定义粒子纹理则回退到从 `faceTextures` 中随机选取一个面的纹理。这与 MC 行为一致。

3. **UV 偏移计算**：挖掘粒子从 16x16 纹理中随机选取 4x4 区域（模拟 MC 1.16.5 的 `field_217587_G`/`field_217588_H`），UV 偏移范围为 0-3。

4. **默认工厂方法**：`create()` 默认使用石头方块状态，但依赖 `VanillaBlocks::STONE` 已初始化。推荐使用 `createWithBlock()` 并传入正确的 `BlockState`。

5. **渲染类型**：必须使用 `TERRAIN_SHEET` 渲染类型，因为方块粒子使用方块纹理图集而非粒子纹理图集。

6. **生命周期淡出**：生命周期超过 70% 后开始淡出，alpha 值线性递减。
