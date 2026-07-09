# 粒子数据(Particle Data)

## 概述

粒子数据类用于携带粒子参数，支持网络序列化和命令行解析。

## 目录结构

```
data/
├── ParticleData.hpp              # 粒子数据基类，定义 getType() / getTypeName() / getParameters() / clone() 接口
├── BasicParticleData.hpp / cpp   # 无参数粒子数据（火焰、烟雾等）
├── BlockParticleData.hpp / cpp   # 方块粒子数据，携带 BlockState 参数
├── DustParticleData.hpp / cpp    # 灰尘粒子数据，携带 ARGB 颜色 + 缩放（Dust/DustColorTransition）
├── EntityEffectParticleData.hpp / cpp # 实体效果粒子数据，携带 ARGB 颜色（对应 MC ColorParticleOption）
├── ItemParticleData.hpp / cpp    # 物品粒子数据，携带 ItemStack 参数
├── RedstoneParticleData.hpp / cpp # 红石粒子数据，携带 RGB 颜色参数
├── VibrationParticleData.hpp / cpp # 振动粒子数据，携带目标来源（方块位置 或 实体ID+yOffset）+ 到达时间 i32
├── TrailParticleData.hpp / cpp   # 轨迹粒子数据，携带目标位置 Vector3d + 颜色 u32(ARGB) + 持续时间 i32
└── README.md
```

## 内部模块关系

```
ParticleData (基类)
    ├── BasicParticleData (无参数粒子)
    ├── BlockParticleData (方块粒子，依赖 BlockState)
    ├── ItemParticleData (物品粒子，依赖 ItemStack)
    ├── DustParticleData (灰尘粒子，携带 ARGB 颜色 + 缩放)
    ├── DustColorTransitionParticleData (颜色过渡灰尘粒子，携带起始颜色 + 目标颜色 + 缩放)
    ├── EntityEffectParticleData (实体效果粒子，仅携带 ARGB 颜色，无缩放)
    ├── RedstoneParticleData (红石粒子，携带 glm::vec3 颜色)
    ├── VibrationParticleData (振动粒子，携带目标来源（方块位置 或 实体ID+yOffset）+ 到达时间 i32)
    └── TrailParticleData (轨迹粒子，携带目标位置 Vector3d + 颜色 u32 + 持续时间 i32)
```

所有具体粒子数据类都继承自 `ParticleData` 基类，实现 `getType()`、`getTypeName()`、`getParameters()`、`clone()` 接口。

## 数据管线

粒子数据通过 `ParticleDataFactory` 传递到粒子创建流程：

1. **网络/命令层**：解析粒子数据，创建 `ParticleData` 子类实例
2. **注册层**：`ParticleRegistry::createParticleWithData()` 检查是否有数据工厂，有则调用数据工厂
3. **工厂层**：数据工厂从 `ParticleData*` 提取参数，调用 `createWithTarget()` 等方法创建粒子
4. **无数据回退**：当 `ParticleData` 为空或类型不匹配时，回退到 `create()` 使用默认值

已注册数据工厂的粒子类型：
- `Vibration`：从 `VibrationParticleData` 提取目标来源和到达时间。方块来源调用 `createWithTarget`，实体来源调用 `createWithEntityTarget`（每 tick 通过 ClientWorld 重新解析实体位置）
- `Trail`：从 `TrailParticleData` 提取目标位置、颜色和持续时间
- `VaultConnection`：从 `VibrationParticleData` 提取目标位置和到达时间（仅方块来源，实体来源回退到默认工厂）
- `Dust`：从 `DustParticleData` 提取 ARGB 颜色和缩放
- `Redstone`：从 `DustParticleData` 提取 ARGB 颜色和缩放
- `DustColorTransition`：从 `DustColorTransitionParticleData` 提取起始颜色、目标颜色和缩放
- `EntityEffect`：从 `EntityEffectParticleData` 提取 ARGB 颜色（对应 MC `ColorParticleOption`）
- `Item` / `ItemSlime` / `ItemCobweb` / `ItemSnowball`：从 `ItemParticleData` 提取 `ItemStack`，调用 `ItemParticle::createWithItemStack` 创建物品破碎粒子（对应 MC Java 1.21.11 的 `ItemParticleProvider`，通过 `BlockItemRegistry` 区分方块物品/非方块物品走不同纹理解析路径）

## EntityEffectParticleData 说明

`EntityEffectParticleData` 对应 MC Java 的 `ColorParticleOption`（用于 `ParticleTypes.ENTITY_EFFECT`）。

### 与 DustParticleData 的区别
| 特性 | EntityEffectParticleData | DustParticleData |
|------|--------------------------|------------------|
| 颜色格式 | i32 ARGB（4 字节） | i32 ARGB + f32 scale（8 字节） |
| 缩放因子 | 无 | 有（限制 [0.01, 4.0]） |
| MC 对应 | `ColorParticleOption(ENTITY_EFFECT)` | `DustParticleOption(DUST)` |
| 典型场景 | BellBlockEntity 共振、药水效果 | 红石、灰尘、告警牌 |

