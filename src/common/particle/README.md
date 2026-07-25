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

定义 `mc::particle::ParticleTypeId` 枚举（底层类型 `u16`），包含 MC Java 1.21.11 所有粒子类型 ID 及项目内部扩展。

**枚举值与 MC 协议 ID 一致**：0~114 为 MC Java 1.21.11 协议中定义的粒子类型，由注册顺序决定。115~123 为项目内部扩展粒子，不参与网络通信。

**协议粒子分类**（基于 MC 功能分组，不影响枚举值）：

| 区间       | 分类               | 示例                                           |
|-----------|-------------------|-----------------------------------------------|
| 0-2       | 方块类粒子         | AngryVillager, Block, BlockMarker              |
| 3-8       | 环境类粒子         | Bubble, Cloud, Crit, DragonBreath              |
| 9-13      | 液体滴落类粒子      | DrippingLava, FallingLava, DrippingWater       |
| 14-15     | 染色粒子           | Dust, DustColorTransition                     |
| 16-28     | 效果类粒子         | Spell, Enchant, Explosion, Gust, SonicBoom     |
| 29-31     | 方块/物品/烟花粒子  | FallingDust, Firework, Fishing                 |
| 32-52     | 火焰/效果粒子       | Flame, Infested, CherryLeaves, Item, Vibration |
| 53-69     | 烟雾/天气/生物粒子  | LargeSmoke, Lava, Smoke, Splash, Witch          |
| 70-79     | 水下/营地/蜂蜜粒子  | BubblePop, CampfireCozy, DrippingHoney         |
| 80-98     | 花蜜/孢子/下界粒子  | FallingNectar, Ash, Snowflake, Glow             |
| 99-114    | 铜蚀/幽匿/试炼粒子  | WaxOn, SculkSoul, TrialSpawnerDetection, Firefly|

**内部扩展粒子**（115~123，不在 MC 协议中）：

| 值  | 名称                  | 说明                                    |
|-----|----------------------|----------------------------------------|
| 115 | Breaking             | 方块破坏粒子（MC 中 Block 兼用此功能）       |
| 116 | Barrier              | 屏障方块显示粒子                           |
| 117 | Light                | 结构方块标记粒子                           |
| 118 | Redstone             | 红石粉尘粒子（MC 中由 Dust + 颜色数据实现）   |
| 119 | LargeExplosion       | 大型爆炸粒子（MC 中为 explosion_emitter）    |
| 120 | ItemPickup           | 物品拾取粒子                              |
| 121 | DrippingCherryLeaves | 滴落樱花树叶                              |
| 122 | FallingCherryLeaves  | 下落樱花树叶                              |
| 123 | LandingCherryLeaves  | 落地樱花树叶                              |

**辅助函数**：

- `isValidParticleType(id)` — 检查 ID 是否在有效范围内（< Count）
- `isProtocolParticleType(id)` — 检查 ID 是否为 MC 协议定义的类型（< 115）
- `toProtocolId(id)` — 将内部粒子类型 ID 转换为 MC 协议 ID（内部扩展粒子映射到最接近的协议粒子）
- `fromProtocolId(protocolId)` — 从 MC 协议 ID 转换为内部粒子类型 ID
- `requiresBlockState(id)` — 检查是否需要方块状态数据（Block/BlockMarker/FallingDust/DustPillar/BlockCrumble）
- `requiresItemData(id)` — 检查是否需要物品数据（Item/ItemSlime/ItemSnowball/ItemCobweb）
- `requiresColorData(id)` — 检查是否需要颜色数据（Dust/DustColorTransition/EntityEffect/Flash/TintedLeaves）
- `requiresDustColor(id)` — 检查是否需要红石颜色数据（Dust/DustColorTransition，以及内部扩展 Redstone）
- `requiresSpellData(id)` — 检查是否需要药水类型数据（Spell/InstantSpell）
- `requiresVibrationData(id)` — 检查是否需要振动数据（Vibration）
- `requiresSculkChargeData(id)` — 检查是否需要幽匿充能数据（SculkCharge）
- `requiresShriekData(id)` — 检查是否需要尖啸延迟数据（Shriek）
- `requiresTrailData(id)` — 检查是否需要轨迹数据（Trail）
- `requiresPowerData(id)` — 检查是否需要力量数据（DragonBreath）

