# Item 核心模块

本目录包含物品系统的核心类型和接口。

## 目录结构

```
core/
├── AdventureModePredicate.hpp/cpp  # 冒险模式谓词（CanPlaceOn/CanDestroy方块匹配）
├── Item.hpp/cpp              # 物品基类，所有物品类型的父类（含 onCraftedBy/onCraftedPostProcess 合成回调）
├── ItemStack.hpp/cpp         # 物品堆，表示游戏中的一个物品实例（包含物品类型、数量、耐久、附魔、冒险模式谓词和结构化自定义标签，含 onCraftedBy 桥接方法）
├── ItemRegistry.hpp/cpp      # 物品注册表，管理所有物品的注册和查找
├── ItemGroup.hpp/cpp         # 创造模式物品组（标签页）
├── UseAction.hpp             # 物品使用动作枚举（EAT、DRINK、BLOCK等）
├── ActionResult.hpp          # 物品使用结果枚举
└── README.md
```

## 内部模块关系

```
Item (抽象基类)
  │
  ├── ItemProperties (构建器模式配置Item属性)
  │
  ├── ItemStack (物品实例，持有Item引用)
  │     ├── 附魔数据 (EnchantmentContainer)
  │     ├── 冒险模式谓词 (AdventureModePredicate × 2: CanPlaceOn + CanDestroy)
  │     └── 自定义标签 (结构化NBT)
  │
  ├── AdventureModePredicate (冒险模式方块匹配谓词，支持精确ID和#标签引用)
  │
  ├── ItemRegistry (单例，管理Item注册)
  │
  └── ItemGroup (创造模式物品分组)
```

## 上下游外部依赖关系

**本目录依赖：**
- `common/core/Types.hpp` - 基础类型定义
- `common/resource/ResourceLocation.hpp` - 资源位置标识
- `common/network/packet/PacketSerializer.hpp` - ItemStack网络序列化
- `item/enchantment/EnchantmentContainer.hpp` - 附魔容器

**依赖本目录的模块：**
- `item/items/` - 所有具体物品实现（FoodItem、ArmorItem、ToolItem等）
- `item/food/` - 食物属性系统
- `item/armor/` - 盔甲材质系统
- `item/crafting/` - 合成系统（Ingredient匹配）
- `entity/` - 实体背包、物品掉落
- `world/` - 方块掉落、物品实体
- `network/` - 物品数据包同步
- `client/inventory/` - 客户端背包UI

## 容易踩的坑

### 1. Item类是抽象基类

不应直接实例化Item类，应通过ItemRegistry注册具体的物品子类。

### 2. ItemStack支持结构化自定义标签

ItemStack现在支持结构化自定义标签（如染色的`display.color`），JSON序列化会通过`Tag`字段保存这些数据。比较两个ItemStack是否可堆叠时，需要确保自定义数据一致。

### 3. ItemRegistry是单例

在游戏初始化时注册所有物品，注册顺序需注意依赖关系（如BlockItem需要Block先注册）。

### 4. 附魔物品堆叠逻辑

MC 1.16.5中，附魔物品堆叠是基于NBT标签完全相等判断的。如果两个物品有相同的附魔，它们可以堆叠。`canMergeWith()`方法会比较：物品类型、耐久度、修复成本、自定义名称、Lore描述、药水ID、附魔、自定义数据。

### 5. 属性修饰符与槽位

部分物品（如盔甲）的属性修饰符与装备槽位相关，调用`getAttributeModifiers(i32 slot)`时需要传入正确的槽位索引。

### 6. Item::inventoryTick与ItemStack::inventoryTick

`ItemStack::inventoryTick()`委托给`Item::inventoryTick()`，若需要实现物品在背包中的tick逻辑（如地图、时钟），应重写Item的虚方法而非ItemStack。

### 7. 合成回调 onCraftedBy/onCraftedPostProcess

物品合成后需要执行特殊后处理时，重写 `Item::onCraftedPostProcess(ItemStack&, IWorld&)`。`Item::onCraftedBy(ItemStack&, IWorld&, Player&)` 由 `ItemStack::onCraftedBy()` 调用，默认转发给 `onCraftedPostProcess`。调用链：`ServerPlayer::onItemCrafted()` → `ItemStack::onCraftedBy()` → `Item::onCraftedBy()` → `Item::onCraftedPostProcess()`。

对于地图物品，`FilledMapItem` 重写了 `onCraftedPostProcess` 处理 `map_scale_direction`（缩放）和 `map_lock`（锁定）NBT 标签，处理完毕后移除标签。

### 8. 冒险模式谓词 CanPlaceOn/CanDestroy

`AdventureModePredicate` 存储在 `ItemStack` 的 `m_canPlaceOn`/`m_canDestroy` 成员中，NBT 键名为 `CanPlaceOn`/`CanDestroy`（字符串列表）。冒险模式下：
- `Player::mayInteract()` 检查手持物品的 CanPlaceOn 标签，匹配目标方块时允许交互
- `BlockInteractionManager::handleBlockPlacement()` 检查 CanPlaceOn 标签
- `BlockInteractionManager::_canBreakBlock()` 检查 CanDestroy 标签
- 谓词条目支持精确方块ID（`minecraft:stone`）和标签引用（`#minecraft:logs`）
- 空谓词列表不匹配任何方块，冒险模式下无 CanPlaceOn/CanDestroy 标签的物品不能放置/破坏方块
