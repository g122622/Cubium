# 方块粒子 (Block Particles)

## 概述

方块粒子用于方块相关的视觉效果，如挖掘、破坏、下落灰尘等。这些粒子使用 TERRAIN_SHEET 渲染类型，从方块纹理图集获取纹理。

## 文件

| 文件 | 描述 |
|------|------|
| DiggingParticle.hpp/cpp | 挖掘粒子 - 挖掘方块时产生 |

## 特性

### DiggingParticle（挖掘粒子）

挖掘粒子在玩家挖掘或破坏方块时产生，显示被挖掘方块的纹理。

#### 核心特性

- **渲染类型**：`TERRAIN_SHEET`（使用方块纹理图集而非粒子纹理图集）
- **生命周期**：约 16-24 tick（随机变化）
- **尺寸**：0.05-0.1 格（随机变化）

#### 纹理获取机制

挖掘粒子在构造时从 `BlockModelCache` 获取方块纹理：

1. 通过 `ChunkMesher::modelCache()` 获取全局 `BlockModelCache` 实例
2. 调用 `getBlockAppearance(&blockState)` 获取 `BlockAppearance`
3. 从 `faceTextures` 映射中随机选择一个面的纹理
4. 将纹理 UV 坐标存储在 `m_textureRegion` 成员中

#### 随机 UV 偏移

参考 MC 1.16.5 的 `field_217587_G` 和 `field_217588_H`：

```cpp
// 在 16x16 纹理中随机选取 4x4 区域
m_uvOffsetU = static_cast<f32>(rng.nextInt(4));  // 0, 1, 2, 或 3
m_uvOffsetV = static_cast<f32>(rng.nextInt(4));

// 在 buildVertices 中计算最终 UV
f64 subU0 = m_textureRegion.u0 + (regionWidth * m_uvOffsetU / 4.0);
f64 subV0 = m_textureRegion.v0 + (regionHeight * m_uvOffsetV / 4.0);
```

#### 物理行为

- 重力：0.03 blocks/tick²
- 空气摩擦：0.92
- 地面摩擦：0.7
- 有碰撞检测

#### 淡出效果

生命周期后半段（>70%）逐渐淡出：

```cpp
if (lifeRatio > 0.7f) {
    m_color.a = 1.0f - static_cast<f32>((lifeRatio - 0.7f) / 0.3f);
}
```

## 用法

```cpp
// 创建挖掘粒子（推荐方式）
const BlockState& state = VanillaBlocks::STONE->defaultState();
auto particle = DiggingParticle::createWithBlock(
    glm::vec3(10.0f, 64.0f, 20.0f),  // 位置
    glm::vec3(0.1f, 0.2f, 0.1f),      // 速度
    state                              // 方块状态
);
particleManager.addParticle(std::move(particle));

// 使用默认石头纹理创建（不推荐）
auto defaultParticle = DiggingParticle::create(
    position,
    velocity,
    nullptr  // world（可选）
);
```

## 实现细节

### 初始化流程

```
构造函数
    ↓
initializeBlockTexture()
    ↓
ChunkMesher::modelCache()
    ↓
BlockModelCache::getBlockAppearance(&blockState)
    ↓
从 faceTextures 随机选择纹理
    ↓
存储到 m_textureRegion
```

### 渲染流程

```
buildVertices()
    ↓
检查 m_hasValidTexture
    ↓
如果有有效纹理：从 m_textureRegion 计算 4x4 子区域 UV
    ↓
如果无效：使用默认全纹理 (0,0)-(1,1)
    ↓
生成 billboard quad 的 4 个顶点
```

### 回退机制

当无法获取方块纹理时：
- `BlockModelCache` 不可用
- 方块状态没有对应的外观
- `faceTextures` 为空

使用默认全纹理 UV 坐标 (0,0)-(1,1)。

## 测试覆盖

测试文件位于 `tests/client/renderer/trident/particle/particles/block/DiggingParticleTest.cpp`：

- 创建测试：`CreateWithBlock_ReturnsValidParticle`
- 渲染类型测试：`GetRenderType_ReturnsTerrainSheet`
- 物理属性测试：`HasGravity`、`HasPhysics`、`HasFriction`
- 生命周期测试：`Tick_ExpiresAfterLifetime`、`Tick_FadesOutInLateLifetime`
- 不同方块类型测试：`CreateWithBlock_DifferentBlockTypes`
- 顶点生成测试：`BuildVertices_WithoutTextureAtlas_ProducesVertices`

## 扩展粒子

后续可添加的其他方块粒子：

| 粒子 | 描述 | MC 1.16.5 参考 |
|------|------|----------------|
| BreakingParticle | 物品破碎效果 | `BreakingParticle` |
| FallingDustParticle | 下落灰尘 | `FallingDustParticle` |
| BlockCrackParticle | 方块裂纹 | `BlockCrackParticle` |

## 参考

- Minecraft Java 1.16.5 `net.minecraft.client.particle.DiggingParticle`
- `field_217587_G` 和 `field_217588_H`：随机 UV 偏移字段
- `BlockModelShapes.getTexture()`：获取方块纹理
