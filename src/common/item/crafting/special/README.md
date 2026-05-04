# 特殊配方模块 (Special Recipes)

本模块实现了 Minecraft 1.16.5 的特殊合成配方，这些配方没有固定的合成图案，而是基于特定的物品组合逻辑。

## 目录结构

```
src/common/item/crafting/special/
├── SpecialRecipe.hpp        # 特殊配方基类
├── SpecialRecipe.cpp        # 特殊配方基类实现
├── RepairItemRecipe.hpp     # 物品修复配方
├── RepairItemRecipe.cpp     # 物品修复配方实现
├── ArmorDyeRecipe.hpp       # 盔甲染色配方
├── ArmorDyeRecipe.cpp       # 盔甲染色配方实现
├── BookCloningRecipe.hpp    # 书复制配方
├── BookCloningRecipe.cpp    # 书复制配方实现
├── MapCloningRecipe.hpp     # 地图复制配方
├── MapCloningRecipe.cpp     # 地图复制配方实现
└── README.md                # 本文档
```

## 配方说明

### SpecialRecipe（基类）

所有特殊配方的基类，继承自 `IRecipe<CraftingInventory>`。

**特点**：
- 无固定合成图案
- 动态匹配物品组合
- 结果物品可能带有 NBT 数据

### RepairItemRecipe（物品修复）

**功能**：在铁砧中修复可损坏物品。

**合成逻辑**：
- 两个相同类型的可损坏物品
- 结果物品耐久度 = 剩余耐久度之和 + 10%最大耐久度
- 结果物品继承两个输入中较高的附魔等级

### ArmorDyeRecipe（盔甲染色）

**功能**：为可染色的盔甲物品上色。

**支持的染料**（MC 1.16.5）：
- 墨囊 (INK_SAC) - 黑色
- 红色染料 (RED_DYE)
- 绿色染料 (GREEN_DYE)
- 可可豆 (COCOA_BEANS) - 棕色
- 青金石 (LAPIS_LAZULI_DYE) - 蓝色
- 紫色染料 (PURPLE_DYE)
- 青色染料 (CYAN_DYE)
- 淡灰色染料 (LIGHT_GRAY_DYE)
- 灰色染料 (GRAY_DYE)
- 粉红色染料 (PINK_DYE)
- 黄绿色染料 (LIME_DYE)
- 黄色染料 (YELLOW_DYE)
- 淡蓝色染料 (LIGHT_BLUE_DYE)
- 品红色染料 (MAGENTA_DYE)
- 橙色染料 (ORANGE_DYE)
- 白色染料 (WHITE_DYE)

**合成逻辑**：
- 1 个可染色盔甲 + 任意数量染料
- 颜色混合使用 RGB 平均算法
- 支持皮革盔甲、皮革马铠等

### BookCloningRecipe（书复制）

**功能**：复制成书（Written Book）。

**合成逻辑**：
- 1 本成书 + 任意数量书与笔
- 结果物品数量 = 书与笔数量
- 代数递增（原版最多复制到第二代）
- 保留原书的 NBT 数据（内容、作者、标题等）

**限制**：
- 最大代数为 2（第三代无法再复制）
- 需要成书和书与笔两种物品

### MapCloningRecipe（地图复制）

**功能**：复制已填充的地图。

**状态**：TODO - 地图物品尚未完全实现

**计划功能**：
- 1 张已填充地图 + 任意数量空地图
- 结果物品数量 = 空地图数量 + 1（原地图保留）
- 复制的地图与原地图共享数据

## 使用方法

特殊配方通过 `RecipeManager` 注册，与普通配方一样使用：

```cpp
#include "item/crafting/special/ArmorDyeRecipe.hpp"

// 注册盔甲染色配方
auto recipe = std::make_unique<ArmorDyeRecipe>(
    ResourceLocation("minecraft", "armor_dye")
);
RecipeManager::instance().registerRecipe(std::move(recipe));
```

## 匹配逻辑

特殊配方的 `matches()` 方法实现自定义匹配逻辑：

```cpp
bool ArmorDyeRecipe::matches(const CraftingInventory& inventory) const override {
    int armorCount = 0;
    int dyeCount = 0;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) continue;

        if (isDyeableArmor(stack)) ++armorCount;
        else if (isDye(stack)) ++dyeCount;
        else return false;  // 有其他物品，不匹配
    }

    return armorCount == 1 && dyeCount >= 1;
}
```

## 容易踩的坑

1. **物品检测**：使用 `Items::XXX` 指针比较，而非字符串比较
2. **染料物品**：需要检查所有 16 种染料 + 墨囊 + 可可豆
3. **剩余物品**：使用 `getRemainingItems()` 返回不消耗的物品
4. **NBT 复制**：复制 NBT 数据时注意深拷贝

## 测试用例

测试文件位于 `tests/common/item/crafting/` 目录。

## 依赖项

| 模块 | 用途 |
|------|------|
| `item/Items.hpp` | 物品静态引用 |
| `item/core/ItemStack.hpp` | 物品堆操作 |
| `item/items/armor/DyeableArmorItem.hpp` | 可染色盔甲 |
| `entity/inventory/CraftingInventory.hpp` | 合成容器 |