### 使用示例
```cpp
// 红色实体效果粒子
auto effectData = std::make_unique<EntityEffectParticleData>(0xFFFF0000);

// BellBlockEntity 共振场景（颜色递增 5）
i32 colorCounter = 16700985;
for (i32 k = 0; k < particleCount; ++k) {
    colorCounter += 5;
    auto data = std::make_unique<EntityEffectParticleData>(static_cast<u32>(colorCounter));
    // ...通过粒子数据管线创建粒子
}
```

## VibrationParticleData 说明

`VibrationParticleData` 对应 MC Java 的 `VibrationParticleOption`（用于 `ParticleTypes.VIBRATION`），携带目标位置来源和到达时间。

### 目标来源类型（对应 MC Java PositionSource）

| 来源类型 | TargetKind | 携带数据 | MC 对应 | 行为 |
|---------|-----------|---------|---------|------|
| 方块来源 | `Block` | `Vector3d` 固定坐标 | `BlockPositionSource` | 目标位置固定，粒子飞行期间不更新 |
| 实体来源 | `Entity` | `EntityId` + `f32 yOffset` | `EntityPositionSource` | 每 tick 通过 `ClientWorld.entityManager().getEntity(id)` 重新解析实体当前位置并叠加 yOffset，实体消失时粒子立即过期 |

### 使用示例
```cpp
// 方块来源：目标已解析为方块中心坐标
auto blockData = std::make_unique<VibrationParticleData>(
    Vector3d(10.5, 64.0, -20.5), 15);

// 实体来源：持有实体 ID 和 Y 轴偏移
auto entityData = std::make_unique<VibrationParticleData>(
    EntityId(42), 1.62f, 15);  // 1.62 = 玩家眼睛高度
```

## 上下游外部依赖关系

### 依赖的上游模块

| 模块 | 用途 |
|------|------|
| `common/world/block/Block.hpp` | BlockState 类型（BlockParticleData） |
| `common/item/core/ItemStack.hpp` | ItemStack 类型（ItemParticleData） |
| `client/renderer/trident/particle/ParticleTypes.hpp` | ParticleTypeId 枚举 |
| `client/renderer/trident/particle/ParticleRegistry.hpp` | 类型名称查找 |
| `common/util/assert/AssertAll.hpp` | 断言检查 |
| `glm/glm.hpp` | glm::vec3 颜色类型（RedstoneParticleData） |

### 被哪些下游模块依赖

| 模块 | 用途 |
|------|------|
| `client/renderer/trident/particle/ParticleRegistry.hpp` | 根据类型创建粒子数据 |
| `client/renderer/trident/particle/ParticleManager.hpp` | 携带粒子数据创建粒子 |
| `client/renderer/trident/particle/ParticleFactories.cpp` | 数据工厂注册 |
| `client/renderer/trident/particle/particles/` | 具体粒子实现（如 DiggingParticle 使用 BlockParticleData） |
| `client/world/ClientWorld.cpp` | `addEntityEffectParticle` 在客户端通过 `EntityEffectParticleData` 走数据管线 |
| `common/world/blockentity/interactive/BellBlockEntity.cpp` | 共振粒子发射使用 `IWorld::addEntityEffectParticle` |
| 服务端粒子包序列化 | 网络同步粒子效果（`ParticlePacket::createEntityEffect`） |
| `/particle` 命令解析 | 命令行参数解析 |

## 容易踩的坑

1. **粒子类型与数据类匹配**：创建粒子数据时必须使用正确的类型，否则会触发断言失败
   - `BlockParticleData` 必须使用 `ParticleTypeId::Block`、`Breaking`、`FallingDust`
   - `ItemParticleData` 必须使用 `ParticleTypeId::Item`、`ItemSlime`、`ItemSnowball`
   - `EntityEffectParticleData` 必须使用 `ParticleTypeId::EntityEffect`
   - 使用 `requiresBlockState()` / `requiresItemData()` / `requiresDustColor()` / `requiresColorData()` 辅助函数检查

2. **颜色范围**：`RedstoneParticleData` 的颜色分量会在构造函数中被 clamp 到[0, 1] 范围，传入时注意不需要手动 clamp

3. **类型名称解析**：通过 `BasicParticleData(const std::string& typeName)` 构造时，如果名称无效会得到 `ParticleTypeId::Invalid`，需要调用方检查 `isValidParticleType()`

4. **BlockState 的 modelKey**：`BlockParticleData::getParameters()` 会输出方块状态的属性参数，格式如 `minecraft:stone[variant=stone]`

5. **数据工厂回退**：当 `createParticleWithData()` 传入的 `ParticleData` 为 nullptr 或类型不匹配（dynamic_cast 失败）时，自动回退到普通 `create()` 工厂，使用默认值创建粒子

6. **EntityEffect 的两种调用路径**：
   - 编程式调用（如 `BellBlockEntity`）：`IWorld::addEntityEffectParticle()` → 客户端 `ClientWorld` 通过 `EntityEffectParticleData` 走数据管线；服务端 `ServerWorld` 广播 `ParticlePacket::createEntityEffect` 给附近玩家
   - 旧式 velocity 编码（如 `WitherEntity`、`SpellcastingIllagerEntity`）：`addParticle(EntityEffect, pos, colorVector)`，颜色编码在速度向量中（[0,1] 范围），由 `EntityEffectParticle::create()` 解码。仅支持 RGB，不支持完整 ARGB

