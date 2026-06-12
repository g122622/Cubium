# 特殊物品模块 (Special Items)

特殊物品模块提供功能性物品的实现。

## 目录结构

```
special/
├── README.md                # 本文档
├── BoneMealItem.cpp/hpp     # 骨粉（加速植物生长、海草生成）
├── BucketItem.cpp/hpp       # 桶（空桶、水桶、岩浆桶）
├── EnchantedBookItem.cpp/hpp # 附魔书（存储附魔）
├── FishBucketItem.cpp/hpp   # 鱼桶（放置水并生成鱼）
├── FlintAndSteelItem.cpp/hpp # 打火石（点火、点燃下界传送门）
├── MilkBucketItem.cpp/hpp   # 牛奶桶（清除药水效果）
├── NameTagItem.cpp/hpp      # 命名牌（给生物命名、持久化）
├── OnAStickItem.cpp/hpp     # 钓竿类物品基类（控制可骑乘实体）
├── SaddleItem.cpp/hpp       # 鞍（装备可骑乘实体）
├── SpawnEggItem.cpp/hpp     # 生成蛋（右键生成实体）
├── StickItems.cpp/hpp       # 具体钓竿物品（胡萝卜钓竿、诡异菌钓竿）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                     特殊物品模块                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐    继承    ┌──────────────┐               │
│  │ OnAStickItem │◄──────────│CarrotOnAStick│               │
│  │   (基类)     │           │    Item      │               │
│  └──────┬──────┘           └──────────────┘               │
│         │                  ┌───────────────┐               │
│         └──────────────────│WarpedFungus   │               │
│            继承            │OnAStickItem   │               │
│                           └───────────────┘               │
│                                                             │
│  OnAStickItem 控制 IRideable 实体（猪、炽足兽）              │
│  SaddleItem 装备 IEquipable 实体（猪、炽足兽、马等）          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

**内部依赖（本模块依赖）：**
- `Item` 基类、`ItemStack`、`ItemActionResult`（物品系统核心）
- `IRideable`、`IEquipable` 接口（实体交互）
- `IGrowable` 接口（骨粉与植物交互）
- `Fluid`、`FluidState`（桶与流体交互）
- `EntityType`（生成蛋、鱼桶创建实体）
- `enchant::Enchantment`（附魔书存储附魔）

**外部依赖（谁依赖本模块）：**
- `Items` 静态注册表（物品注册）
- `Player` 玩家实体（物品使用）
- 实体模块（猪、炽足兽、牛等实体与物品交互）

## 容易踩的坑

1. **OnAStickItem 耐久度消耗顺序**：MC 1.16.5 中，先触发 `IRideable::boost()` 加速，再消耗耐久度
2. **钓鱼竿转换**：耐久度耗尽后转换为钓鱼竿，需确保 `Items::FISHING_ROD` 已注册
3. **实体类型匹配**：OnAStickItem 使用字符串 ID 匹配（如 `"minecraft:pig"`），需与实体注册 ID 一致
4. **canBeSteered 条件**：需同时满足：有鞍 + 有乘客 + 玩家手持正确钓竿
5. **IRideable::boost() 返回值**：加速可能失败（已在加速中或没有鞍），需检查返回值
6. **BoneMealItem 水下使用**：需检查目标位置是否为完整水源方块（流体等级 == 8）
7. **SpawnEggItem 实体类型不可拷贝**：`EntityType` 的拷贝构造函数是 deleted 的，`getEntityType()` 返回 `const EntityType&`（引用），构造函数参数按值传递后需用 `std::move` 初始化成员
8. **SpawnEggItem 命名空间**：`spawnEntity()` 方法中的 `SpawnReason` 属于 `world::spawn` 命名空间，非 `entity` 命名空间
