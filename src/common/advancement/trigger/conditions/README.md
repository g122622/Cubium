# 触发器条件谓词 (Trigger Conditions)

本目录包含进度触发器使用的条件谓词类，用于匹配特定的游戏状态。

## 目录结构树

```
conditions/
├── BlockPredicate.hpp/cpp           # 方块谓词 - 匹配方块状态、标签、属性；包含 FluidPredicate
├── EntityPredicate.hpp/cpp          # 实体谓词 - 匹配实体类型、距离、位置、效果、NBT、标志、装备；包含 DamageSourcePredicate
├── EntityFlagsPredicate.hpp/cpp     # 实体标志谓词 - 匹配燃烧、潜行、疾跑、游泳、幼年状态
├── EntityEquipmentPredicate.hpp/cpp # 装备谓词 - 匹配实体装备（头盔、胸甲、护腿、靴子、主手、副手）
├── NBTPredicate.hpp/cpp             # NBT谓词 - 匹配NBT数据（支持递归比较，期望标签须是实际标签子集）
├── ItemPredicate.hpp/cpp            # 物品谓词 - 匹配物品ID、数量、耐久、药水、附魔、NBT
├── LocationPredicate.hpp/cpp        # 位置谓词 - 匹配坐标、维度、生物群系；包含 DistancePredicate
└── MobEffectsPredicate.hpp/cpp      # 效果谓词 - 匹配实体身上的效果状态；包含 EffectInstancePredicate
```

## 内部模块关系

```
谓词组合关系：
EntityPredicate ─┬─→ MobEffectsPredicate    （效果检查）
                 ├─→ EntityFlagsPredicate    （标志检查）
                 ├─→ EntityEquipmentPredicate（装备检查）
                 ├─→ NBTPredicate            （NBT检查）
                 ├─→ LocationPredicate       （位置检查）
                 └─→ DistancePredicate       （距离检查，定义在 LocationPredicate.hpp）

LocationPredicate ─→ DistancePredicate       （距离范围检查）

BlockPredicate ─→ FluidPredicate             （流体检查，定义在 BlockPredicate.hpp）

MobEffectsPredicate ─→ EffectInstancePredicate（单个效果检查）
```

## 上下游外部依赖关系

**上游依赖（本目录依赖的模块）**：
- `common/advancement/MinMaxBounds.hpp` - 范围谓词（IntBounds、DoubleBounds 等）
- `common/entity/effect/EffectType.hpp` - 效果类型枚举
- `common/entity/effect/EffectInstance.hpp` - 效果实例
- `common/entity/core/Entity.hpp` - 实体基类
- `common/entity/core/LivingEntity.hpp` - 生物实体（效果容器、装备容器）
- `common/entity/damage/DamageSource.hpp` - 伤害源基类
- `common/world/IWorld.hpp` - 世界接口（维度检查）
- `common/world/biome/BiomeRegistry.hpp` - 生物群系注册表
- `common/world/chunk/ChunkData.hpp` - 区块数据（生物群系查询）
- `common/world/fluid/Fluid.hpp` - 流体定义
- `common/world/block/BlockState.hpp` - 方块状态
- `common/item/ItemStack.hpp` - 物品堆
- `common/resource/ResourceLocation.hpp` - 资源位置
- `common/util/nbt/` - NBT 标签系统
- `nlohmann/json.hpp` - JSON 解析

**下游依赖（依赖本目录的模块）**：
- `common/advancement/trigger/impl/*` - 各触发器实现（使用条件谓词进行检测）
- `common/item/loot/conditions/*` - 战利品条件（复用谓词进行条件判断）
- `server/advancement/` - 服务端成就系统

## 容易踩的坑

1. **效果类型解析**：使用 `getEffectByResourceLocation()` 解析效果类型，未知效果会被跳过并输出警告，不会导致解析失败

2. **实体类型检查**：只有 `LivingEntity` 有效果和装备，非 `LivingEntity` 对效果谓词和装备谓词返回 false（除非谓词为空即 `isAny()`）

3. **JSON 格式要求**：效果ID、方块ID、物品ID 等必须使用完整的 `minecraft:` 命名空间前缀

4. **伤害源标志实现差异**：`EnvironmentalDamage` 的 `isProjectile()` 和 `isExplosion()` 始终返回 false，投射物和爆炸伤害需要使用 `EntityDamageSource` 或 `IndirectEntityDamageSource`

5. **维度名称匹配**：维度检查使用 ResourceLocation 路径部分（如 `overworld`），不是完整字符串比较

6. **流体等效性**：`minecraft:water` 同时匹配水源和流动水，`minecraft:lava` 同时匹配岩浆源和流动岩浆，使用 `Fluid::isEquivalentTo()` 比较

7. **NBT 匹配规则**：期望标签必须是实际标签的子集，实际标签可以包含额外字段；空期望匹配任意 NBT

8. **DistancePredicate 位置**：定义在 `LocationPredicate.hpp` 中，不在单独文件

9. **FluidPredicate 位置**：定义在 `BlockPredicate.hpp` 中，不在单独文件

10. **DamageSourcePredicate 位置**：定义在 `EntityPredicate.hpp` 中，不在单独文件
