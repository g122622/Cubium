# Item Tag 模块

## 目录结构

```
tag/
├── ItemTag.hpp       # 物品标签类定义
├── ItemTag.cpp       # 物品标签实现
├── ItemTags.hpp      # 物品标签注册表定义
├── ItemTags.cpp      # 物品标签注册表实现
└── README.md         # 本文件
```

## 模块职责

物品标签系统提供物品分组功能，用于配方、功能判断和游戏逻辑。

### ItemTag 类

单个物品标签，包含一组物品的集合。

**核心方法**：
- `add(const Item* item)` - 添加物品到标签
- `contains(const Item* item)` - 检查物品是否在标签中
- `contains(const ItemStack& stack)` - 检查物品堆是否在标签中
- `getItems()` - 获取标签中所有物品
- `getId()` - 获取标签资源位置

### ItemTags 类

内置物品标签注册表，管理所有预定义标签。

**内置标签**：
| 标签 | 说明 |
|------|------|
| `FLOWERS` | 所有花朵物品，用于蜜蜂繁殖和授粉 |
| `CARPETS` | 所有颜色地毯物品，用于羊驼装饰槽位判断 |

**核心方法**：
- `FLOWERS()` - 获取花朵标签
- `CARPETS()` - 获取地毯标签
- `initialize()` - 初始化所有内置标签
- `registerTag(id)` - 注册新标签
- `getTag(id)` - 根据ID获取标签

## 使用方法

### 检查物品是否在标签中

```cpp
#include "item/tag/ItemTags.hpp"
#include "item/core/Item.hpp"

// 方式一：通过 Item::isIn()
if (item->isIn(item::tag::ItemTags::FLOWERS())) {
    // 物品是花朵
}

// 方式二：通过 ItemTag::contains()
if (item::tag::ItemTags::FLOWERS().contains(item)) {
    // 物品是花朵
}

// 方式三：检查 ItemStack
if (item::tag::ItemTags::FLOWERS().contains(stack)) {
    // 物品堆是花朵
}
```

### 实体繁殖物品检查

```cpp
bool BeeEntity::isBreedingItem(const ItemStack& itemStack) const
{
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item->isIn(item::tag::ItemTags::FLOWERS());
}
```

## FLOWERS 标签内容

`ItemTags::FLOWERS()` 包含以下物品：

**小型花朵（13种）**：
- 蒲公英 (dandelion)
- 虞美人 (poppy)
- 兰花 (blue_orchid)
- 绒球葱 (allium)
- 蓝花美耳草 (azure_bluet)
- 红色郁金香 (red_tulip)
- 橙色郁金香 (orange_tulip)
- 白色郁金香 (white_tulip)
- 粉色郁金香 (pink_tulip)
- 滨菊 (oxeye_daisy)
- 铃兰 (lily_of_the_valley)
- 矢车菊 (cornflower)
- 凋零玫瑰 (wither_rose)

**大型花朵（4种）**：
- 向日葵 (sunflower)
- 丁香 (lilac)
- 玫瑰丛 (rose_bush)
- 牡丹 (peony)

## CARPETS 标签内容

`ItemTags::CARPETS()` 包含以下物品，用于羊驼装饰槽位判断：

**16 色地毯**：
- 白色地毯 (white_carpet)
- 橙色地毯 (orange_carpet)
- 品红地毯 (magenta_carpet)
- 淡蓝色地毯 (light_blue_carpet)
- 黄色地毯 (yellow_carpet)
- 黄绿色地毯 (lime_carpet)
- 粉色地毯 (pink_carpet)
- 灰色地毯 (gray_carpet)
- 淡灰色地毯 (light_gray_carpet)
- 青色地毯 (cyan_carpet)
- 紫色地毯 (purple_carpet)
- 蓝色地毯 (blue_carpet)
- 棕色地毯 (brown_carpet)
- 绿色地毯 (green_carpet)
- 红色地毯 (red_carpet)
- 黑色地毯 (black_carpet)

**使用示例**：

```cpp
// 在 LlamaEntity 中判断是否为有效装饰（地毯）
bool LlamaEntity::isValidArmorForSlot(const ItemStack& item) const
{
    const Item* itemPtr = item.getItem();
    if (itemPtr == nullptr) {
        return false;
    }
    return itemPtr->isIn(item::tag::ItemTags::CARPETS());
}
```

## 初始化顺序

`ItemTags::initialize()` 必须在以下初始化之后调用：

1. `VanillaBlocks::initialize()` - 方块注册
2. `Items::initialize()` - 物品注册
3. `BlockItemRegistry::instance().initializeVanillaBlockItems()` - 方块物品注册

**客户端初始化**：`ClientApplication::initializeCoreRegistries()`
**服务器初始化**：`MinecraftServer::initializeRegistries()`

## 参考

- MC 1.16.5 `net.minecraft.tags.ItemTags`
- MC 1.16.5 `net.minecraft.item.Item#isIn(Tag<Item>)`
- 项目目录：`src/common/world/block/BlockTags.hpp`（方块标签参考实现）

## 测试文件

- `tests/common/item/tag/ItemTagsTest.cpp` - FLOWERS 标签测试
