# 效果实体

本目录包含非生物、非物品的效果类实体。

## 目录结构

```
effect/
├── EffectEntities.hpp     # 效果实体定义（末影水晶、闪电、区域效果云、盔甲架）
├── EffectEntities.cpp     # 效果实体实现
└── README.md              # 本文档
```

> **注意**: ExperienceOrbEntity（经验球）已移动到独立的 `orb/` 目录。

## 实体列表

| 实体 | 说明 |
|------|------|
| EnderCrystalEntity | 末影水晶，治愈末影龙、光束、爆炸 |
| LightningBoltEntity | 闪电，伤害实体、生成火焰 |
| AreaEffectCloudEntity | 区域效果云，滞留药水效果 |
| ArmorStandEntity | 盔甲架，展示盔甲、可摆姿势 |

## 内部模块关系

```
EffectEntities.hpp/cpp
├── EnderCrystalEntity
│   └── 依赖 EnderDragonEntity（治愈目标）
├── LightningBoltEntity
│   └── 依赖 DamageSource、FlintAndSteelItem（点燃）
├── AreaEffectCloudEntity
│   └── 依赖 EffectInstance、LivingEntity
└── ArmorStandEntity
    └── 独立实现，无特殊依赖
```

## 上下游外部依赖关系

**上游依赖（本目录依赖）**：
- `core/Entity.hpp` - 实体基类
- `core/LivingEntity.hpp` - 生物实体基类（效果应用）
- `effect/EffectInstance.hpp` - 药水效果实例
- `damage/DamageSource.hpp` - 伤害来源
- `world/IWorld.hpp` - 世界接口
- `boss/EnderDragonEntity.hpp` - 末影龙（治愈目标）

**下游依赖（依赖本目录）**：
- `monster/CreeperEntity.hpp` - 苦力怕爆炸时创建 AreaEffectCloudEntity
- `item/ThrowablePotionEntity.hpp` - 喷溅药水创建 AreaEffectCloudEntity
- `world/WeatherManager.hpp` - 雷暴天气创建 LightningBoltEntity
- `world/DragonFightManager.hpp` - 末地战斗管理器创建 EnderCrystalEntity

## 容易踩的坑

### 1. 末影水晶治愈末影龙

- `healDragon()` 需要在服务端执行，客户端调用无效
- 治愈冷却 `HEAL_COOLDOWN` 为 10 ticks，需要每 tick 检查
- 爆炸时使用 `ExplosionMode::Destroy` 会破坏方块并掉落物品

### 2. 闪电点燃方块

- 点燃方块需要检查游戏规则 `doFireTick`
- NORMAL/HARD 难度点燃 4 个额外方块，EASY/PEACEFUL 不点燃
- 客户端需要调用 `world->setTimeLightningFlash(2)` 实现天空闪烁

### 3. 区域效果云效果应用

- 瞬间效果（瞬间治疗、瞬间伤害、饱和）使用 `applyInstantEffect()` 并乘以 0.5
- 持续效果直接使用 `living->addEffect()`
- 需要检查 `canBeHitWithPotion()`，盔甲架返回 false
- 瞬间效果的伤害来源使用 `DamageSources::indirectMagic(this, owner)`，使伤害归属于 owner
- 当 owner 为 nullptr 时回退到 `DamageSources::magic()`

### 3.1 区域效果云 Owner 追踪

- 采用双重追踪模式（缓存指针 + UUID），owner 实体失效后通过 UUID 重新查找
- `getOwner()` 非const版本会执行懒加载查找，const版本直接返回缓存指针
- owner 为 nullptr 时，瞬间伤害使用 `DamageSources::magic()`；owner 非空时使用 `DamageSources::indirectMagic(this, owner)`
- NBT 键名：`OwnerUUIDMost` / `OwnerUUIDLeast`（i64 格式）

### 4. 盔甲架标记模式

- 标记模式（`marker = true`）下盔甲架无碰撞箱
- 重力默认开启，标记模式下自动禁用

### 5. 经验球已移动

- `ExperienceOrbEntity` 已移动到 `entities/orb/` 目录
- 不要在本目录引用经验球相关代码
