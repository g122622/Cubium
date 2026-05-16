# 触发器条件谓词 (Trigger Conditions)

本目录包含进度触发器使用的条件谓词类，用于匹配特定的游戏状态。

## 目录结构树

```
conditions/
├── BlockPredicate.hpp/cpp           # 方块谓词 - 匹配方块状态、标签、属性
├── EntityPredicate.hpp/cpp          # 实体谓词 - 匹配实体类型、距离、位置、效果、NBT、标志、装备；包含 DamageSourcePredicate
├── EntityFlagsPredicate.hpp/cpp     # 实体标志谓词 - 匹配燃烧、潜行、疾跑、游泳、幼年状态
├── EntityEquipmentPredicate.hpp/cpp # 装备谓词 - 匹配实体装备（头盔、胸甲、护腿、靴子、主手、副手）
├── NBTPredicate.hpp/cpp             # NBT谓词 - 匹配NBT数据
├── FluidPredicate.hpp/cpp           # 流体谓词 - 匹配流体类型（定义在 BlockPredicate.hpp）
├── ItemPredicate.hpp/cpp            # 物品谓词 - 匹配物品ID、数量、耐久、药水等
├── LocationPredicate.hpp/cpp        # 位置谓词 - 匹配坐标、维度、生物群系等；包含 DistancePredicate
└── MobEffectsPredicate.hpp/cpp      # 效果谓词 - 匹配实体身上的效果状态
```

## 文件介绍

### MobEffectsPredicate.hpp/cpp

效果谓词，用于检查实体身上的效果状态。

**主要类：**

#### EffectInstancePredicate
匹配单个效果实例的条件，检查：
- `amplifier` - 效果等级范围（0 = I级，1 = II级，以此类推）
- `duration` - 持续时间范围（tick）
- `ambient` - 是否为环境效果（如信标）
- `visible` - 是否显示粒子

```cpp
// 创建效果实例谓词
IntBounds amplifier = IntBounds::between(0, 2);  // 等级 I-III
IntBounds duration = IntBounds::atLeast(100);     // 至少 100 tick
EffectInstancePredicate predicate(amplifier, duration, false, std::nullopt);

// 检查效果实例
const EffectInstance* effect = entity.getEffect(EffectType::Speed);
bool matches = predicate.test(effect);
```

#### MobEffectsPredicate
检查实体身上的效果状态组合。

```cpp
// 从 JSON 解析
nlohmann::json json = R"({
    "minecraft:speed": { "amplifier": { "min": 1 } },
    "minecraft:regeneration": {}
})"_json;

auto result = MobEffectsPredicate::fromJson(json);
if (result.success()) {
    MobEffectsPredicate predicate = result.value();
    
    // 检查实体
    bool matches = predicate.test(livingEntity);
}
```

**JSON 格式示例：**
```json
{
  "minecraft:speed": {
    "amplifier": { "min": 0, "max": 2 },
    "duration": { "min": 100 },
    "ambient": false,
    "visible": true
  },
  "minecraft:regeneration": {}
}
```

### EntityPredicate.hpp/cpp

实体谓词，用于匹配实体的条件。参考 MC 1.16.5: `net.minecraft.advancements.criterion.EntityPredicate`

**主要字段：**

| 字段 | JSON 键 | 说明 |
|------|---------|------|
| `m_type` | `type` | 实体类型（如 `minecraft:zombie`） |
| `m_distance` | `distance` | 距离谓词（与参考点的距离） |
| `m_location` | `location` | 位置谓词（生物群系、维度等） |
| `m_effects` | `effects` | 效果谓词（MobEffectsPredicate） |
| `m_nbt` | `nbt` | NBT谓词 |
| `m_flags` | `flags` | 标志谓词（燃烧、潜行等） |
| `m_equipment` | `equipment` | 装备谓词 |

**检查方法：**
- `test(const Entity& entity)` - 基础检查（不含距离和位置）
- `test(const IWorld& world, f64 x, f64 y, f64 z, const Entity& entity)` - 完整检查（包含距离和位置）

```cpp
// 从 JSON 解析
nlohmann::json json = R"({
  "type": "minecraft:zombie",
  "distance": {"max": 30.0},
  "flags": {"is_baby": true},
  "equipment": {
    "head": {"item": "minecraft:diamond_helmet"}
  }
})"_json;

auto result = EntityPredicate::fromJson(json);
```

### EntityFlagsPredicate.hpp/cpp

实体标志谓词，用于匹配实体的状态标志。参考 MC 1.16.5: `net.minecraft.advancements.criterion.EntityFlagsPredicate`

**主要字段：**

| 字段 | JSON 键 | 说明 |
|------|---------|------|
| `m_isOnFire` | `is_on_fire` | 是否燃烧 |
| `m_isSneaking` | `is_sneaking` | 是否潜行 |
| `m_isSprinting` | `is_sprinting` | 是否疾跑（仅玩家） |
| `m_isSwimming` | `is_swimming` | 是否游泳（仅玩家） |
| `m_isBaby` | `is_baby` | 是否幼年 |

