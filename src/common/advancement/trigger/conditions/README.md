# 触发器条件谓词 (Trigger Conditions)

本目录包含进度触发器使用的条件谓词类，用于匹配特定的游戏状态。

## 目录结构树

```
conditions/
├── BlockPredicate.hpp/cpp      # 方块谓词 - 匹配方块状态、标签、属性
├── EntityPredicate.hpp/cpp     # 实体谓词 - 匹配实体类型、效果等
├── FluidPredicate.hpp/cpp      # 流体谓词 - 匹配流体类型（定义在 BlockPredicate.hpp）
├── ItemPredicate.hpp/cpp       # 物品谓词 - 匹配物品ID、数量、耐久等
├── LocationPredicate.hpp/cpp   # 位置谓词 - 匹配坐标、维度、生物群系等
└── MobEffectsPredicate.hpp/cpp # 效果谓词 - 匹配实体身上的效果状态
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

实体谓词，用于匹配实体的条件。

**主要字段：**
- `type` - 实体类型（如 `minecraft:zombie`）
- `effects` - 效果谓词（MobEffectsPredicate）

```cpp
// 从 JSON 解析
nlohmann::json json = R"({
  "type": "minecraft:player",
  "effects": {
    "minecraft:speed": { "amplifier": 1 }
  }
})"_json;

auto result = EntityPredicate::fromJson(json);
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

位置谓词，用于匹配位置条件。

**主要字段：**
- `x`, `y`, `z` - 坐标范围
- `biome` - 生物群系
- `dimension` - 维度

## 模块关系

```
触发器条件谓词
├── 被触发器实例使用
│   ├── EffectsChangedTriggerInstance ── MobEffectsPredicate
│   ├── PlayerKilledEntityTriggerInstance ── EntityPredicate
│   ├── InventoryChangedTriggerInstance ── ItemPredicate
│   └── PlacedBlockTriggerInstance ── BlockPredicate
└── 被其他谓词组合使用
    └── EntityPredicate ── MobEffectsPredicate
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

## 依赖项

- `common/advancement/MinMaxBounds.hpp` - 范围谓词
- `common/entity/effect/EffectType.hpp` - 效果类型枚举
- `common/entity/effect/EffectInstance.hpp` - 效果实例
- `common/entity/core/LivingEntity.hpp` - 生物实体（效果容器）
- `nlohmann/json.hpp` - JSON 解析

## 容易踩的坑

1. **效果类型解析**：使用 `getEffectByResourceLocation()` 解析效果类型，未知效果会被跳过并输出警告
2. **实体类型检查**：只有 `LivingEntity` 有效果，非 `LivingEntity` 对效果谓词返回 false（除非谓词为空）
3. **JSON 格式**：效果ID 必须使用完整的 `minecraft:` 命名空间前缀

## 测试用例

相关测试文件：`tests/advancement/MobEffectsPredicateTest.cpp`

- EffectInstancePredicate 测试（12 个用例）
- MobEffectsPredicate 测试（5 个用例）
- 集成测试（2 个用例）
