# 杂项实体

本目录包含其他类别的实体和效果类。

## 目录结构

```
misc/
├── MiscEntities.hpp     # FallingBlockEntity、TNTEntity、WardenWarningEffect 定义
├── MiscEntities.cpp     # 实现文件
└── README.md            # 本文档
```

## 内容说明

| 类型 | 说明 |
|------|------|
| FallingBlockEntity | 下落方块实体（沙子、砾石、铁砧等） |
| TNTEntity | TNT实体（点燃的TNT，倒计时爆炸） |
| WardenWarningEffect | 寂守者警告效果（非实体，效果类） |

**注意**：
- `EyeOfEnderEntity` 和 `EvokerFangsEntity` 已移至 `projectile/OtherProjectiles.hpp`
- `ConduitEntity`（潮涌核心）是方块实体，位于 `world/blockentity/processing/ConduitEntity.hpp`

## 内部模块关系

```
MiscEntities.hpp
├── FallingBlockEntity 继承 Entity
│   ├── 依赖 FallingBlock（方块基类）的回调：onStartFalling/onEndFalling/onBroken
│   └── 依赖 ItemDropHelper 进行物品掉落
├── TNTEntity 继承 Entity
│   └── 依赖 IWorld::createExplosion() 进行爆炸
└── WardenWarningEffect（独立类，非实体）
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
- `ItemDropHelper` (`entity/utils/ItemDropHelper.hpp`)
- `BlockItemRegistry` (`item/items/block/BlockItemRegistry.hpp`)
- `Explosion` 系统 (`world/explosion/`)
- `DamageSource` (`entity/damage/DamageSource.hpp`)
- `ParticleTypes` (客户端粒子)

## 容易踩的坑

1. **EyeOfEnderEntity 和 EvokerFangsEntity 的位置**：这两个实体在 `projectile/OtherProjectiles.hpp` 中实现，不在本目录，因为它们的行为更接近投掷物。

2. **ConduitEntity 不是实体**：潮涌核心是方块实体（BlockEntity），实现位于 `world/blockentity/processing/ConduitEntity.hpp`，不要在本目录查找。

3. **WardenWarningEffect 非实体**：这是一个纯效果类，不继承 Entity，用于寂守者的警告等级管理。

4. **FallingBlockEntity 的伤害类型**：铁砧使用 `DamageType::Anvil`，其他下落方块使用 `DamageType::FallingBlock`，在 `_hurtEntities()` 中根据方块ID判断。

5. **FallingBlockEntity 的铁砧损坏机制**：铁砧下落伤害实体时，有概率降级（anvil → chipped_anvil → damaged_anvil → 摧毁）。概率公式: `0.05 + ceil(fallDistance - 1) * 0.05`。降级由 `AnvilBlock::damageAnvil()` 处理，保留朝向属性。完全摧毁时 `m_cancelDrop=true`，不掉落物品。

5. **TNTEntity 爆炸模式**：TNT 爆炸使用 `ExplosionMode::Break`（破坏方块但不掉落物品），与苦力怕的 `ExplosionMode::Destroy` 不同。

6. **FallingBlockEntity 放置条件**：只有当下方方块不可穿透且目标位置可替换时才能放置，否则掉落物品。检查 `FallingBlock::canFallThrough()` 理解穿透判定。

7. **MAX_FALL_TIME 超时处理**：FallingBlockEntity 下落超过 600 tick（30秒）后会强制放置，防止永久下落。
