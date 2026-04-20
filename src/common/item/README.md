# Item 系统

Minecraft Reborn 物品系统完整实现。

## 目录结构

```
item/
├── core/                         # 核心类型
│   ├── Item.hpp/cpp              # 物品基类
│   ├── ItemStack.hpp/cpp         # 物品堆
│   ├── ItemRegistry.hpp/cpp      # 物品注册表
│   ├── ItemGroup.hpp/cpp         # 创造模式物品组
│   ├── UseAction.hpp             # 使用动作枚举
│   ├── ActionResult.hpp          # 动作结果
│   └── README.md
├── food/                         # 食物系统
│   ├── Food.hpp/cpp              # 食物属性
│   ├── Foods.hpp/cpp             # 原版食物定义
│   └── README.md
├── armor/                        # 盔甲系统
│   ├── ArmorMaterial.hpp/cpp     # 盔甲材质接口
│   └── README.md
├── tier/                         # 材质等级
│   ├── IItemTier.hpp             # 材质接口
│   ├── ItemTiers.hpp/cpp         # 原版材质
│   └── README.md
├── attribute/                    # 属性修饰符
│   ├── ItemAttributeModifiers.hpp/cpp
│   └── README.md
├── context/                      # 使用上下文
│   ├── ItemUseContext.hpp/cpp    # 物品使用上下文
│   ├── BlockItemUseContext.hpp/cpp
│   └── README.md
├── tag/                          # 物品标签
│   ├── ItemTag.hpp/cpp           # 物品标签类
│   └── README.md
├── items/                        # 具体物品实现
│   ├── food/                     # 食物物品
│   │   ├── FoodItem.hpp/cpp      # 食物基类
│   │   └── ...
│   ├── armor/                    # 盔甲物品
│   │   ├── ArmorItem.hpp/cpp     # 盔甲基类
│   │   ├── DyeableArmorItem.hpp/cpp
│   │   ├── ElytraItem.hpp/cpp    # 鞘翅
│   │   └── ...
│   ├── tool/                     # 工具物品
│   │   ├── ToolItem.hpp/cpp      # 工具基类
│   │   ├── TieredItem.hpp/cpp    # 层级物品
│   │   ├── PickaxeItem.hpp/cpp   # 镐
│   │   ├── AxeItem.hpp/cpp       # 斧
│   │   ├── ShovelItem.hpp/cpp    # 锹
│   │   ├── HoeItem.hpp/cpp       # 锄
│   │   ├── SwordItem.hpp/cpp     # 剑
│   │   └── ToolType.hpp/cpp      # 工具类型
│   └── block/                    # 方块物品
│       ├── BlockItem.hpp/cpp     # 方块物品基类
│       ├── BlockItemRegistry.hpp/cpp
│       └── ...
├── enchantment/                  # 附魔系统
│   ├── Enchantment.hpp/cpp       # 附魔基类
│   ├── EnchantmentContainer.hpp/cpp
│   ├── EnchantmentHelper.hpp/cpp
│   ├── EnchantmentRegistry.hpp/cpp
│   └── enchantments/             # 具体附魔
│       ├── FortuneEnchantment.hpp/cpp
│       ├── SilkTouchEnchantment.hpp/cpp
│       └── ...
├── crafting/                     # 合成系统
│   ├── IRecipe.hpp               # 配方接口
│   ├── Ingredient.hpp/cpp        # 原料匹配
│   ├── RecipeManager.hpp/cpp     # 配方管理
│   ├── ShapedRecipe.hpp/cpp      # 有序合成
│   ├── ShapelessRecipe.hpp/cpp   # 无序合成
│   ├── SmeltingRecipe.hpp/cpp    # 熔炼配方
│   └── ...
├── Items.hpp/cpp                 # 原版物品注册
└── README.md                     # 本文件
```

## 已实现模块

### 核心系统 (100%)

| 模块 | 文件 | 状态 |
|------|------|------|
| Item 基类 | core/Item.hpp | ✅ 完成 |
| ItemStack | core/ItemStack.hpp | ✅ 完成 |
| ItemRegistry | core/ItemRegistry.hpp | ✅ 完成 |
| ItemGroup | core/ItemGroup.hpp | ✅ 完成 |
| UseAction | core/UseAction.hpp | ✅ 完成 |
| ActionResult | core/ActionResult.hpp | ✅ 完成 |

### 食物系统 (80%)

| 模块 | 文件 | 状态 |
|------|------|------|
| Food | food/Food.hpp | ✅ 完成 |
| Foods | food/Foods.hpp | ✅ 完成 |
| FoodItem | items/food/FoodItem.hpp | ✅ 完成 |

### 盔甲系统 (90%)

| 模块 | 文件 | 状态 |
|------|------|------|
| ArmorMaterial | armor/ArmorMaterial.hpp | ✅ 完成 |
| ArmorMaterials | armor/ArmorMaterial.cpp | ✅ 完成 (7种材质) |
| ArmorItem | items/armor/ArmorItem.hpp | ✅ 完成 |
| DyeableArmorItem | items/armor/DyeableArmorItem.hpp | ✅ 完成 |
| ElytraItem | items/armor/ElytraItem.hpp | ✅ 完成 |

盔甲物品现在支持右键自动装备对应槽位；如果目标槽位已被占用，则保持原物品不变并返回透传结果。

### 工具系统 (85%)

