# 实体类型标签模块

实体类型标签（EntityTypeTag）用于将实体类型分组以便功能判断。

## 架构

```
entity/tag/
├── EntityTypeTag.hpp       # 实体类型标签类
├── EntityTypeTag.cpp       # 实体类型标签实现
├── EntityTypeTags.hpp      # 内置标签集合
├── EntityTypeTags.cpp      # 标签注册和初始化
├── EntityTypeTagLoader.hpp # 数据包 JSON 加载器
├── EntityTypeTagLoader.cpp # 加载器实现
└── README.md               # 本文档
```

## 设计模式

EntityTypeTag 采用与 BlockTag/FluidTag 一致的设计模式：

- 使用 `std::unordered_set<ResourceLocation>` 存储实体类型资源位置（如 `minecraft:arrow`）
- `EntityTypeTags` 静态注册表管理所有内置标签
- `EntityTypeTagLoader` 从数据包 JSON 文件加载标签内容

## 数据包格式

数据包路径：`data/<namespace>/tags/entity_type/<path>.json`

```json
{
  "replace": false,
  "values": [
    "minecraft:arrow",
    "#minecraft:arrows",
    { "id": "minecraft:optional_entity", "required": false }
  ]
}
```

## 使用示例

```cpp
// 检查实体类型是否在标签中
if (entityType.isIn(EntityTypeTags::IMPACT_PROJECTILES())) {
    // 该实体类型属于冲击投射物
}

// 检查实体是否在标签中（通过 getTypeId()）
if (EntityTypeTags::IMPACT_PROJECTILES().contains(entity.getTypeId())) {
    // 该实体属于冲击投射物
}
```

## 已注册标签

- `IMPACT_PROJECTILES` — 冲击投射物（箭矢、三叉戟、火球等，可破坏陶罐等方块）
- `ARROWS` — 箭矢（普通箭矢、光灵箭）
- `REDIRECTABLE_PROJECTILE` — 可偏转投射物（火球、风弹等）
- `ARTHROPOD` — 节肢动物
- `UNDEAD` — 亡灵（包含 SKELETONS 和 ZOMBIES 子标签）
- `SKELETONS` — 骷髅类
- `ZOMBIES` — 僵尸类
- `AQUATIC` — 水生生物
- `SENSITIVE_TO_BANE_OF_ARTHROPODS` — 节肢杀手敏感（= ARTHROPOD）
- `SENSITIVE_TO_SMITE` — 亡灵杀手敏感（= UNDEAD）
- `SENSITIVE_TO_IMPALING` — 穿刺敏感（= AQUATIC）
- `ILLAGER` — 灾厄村民
- `RAIDERS` — 袭击者
- `BURN_IN_DAYLIGHT` — 白天燃烧
- `CAN_BREATHE_UNDER_WATER` — 可水下呼吸
- `FALL_DAMAGE_IMMUNE` — 摔落伤害免疫
- `FREEZE_IMMUNE_ENTITY_TYPES` — 冻结免疫
- `FREEZE_HURTS_EXTRA_TYPES` — 冻结额外伤害
- `BEEHIVE_INHABITORS` — 蜂巢居民
- `DEFLECTS_PROJECTILES` — 可偏转投射物的实体
- `IGNORES_POISON_AND_REGEN` — 忽略中毒和再生
- `INVERTED_HEALING_AND_HARM` — 治疗与伤害反转
- `IMMUNE_TO_INFESTED` — 免疫蠹虫效果
- `IMMUNE_TO_OOZING` — 免疫渗出效果
- `NO_ANGER_FROM_WIND_CHARGE` — 风弹不激怒
- `DISMOUNTS_UNDERWATER` — 水下强制下坐骑（马、猪、骆驼、蜘蛛、炽足兽等陆地骑乘实体，船不在其中）
- `POWDER_SNOW_WALKABLE_MOBS` — 细雪可行走
- `ACCEPTS_IRON_GOLEM_GIFT` — 接受铁傀儡礼物（铜傀儡），数据包加载时填充
- `CANDIDATE_FOR_IRON_GOLEM_GIFT` — 铁傀儡赠花候选（村民 + #accepts_iron_golem_gift），数据包加载时填充

## 安全检查

`EntityTypeTags::isInitialized()` 提供标签系统初始化状态查询，用于在标签未初始化时避免访问空数据：

- `Entity::canFreeze()` — 检查 `FREEZE_IMMUNE_ENTITY_TYPES` 前，先检查 `EntityTypeTags::isInitialized()`，未初始化时默认允许冰冻
- `LivingEntity::canFreeze()` — 检查 `ItemTags::FREEZE_IMMUNE_WEARABLES` 前，先检查 `ItemTags::isInitialized()`，未初始化时跳过皮革护甲检查
