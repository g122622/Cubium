# 粒子类型定义 (Particle Types)

本目录定义了粒子类型 ID 枚举及相关辅助函数，供 common/client/server 三层共同使用。

## 目录结构

```
src/common/particle/
├── README.md              # 本文件
└── ParticleTypes.hpp      # 粒子类型枚举及辅助函数
```

## 模块说明

### ParticleTypes.hpp

定义 `mc::particle::ParticleTypeId` 枚举（底层类型 `u16`），包含 MC Java 版所有粒子类型 ID。

**枚举值按分类组织**：

| 区间       | 分类           | 示例                                       |
|-----------|---------------|-------------------------------------------|
| 0-9       | 环境类粒子      | AmbientEntityEffect, Bubble, Soul          |
| 10-19     | 方块/物品类粒子  | Block, Breaking, Item, DustPillar          |
| 20-39     | 效果类粒子      | Flame, Smoke, Explosion, Portal            |
| 40-49     | 液体滴落类粒子   | DrippingWater, DrippingLava, DrippingHoney  |
| 50-59     | 天气类粒子      | Rain, Snowflake, Splash, Cloud             |
| 60-69     | 生物相关粒子    | Heart, AngryVillager, Sneeze               |
| 70-79     | 特殊粒子       | TotemOfUndying, Flash, Firework            |
| 80-99     | 下界更新粒子    | Ash, Vibration, SculkSoul, CherryLeaves    |
| 100-116   | 扩展粒子       | CampfireCozy, OminousSpawning, GustDust    |

**辅助函数**：

- `isValidParticleType(id)` — 检查 ID 是否在有效范围内
- `requiresBlockState(id)` — 检查是否需要方块状态数据（Block/Breaking/FallingDust/DustPillar）
- `requiresItemData(id)` — 检查是否需要物品数据（Item/ItemSlime/ItemSnowball）
- `requiresDustColor(id)` — 检查是否需要红石颜色数据（Redstone/Dust/DustColorTransition）
- `requiresVibrationData(id)` — 检查是否需要振动数据（Vibration）

## 架构说明

### 迁移历史

`ParticleTypeId` 枚举最初定义在客户端层 `client/renderer/trident/particle/ParticleTypes.hpp`（命名空间 `mc::client::renderer::trident::particle`），但被 55+ 个 common 层文件依赖，违反了分层架构原则。

现已将枚举迁移至 common 层（`mc::particle` 命名空间），客户端层通过 using 别名保持向后兼容：

```cpp
// client/renderer/trident/particle/ParticleTypes.hpp
namespace mc::client::renderer::trident::particle {
using ParticleTypeId = mc::particle::ParticleTypeId;
// ... 其他 using 声明
}
```

### 上下游依赖

**上游**（本模块依赖）：
- `common/core/Types.hpp` — 基础类型定义（u16 等）

**下游**（依赖本模块）：
- `common/world/IWorld.hpp` — `addParticle()` 接口使用 `ParticleTypeId` 作为参数类型
- `common/world/block/IBlockAnimateContext.hpp` — `addAnimateParticle()` 接口
- `common/network/packet/ParticlePacket.hpp` — 网络包使用 `ParticleTypeId`
- `common/entity/` — 实体类生成粒子
- `common/world/block/` — 方块 animateTick 生成粒子
- `client/renderer/trident/particle/` — 客户端粒子渲染系统
- `server/` — 服务端粒子广播

### 对应 MC Java 版

本模块对应 MC Java 版的 `net.minecraft.core.particles.ParticleType` 和 `ParticleTypes` 注册表。MC Java 版使用泛型注册表模式（`ParticleType<T extends ParticleOptions>`），本项目采用更简洁的枚举 ID + 数据类方案。

## 注意事项

1. **枚举值分配**：新增粒子类型时，必须在此处分配唯一 ID 并按分类区间编排，避免与已有值冲突
2. **命名空间**：common 层使用 `mc::particle::ParticleTypeId`，客户端兼容层仍可使用 `mc::client::renderer::trident::particle::ParticleTypeId`
3. **前向声明**：如需前向声明，使用 `namespace mc::particle { enum class ParticleTypeId : u16; }`