| 模块 | 文件 | 状态 |
|------|------|------|
| IItemTier | tier/IItemTier.hpp | ✅ 完成 |
| ItemTiers | tier/ItemTiers.cpp | ✅ 完成 (6种材质) |
| ToolType | items/tool/ToolType.hpp | ✅ 完成 |
| TieredItem | items/tool/TieredItem.hpp | ✅ 完成 |
| ToolItem | items/tool/ToolItem.hpp | ✅ 完成 |
| PickaxeItem | items/tool/PickaxeItem.hpp | ✅ 完成 |
| AxeItem | items/tool/AxeItem.hpp | ✅ 完成 |
| ShovelItem | items/tool/ShovelItem.hpp | ✅ 完成 |
| HoeItem | items/tool/HoeItem.hpp | ✅ 完成 |
| SwordItem | items/tool/SwordItem.hpp | ✅ 完成 |

### 附魔系统 (10%)

| 模块 | 文件 | 状态 |
|------|------|------|
| Enchantment | enchantment/Enchantment.hpp | ✅ 完成 |
| EnchantmentContainer | enchantment/EnchantmentContainer.hpp | ✅ 完成 |
| EnchantmentHelper | enchantment/EnchantmentHelper.hpp | ✅ 完成 |
| FortuneEnchantment | enchantments/FortuneEnchantment.hpp | ✅ 完成 |
| SilkTouchEnchantment | enchantments/SilkTouchEnchantment.hpp | ✅ 完成 |
| 其他附魔 (32个) | - | ⏳ 待实现 |

### 合成系统 (50%)

| 模块 | 文件 | 状态 |
|------|------|------|
| IRecipe | crafting/IRecipe.hpp | ✅ 完成 |
| Ingredient | crafting/Ingredient.hpp | ✅ 完成 |
| ShapedRecipe | crafting/ShapedRecipe.hpp | ✅ 完成 |
| ShapelessRecipe | crafting/ShapelessRecipe.hpp | ✅ 完成 |
| SmeltingRecipe | crafting/SmeltingRecipe.hpp | ⚠️ 基础完成 |
| StonecuttingRecipe | - | ❌ 未实现 |
| SmithingRecipe | - | ❌ 未实现 |

## 待实现模块

### 优先级高

1. **武器物品** (items/weapon/)
   - BowItem (弓)
   - CrossbowItem (弩)
   - TridentItem (三叉戟)
   - ShieldItem (盾牌)
   - ArrowItem (箭矢)

2. **投掷物品** (items/throw/)
   - SnowballItem (雪球)
   - EggItem (鸡蛋)
   - EnderPearlItem (末影珍珠)
   - ExperienceBottleItem (附魔之瓶)

3. **核心附魔** (enchantment/enchantments/)
   - UnbreakingEnchantment (耐久)
   - EfficiencyEnchantment (效率)
   - SharpnessEnchantment (锋利)
   - ProtectionEnchantment (保护系列)

### 优先级中

4. **特殊物品** (items/special/)
   - SpawnEggItem (生成蛋)
   - CompassItem (指南针)
   - ClockItem (时钟)
   - FishingRodItem (钓鱼竿)
   - EnchantedBookItem (附魔书)

5. **桶类物品** (items/bucket/)
   - BucketItem (空桶)
   - WaterBucketItem (水桶)
   - LavaBucketItem (熔岩桶)
   - MilkBucketItem (牛奶桶)

6. **药水物品** (items/potion/)
   - PotionItem (药水)
   - SplashPotionItem (溅射药水)
   - LingeringPotionItem (滞留药水)

## 使用示例

### 注册物品

```cpp
// 注册简单物品
auto& stick = ItemRegistry::instance().registerItem(
    ResourceLocation("minecraft:stick"),
    ItemProperties().maxStackSize(64)
);

// 注册食物
auto& apple = ItemRegistry::instance().registerItem<FoodItem>(
    ResourceLocation("minecraft:apple"),
    &Foods::APPLE,
    ItemProperties().maxStackSize(64)
);

// 注册盔甲
auto& ironHelmet = ItemRegistry::instance().registerItem<ArmorItem>(
    ResourceLocation("minecraft:iron_helmet"),
    ArmorMaterials::IRON,
    ArmorSlot::Head,
    ItemProperties().maxDamage(ArmorMaterials::IRON.getDurability(ArmorSlot::Head))
);
```

### 创建物品堆

```cpp
ItemStack stack(item, 32);  // 32个物品
stack.setDamage(50);        // 设置耐久度
stack.addEnchantment("minecraft:sharpness", 5);  // 添加附魔
```

### 使用物品

```cpp
// 右键使用
ItemActionResult result = item.onItemRightClick(world, player, Hand::MainHand);
if (result.isSuccessOrConsume()) {
    // 物品使用成功
}

// 方块交互
ActionResultType action = item.onItemUse(context);
```

## 设计原则

1. **单一职责**: 每个子目录负责一个功能领域
2. **依赖倒置**: 具体物品实现依赖抽象接口
3. **开闭原则**: 新增物品类型无需修改核心代码
4. **命名空间**: 使用 `mc::item::*` 命名空间隔离

## 与 MC Java 1.16.5 的兼容性

本系统设计参考 Minecraft Java Edition 1.16.5：
- 物品ID保持一致
- 物品属性值（耐久、伤害等）保持一致
- 附魔ID和效果保持一致
- 合成配方格式兼容

## 测试文件

相关测试文件位于 `tests/common/item/` 目录。