**内部扩展粒子的协议映射**：

`toProtocolId()` 将内部扩展粒子（115~123）映射到最接近的 MC 协议粒子类型，确保 Cubium 向原版 MC 客户端发送这些粒子时不会出现协议错误：

| 内部扩展粒子 | → 协议粒子 | 映射原因 |
|------------|-----------|---------|
| Breaking(115) | Block(1) | MC 中方块破坏由 Block 粒子承担 |
| Barrier(116) | Block(1) | 屏障显示使用方块粒子 |
| Light(117) | Block(1) | 结构方块标记使用方块粒子 |
| Redstone(118) | Dust(14) | MC 中红石粉尘由 Dust 粒子承担 |
| LargeExplosion(119) | HugeExplosion(22) | 大型爆炸映射为爆炸发射器 |
| ItemPickup(120) | Poof(57) | MC 中物品拾取使用消散效果 |
| DrippingCherryLeaves(121) | CherryLeaves(34) | 樱花树叶滴落映射为樱花树叶 |
| FallingCherryLeaves(122) | CherryLeaves(34) | 樱花树叶下落映射为樱花树叶 |
| LandingCherryLeaves(123) | CherryLeaves(34) | 樱花树叶落地映射为樱花树叶 |

## 容易踩的坑

- **协议 ID 一致性**：0~114 的枚举值必须与 MC Java 1.21.11 协议严格一致，不可随意修改。新增 MC 协议粒子时必须按注册顺序追加。
- **内部扩展粒子的网络通信**：115~123 的内部扩展粒子在序列化时会通过 `toProtocolId()` 映射为最接近的 MC 协议粒子类型。反序列化时通过 `fromProtocolId()` 转换，仅接受 0~114 的协议 ID。
- **命名差异**：部分 Cubium 内部扩展粒子的名称与 MC 不同（如 Cubium 用 `Breaking` 对应 MC 的 `block` 方块破坏场景，Cubium 用 `Redstone` 对应 MC 的 `dust` 红石场景），使用时注意区分。

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
- `common/network/ir/packets/play/PlayPacketsExtended.hpp` — `ir::play::LevelParticles` IR 包使用 `ParticleTypeId`（旧 `common/network/packet/ParticlePacket.hpp` 已删除）
- `common/entity/` — 实体类生成粒子
- `common/world/block/` — 方块 animateTick 生成粒子
- `client/renderer/trident/particle/` — 客户端粒子渲染系统
- `server/` — 服务端粒子广播

### 对应 MC Java 版

本模块对应 MC Java 版的 `net.minecraft.core.particles.ParticleType` 和 `ParticleTypes` 注册表。MC Java 版使用泛型注册表模式（`ParticleType<T extends ParticleOptions>`），本项目采用更简洁的枚举 ID + 数据类方案。

## 注意事项

1. **枚举值分配**：0~114 为 MC 协议 ID，不可修改。项目内部扩展粒子使用 115+，新增内部扩展粒子时需在 Count 之前追加。
2. **命名空间**：common 层使用 `mc::particle::ParticleTypeId`，客户端兼容层仍可使用 `mc::client::renderer::trident::particle::ParticleTypeId`
3. **前向声明**：如需前向声明，使用 `namespace mc::particle { enum class ParticleTypeId : u16; }`
4. **网络序列化**：`ir::play::LevelParticles`（旧 `ParticlePacket` 已删除）序列化时使用 `toProtocolId()` 将内部扩展粒子映射为协议 ID，反序列化时使用 `fromProtocolId()` 将协议 ID 转换为枚举值。
