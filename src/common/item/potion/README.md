# 药水系统 (Potion System)

Minecraft 1.16.5 药水系统，包括药水类型定义、注册表、酿造配方和工具类。

## 目录结构

```
src/common/item/potion/
├── Potion.hpp/cpp           # 药水类型类（效果组合）
├── PotionType.hpp           # 药水ID枚举和前缀类型
├── PotionRegistry.hpp/cpp   # 药水注册表（单例，管理所有药水类型）
├── Potions.hpp/cpp          # 原版药水静态引用（便捷访问指针）
├── PotionBrewing.hpp/cpp    # 酿造配方管理（药水类型转换、容器转换）
├── PotionUtils.hpp/cpp      # 药水工具类（物品堆操作、颜色计算）
└── README.md
```

## 内部模块关系

```
PotionType (枚举)
     ↓
Potion (药水类型，包含效果列表)
     ↓
PotionRegistry (注册表，管理所有药水)
     ↓
Potions (静态引用，便捷访问原版药水)

PotionBrewing (酿造配方)
     ├── PotionMix (药水类型转换配方)
     └── ItemMix (容器转换配方：药水→喷溅药水→滞留药水)

PotionUtils (工具类)
     ├── 依赖 PotionRegistry
     └── 依赖 ItemStack NBT 操作
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 模块 | 用途 |
|------|------|
| `entity/effect/EffectInstance` | 药水效果实例 |
| `entity/effect/EffectType` | 效果类型枚举 |
| `item/core/ItemStack` | 物品堆（药水物品存储） |
| `item/crafting/Ingredient` | 酿造材料匹配 |
| `resource/ResourceLocation` | 资源位置 ID |
| `core/Types.hpp` | 基础类型（PotionId 枚举定义） |

### 下游依赖（依赖本模块的）

| 模块 | 用途 |
|------|------|
| `item/items/potion/` | 药水物品实现（PotionItem、SplashPotionItem、LingeringPotionItem、GlassBottleItem） |
| `item/Items.cpp` | 原版物品注册 |
| `world/blockentity/BrewingStandEntity` | 酿造台方块实体 |
| `entity/inventory/container/BrewingStandContainer` | 酿造台容器 |
| `world/block/CauldronBlock` | 炼药锅（装水瓶） |
| `world/block/dispense/DispenseItemBehaviorRegistry` | 发射器行为（喷溅药水） |
| `item/items/weapon/TippedArrowItem` | 药水箭 |
| `item/crafting/special/TippedArrowRecipe` | 药水箭配方 |
| `entity/entities/projectile/ProjectileItemEntity` | 投掷药水实体 |
| `entity/entities/monster/illager/WitchEntity` | 女巫（使用药水） |
| `advancement/trigger/conditions/ItemPredicate` | 物品谓词（药水匹配） |

## 容易踩的坑

### 1. PotionRegistry 指针稳定性

**问题**：`PotionRegistry` 使用 `unique_ptr` 存储药水对象，确保指针稳定。但不要存储裸指针到本地变量长期持有，注册新药水可能改变内部结构。

**解决方案**：使用 `Potions::XXX` 静态引用或每次从注册表获取。

### 2. PotionUtils 颜色计算

**问题**：药水颜色有两种来源——药水效果计算的颜色和 `CustomPotionColor` 标签设置的自定义颜色。

**解决方案**：`PotionUtils::getColor(stack)` 会优先使用自定义颜色，没有自定义颜色才计算效果颜色平均值。

### 3. PotionBrewing 初始化顺序

**问题**：`PotionBrewing::initialize()` 必须在 `Potions::initialize()` 之后调用，否则配方中的药水指针为空。

**解决方案**：确保初始化顺序为 `Potions::initialize()` → `PotionBrewing::initialize()`。

### 4. 酿造配方中的 Ingredient 依赖

**问题**：`PotionBrewing` 的配方使用 `Ingredient` 匹配材料，依赖物品注册完成。

**解决方案**：酿造配方注册必须在 `Items::initialize()` 之后。

### 5. 自定义效果的合并规则

**问题**：`PotionUtils::addCustomEffect()` 添加效果时会合并同类型效果（取较强者），不会简单叠加。

**解决方案**：如需强制覆盖，先用 `removeCustomEffects()` 清除再添加，或使用 `setCustomEffects()` 替换全部。

### 6. 瞬间药水的特殊处理

**问题**：瞬间治疗、瞬间伤害等瞬间药水的 `hasInstantEffect()` 返回 true，喷溅药水和滞留药水的处理逻辑不同。

**解决方案**：`PotionEntity` 需要根据 `hasInstantEffect()` 区分瞬间效果和持续效果的应用方式。

### 7. 玻璃瓶装水检测

**问题**：`GlassBottleItem` 需要检测水源方块和装水的炼药锅，液体方块没有碰撞形状。

**解决方案**：通过流体状态检测（`Fluids::WATER` 且 `level == 0`），而非碰撞检测。详见 `items/potion/README.md`。
