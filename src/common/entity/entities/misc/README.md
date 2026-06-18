# 杂项实体

本目录包含其他类别的实体和效果类。

## 目录结构

```
misc/
├── MiscEntities.hpp            # FallingBlockEntity、TNTEntity、WardenWarningEffect 定义
├── MiscEntities.cpp            # 实现文件
├── OminousItemSpawnerEntity.hpp # 不祥物品生成器实体
├── OminousItemSpawnerEntity.cpp # 不祥物品生成器实现
└── README.md                   # 本文档
```

## 内容说明

| 类型 | 说明 |
|------|------|
| FallingBlockEntity | 下落方块实体（沙子、砾石、铁砧、混凝土粉末等） |
| TNTEntity | TNT实体（点燃的TNT，倒计时爆炸） |
| WardenWarningEffect | 寂守者警告效果（非实体，效果类） |
| OminousItemSpawnerEntity | 不祥物品生成器实体 |

**注意**：
- `EyeOfEnderEntity` 和 `EvokerFangsEntity` 已移至 `projectile/OtherProjectiles.hpp`
- `ConduitEntity`（潮涌核心）是方块实体，位于 `world/blockentity/processing/ConduitEntity.hpp`

## 内部模块关系

```
MiscEntities.hpp
├── FallingBlockEntity 继承 Entity
│   ├── 依赖 FallingBlock（方块基类）的回调：onStartFalling/onEndFalling/onBroken
│   ├── 依赖 ConcretePowderBlock 的 getConcreteBlock() 进行下落遇水固化
│   ├── 依赖 AnvilBlock::damageAnvil() 进行铁砧降级
│   ├── 依赖 ItemDropHelper 进行物品掉落
│   ├── 依赖 BlockStateProperties::WATERLOGGED 进行水浸透处理
│   └── 依赖 VanillaBlocks::MOVING_PISTON 进行活塞移动检查
├── TNTEntity 继承 Entity
│   └── 依赖 IWorld::createExplosion() 进行爆炸
├── WardenWarningEffect（独立类，非实体）
└── OminousItemSpawnerEntity 继承 Entity
```

## 上下游外部依赖关系

### 依赖方（谁依赖了这个目录）
- `VanillaEntities` - 注册 FallingBlockEntity、TNTEntity
- `FallingBlock` 方块基类 - 创建 FallingBlockEntity 实例
- `TNTBlock` - 创建 TNTEntity 实例
- `AnvilBlock` - 创建 FallingBlockEntity 并设置伤害标志

### 被依赖方（这个目录依赖了谁）
- `Entity` 基类 (`entity/core/Entity.hpp`)
- `IWorld` 接口 (`world/IWorld.hpp`)
- `FallingBlock` 方块基类 (`world/block/blocks/FallingBlock.hpp`)
- `ConcretePowderBlock` (`world/block/blocks/ConcretePowderBlock.hpp`) - 下落遇水固化
- `AnvilBlock` (`world/block/blocks/functional/AnvilBlock.hpp`) - 铁砧降级
- `ItemDropHelper` (`entity/utils/ItemDropHelper.hpp`)
- `BlockItemRegistry` (`item/items/block/BlockItemRegistry.hpp`)
- `Explosion` 系统 (`world/explosion/`)
- `DamageSource` (`entity/damage/DamageSource.hpp`)
- `ParticleTypes` (客户端粒子)
- `BlockStateProperties` (`util/property/Properties.hpp`) - 水浸透属性
- `VanillaBlocks` (`world/block/registry/VanillaBlocks.hpp`) - 活塞检查
- `FluidTags` (`world/fluid/FluidTags.hpp`) - 水流体标签

## 容易踩的坑

1. **EyeOfEnderEntity 和 EvokerFangsEntity 的位置**：这两个实体在 `projectile/OtherProjectiles.hpp` 中实现，不在本目录，因为它们的行为更接近投掷物。

2. **ConduitEntity 不是实体**：潮涌核心是方块实体（BlockEntity），实现位于 `world/blockentity/processing/ConduitEntity.hpp`，不要在本目录查找。

3. **WardenWarningEffect 非实体**：这是一个纯效果类，不继承 Entity，用于寂守者的警告等级管理。

4. **FallingBlockEntity 的伤害类型**：铁砧使用 `DamageType::Anvil`，其他下落方块使用 `DamageType::FallingBlock`，在 `_hurtEntities()` 中根据方块ID判断。

5. **FallingBlockEntity 的铁砧损坏机制**：铁砧下落伤害实体时，有概率降级（anvil → chipped_anvil → damaged_anvil → 摧毁）。概率公式: `0.05 + ceil(fallDistance - 1) * 0.05`。降级由 `AnvilBlock::damageAnvil()` 处理，保留朝向属性。完全摧毁时设置 `m_dontSetBlock=true` 和 `m_shouldDropItem=false`（而非 `m_cancelDrop`），以确保 `onBroken` 回调被触发（播放铁砧破碎音效）。

6. **TNTEntity 爆炸模式**：TNT 爆炸使用 `ExplosionMode::Break`（破坏方块但不掉落物品），与苦力怕的 `ExplosionMode::Destroy` 不同。

7. **FallingBlockEntity 落地放置逻辑（_tryPlaceBlock）**：落地时依次检查以下条件，任一不满足则放置失败并掉落物品：
   - 目标位置不是移动中的活塞（`VanillaBlocks::MOVING_PISTON`）
   - 下方方块不可穿透（`FallingBlock::canFallThrough()` 返回 false）
   - 目标位置可替换（`canBeReplaced()` 或 `!blocksMovement()`）
   - 方块可以在该位置放置（`Block::isValidPosition()`）
   - 水浸透处理：如果方块支持 `WATERLOGGED` 属性且目标位置有水源，设置 `waterlogged=true`

8. **FallingBlockEntity 对 ConcretePowderBlock 的特殊处理**：下落过程中，如果下落方块是混凝土粉末且当前位置有水，立即固化为对应颜色的混凝土。此逻辑在 `tick()` 中检测，优先于落地处理。

9. **FallingBlockEntity 铁砧完全摧毁的标志选择**：铁砧完全损坏时使用 `m_dontSetBlock=true`（而非 `m_cancelDrop`），因为 `m_cancelDrop` 会跳过 `onBroken` 回调，而铁砧需要 `onBroken` 来播放破碎音效。`m_dontSetBlock` 路径会调用 `onBroken` 但不放置方块也不掉落物品。

10. **MAX_FALL_TIME 超时处理**：FallingBlockEntity 下落超过 600 tick（30秒）后会强制放置，防止永久下落。