```cpp
// 从 JSON 解析
nlohmann::json json = R"({
  "is_on_fire": true,
  "is_baby": false
})"_json;

auto result = EntityFlagsPredicate::fromJson(json);
if (result.success()) {
    EntityFlagsPredicate predicate = result.value();
    bool matches = predicate.test(entity);
}
```

### EntityEquipmentPredicate.hpp/cpp

装备谓词，用于匹配实体的装备。参考 MC 1.16.5: `net.minecraft.advancements.criterion.EntityEquipmentPredicate`

**主要字段：**

| 字段 | JSON 键 | 说明 |
|------|---------|------|
| `m_head` | `head` | 头盔 |
| `m_chest` | `chest` | 胸甲 |
| `m_legs` | `legs` | 护腿 |
| `m_feet` | `feet` | 靴子 |
| `m_mainHand` | `mainhand` | 主手物品 |
| `m_offHand` | `offhand` | 副手物品 |

**注意**：只有 `LivingEntity` 才有装备，非 `LivingEntity` 对装备谓词返回 false（除非谓词为空）。

```cpp
// 从 JSON 解析
nlohmann::json json = R"({
  "head": {"item": "minecraft:diamond_helmet"},
  "mainhand": {"item": "minecraft:diamond_sword"}
})"_json;

auto result = EntityEquipmentPredicate::fromJson(json);
if (result.success()) {
    EntityEquipmentPredicate predicate = result.value();
    bool matches = predicate.test(livingEntity);
}
```

### NBTPredicate.hpp/cpp

NBT谓词，用于匹配实体或物品的NBT数据。参考 MC 1.16.5: `net.minecraft.advancements.criterion.NBTPredicate`

**主要功能：**
- 支持实体NBT匹配
- 支持物品NBT匹配
- 递归比较NBT标签（compound_tag、list_tag、数值标签等）
- 期望标签必须是实际标签的子集

```cpp
// 从 JSON 解析（Mojangson 格式）
nlohmann::json json = "{CustomName:'{\"text\":\"Test\"}'}";

auto result = NBTPredicate::fromJson(json);
if (result.success()) {
    NBTPredicate predicate = result.value();
    bool matches = predicate.test(entity);
}
```

**匹配规则：**
- 如果期望NBT为空（`isAny()`），匹配任意NBT
- 如果实际NBT为空，不匹配（除非期望也为空）
- 递归比较：期望标签中的所有字段必须在实际标签中存在且值相等
- 实际标签可以包含额外的字段

### DamageSourcePredicate (定义在 EntityPredicate.hpp)

伤害源谓词，用于匹配伤害来源的条件。参考 MC 1.16.5: `net.minecraft.advancements.criterion.DamageSourcePredicate`

**主要字段（8 个伤害标志）：**

| 字段 | JSON 键 | 说明 |
|------|---------|------|
| `isProjectile` | `is_projectile` | 是否为投射物伤害（箭矢、三叉戟等） |
| `isExplosion` | `is_explosion` | 是否为爆炸伤害（TNT、苦力怕等） |
| `isFire` | `is_fire` | 是否为火焰伤害（火焰、岩浆等） |
| `isMagic` | `is_magic` | 是否为魔法伤害（药水、凋零等） |
| `isLightning` | `is_lightning` | 是否为闪电伤害 |
| `bypassesArmor` | `bypasses_armor` | 是否绕过护甲（溺水、摔落等） |
| `bypassesInvulnerability` | `bypasses_invulnerability` | 是否绕过无敌模式（虚空伤害） |
| `bypassesMagic` | `bypasses_magic` | 是否绕过魔法保护（饥饿伤害） |

**实现方式：**
- `isProjectile()` - 调用 `DamageSource::isProjectile()` 虚方法
- `isExplosion()` - 调用 `DamageSource::isExplosion()` 虚方法
- `isFire()` - 调用 `DamageSource::isFire()` 虚方法
- `isMagic()` - 调用 `DamageSource::isMagic()` 虚方法
- `isLightning` - 直接检查 `source.type() == DamageType::LightningBolt`
- `bypassesArmor()` - 调用 `DamageSource::bypassesArmor()` 虚方法
- `bypassesInvulnerability` - 调用 `DamageSource::canDamageCreative()` 虚方法
- `bypassesMagic` - 调用 `DamageSource::isDamageAbsolute()` 虚方法

```cpp
// 从 JSON 解析
nlohmann::json json = R"({
  "is_fire": true,
  "bypasses_armor": false
})"_json;

auto result = DamageSourcePredicate::fromJson(json);
if (result.success()) {
    DamageSourcePredicate predicate = result.value();
    
    // 检查伤害源
    EnvironmentalDamage fire(DamageType::InFire);
    bool matches = predicate.test(fire);  // true
}
```

**JSON 格式示例：**
```json
{
  "is_projectile": true,
  "is_fire": false,
  "is_explosion": true,
  "bypasses_armor": true
}
```

### ItemPredicate.hpp/cpp

物品谓词，用于匹配物品堆。

**主要字段：**
- `item` - 物品ID
- `count` - 数量范围
- `durability` - 耐久范围
- `potion` - 药水类型

### BlockPredicate.hpp/cpp

方块谓词，用于匹配方块状态。

**主要字段：**
- `block` - 方块ID
- `state` - 状态属性
- `tag` - 方块标签
- `fluid` - 流体谓词

### LocationPredicate.hpp/cpp

位置谓词，用于匹配位置条件。参考 MC 1.16.5: `net.minecraft.advancements.criterion.LocationPredicate`

**主要字段：**
- `x`, `y`, `z` - 坐标范围（DoubleBounds）
- `biome` - 生物群系（ResourceLocation）
- `dimension` - 维度（ResourceLocation）

**维度检查实现：**
```cpp
// 维度 ID 映射到名称
// 0  -> minecraft:overworld
// -1 -> minecraft:the_nether
// 1  -> minecraft:the_end
bool matchesDimension(DimensionId dimensionId, const ResourceLocation& expected);
```

**生物群系检查实现：**
```cpp
// 通过 ChunkData::getBiomeAtBlock() 获取生物群系ID
// 通过 BiomeRegistry::instance().get() 获取生物群系定义
// 比较 biome.name() 与期望的生物群系路径
```

```cpp
// 从 JSON 解析
nlohmann::json json = R"({
  "dimension": "minecraft:the_nether",
  "biome": "minecraft:crimson_forest",
  "position": {
    "x": { "min": 100 },
    "y": { "min": 60, "max": 80 }
  }
})"_json;

auto result = LocationPredicate::fromJson(json);
```

## 模块关系

```
触发器条件谓词
├── 被触发器实例使用
│   ├── EffectsChangedTriggerInstance ── MobEffectsPredicate
│   ├── PlayerKilledEntityTriggerInstance ── EntityPredicate, DamageSourcePredicate
│   ├── InventoryChangedTriggerInstance ── ItemPredicate
│   └── PlacedBlockTriggerInstance ── BlockPredicate
└── 被其他谓词组合使用
    └── EntityPredicate ── MobEffectsPredicate, DamageSourcePredicate
```

## 使用示例

### 在触发器中使用

```cpp
// EffectsChangedTrigger 触发检测
void EffectsChangedTrigger::trigger(ServerPlayer& player) {
    for (auto& listener : m_listeners) {
        const auto& instance = listener.second->instance<EffectsChangedTriggerInstance>();
        if (instance.test(player)) {
            listener.second->grant();
        }
    }
}
```

### JSON 进度示例

```json
{
  "criteria": {
    "have_speed": {
      "trigger": "minecraft:effects_changed",
      "conditions": {
        "effects": {
          "minecraft:speed": {
            "amplifier": { "min": 1 },
            "duration": { "min": 200 }
          }
        }
      }
    }
  }
}
```

### 伤害源谓词示例

```json
{
  "criteria": {
    "killed_by_fire": {
      "trigger": "minecraft:entity_killed_player",
      "conditions": {
        "damage": {
          "is_fire": true
        }
      }
    }
  }
}
```

## 依赖项

- `common/advancement/MinMaxBounds.hpp` - 范围谓词
- `common/entity/effect/EffectType.hpp` - 效果类型枚举
- `common/entity/effect/EffectInstance.hpp` - 效果实例
- `common/entity/core/LivingEntity.hpp` - 生物实体（效果容器）
- `common/entity/damage/DamageSource.hpp` - 伤害源基类
- `common/world/IWorld.hpp` - 世界接口（维度检查）
- `common/world/biome/BiomeRegistry.hpp` - 生物群系注册表
- `common/world/chunk/ChunkData.hpp` - 区块数据（生物群系查询）
- `nlohmann/json.hpp` - JSON 解析

## 容易踩的坑

1. **效果类型解析**：使用 `getEffectByResourceLocation()` 解析效果类型，未知效果会被跳过并输出警告
2. **实体类型检查**：只有 `LivingEntity` 有效果，非 `LivingEntity` 对效果谓词返回 false（除非谓词为空）
3. **JSON 格式**：效果ID 必须使用完整的 `minecraft:` 命名空间前缀
4. **伤害源标志**：`EnvironmentalDamage` 的 `isProjectile()` 和 `isExplosion()` 始终返回 false，投射物和爆炸伤害需要使用 `EntityDamageSource` 或 `IndirectEntityDamageSource`
5. **维度名称**：维度检查使用 ResourceLocation 路径部分（如 `overworld`），不是完整字符串

## 测试用例

相关测试文件：
- `tests/advancement/MobEffectsPredicateTest.cpp` - 效果谓词测试（19 个用例）
- `tests/advancement/DamageSourcePredicateTest.cpp` - 伤害源谓词测试（18 个用例）
- `tests/advancement/BlockPredicateTest.cpp` - 方块谓词测试
