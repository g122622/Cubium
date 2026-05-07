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
│       ├── WallOrFloorItem.hpp/cpp # 墙壁/地板物品（告示牌、旗帜等）
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

**Item 基类新增方法 (MC 1.16.5 对齐)**:
- `hitEntity()` - 攻击实体时调用，用于工具耐久消耗
- `onBlockDestroyed()` - 破坏方块时调用，用于工具耐久消耗
- `isSuitableFor()` - 检查物品是否适合作为方块工具
- `fillItemGroup()` - 填充物品到创造模式物品组
- `isInGroup()` - 检查物品是否在指定物品组中
- `getRarity(const ItemStack&)` - 获取物品堆稀有度（考虑附魔）
- `isEnchantable(const ItemStack&)` - 检查物品是否可附魔
- `getIsRepairable(const ItemStack&, const ItemStack&)` - 检查修复材料兼容性
- `getAttributeModifiers(i32)` - 获取装备槽位的属性修饰符

### 食物系统 (85%)

| 模块 | 文件 | 状态 |
|------|------|------|
| Food | food/Food.hpp | ✅ 完成 |
| Foods | food/Foods.hpp | ✅ 完成 |
| FoodItem | items/food/FoodItem.hpp | ✅ 完成 |
| HoneyBottleItem | items/food/HoneyBottleItem.hpp | ✅ 完成 |
| ChorusFruitItem | items/food/ChorusFruitItem.hpp | ✅ 完成 |

**特殊食物物品**:
- **蜂蜜瓶 (HoneyBottleItem)**: 清除中毒效果，使用时间40 ticks，返回玻璃瓶
- **紫颂果 (ChorusFruitItem)**: 食用后随机传送，播放传送音效

### 盔甲系统 (100%)

| 模块 | 文件 | 状态 |
|------|------|------|
| ArmorMaterial | armor/ArmorMaterial.hpp | ✅ 完成 |
| ArmorMaterials | armor/ArmorMaterial.cpp | ✅ 完成 (7种材质: 皮革、锁链、铁、金、钻石、海龟、下界合金) |
| ArmorItem | items/armor/ArmorItem.hpp | ✅ 完成 |
| DyeableArmorItem | items/armor/DyeableArmorItem.hpp | ✅ 完成 |
| ElytraItem | items/armor/ElytraItem.hpp | ✅ 完成 |

**已注册盔甲物品**:
- 皮革盔甲 (头盔、胸甲、护腿、靴子) - 可染色
- 锁链盔甲 (头盔、胸甲、护腿、靴子)
- 铁盔甲 (头盔、胸甲、护腿、靴子)
- 金盔甲 (头盔、胸甲、护腿、靴子)
- 钻石盔甲 (头盔、胸甲、护腿、靴子)
- 海龟壳 (头盔)
- 下界合金盔甲 (头盔、胸甲、护腿、靴子)
- 鞘翅 (胸甲槽)

盔甲物品现在支持右键自动装备对应槽位；如果目标槽位已被占用，则保持原物品不变并返回透传结果。可染色盔甲通过 `ItemStack` 的结构化标签保存 `display.color`，因此序列化、复制和比较都会保留染色数据。

**盔甲属性修饰符 (MC 1.16.5 对齐)**:
- `getAttributeModifiers()` 返回护甲值、韧性、击退抗性修饰符
- `getIsRepairable()` 检查修复材料（皮革、铁锭、金锭、钻石、下界合金锭、鳞甲）

### 工具系统 (90%)

| 模块 | 文件 | 状态 |
|------|------|------|
| IItemTier | tier/IItemTier.hpp | ✅ 完成 |
| ItemTiers | tier/ItemTiers.cpp | ✅ 完成 (6种材质: 木、石、铁、金、钻石、下界合金) |
| ToolType | items/tool/ToolType.hpp | ✅ 完成 |
| TieredItem | items/tool/TieredItem.hpp | ✅ 完成 |
| ToolItem | items/tool/ToolItem.hpp | ✅ 完成 |
| PickaxeItem | items/tool/PickaxeItem.hpp | ✅ 完成 (含MC 1.16.5有效方块) |
| AxeItem | items/tool/AxeItem.hpp | ✅ 完成 (含原木去皮映射) |
| ShovelItem | items/tool/ShovelItem.hpp | ✅ 完成 (含营火熄灭、土径创建) |
| HoeItem | items/tool/HoeItem.hpp | ✅ 完成 |
| SwordItem | items/tool/SwordItem.hpp | ✅ 完成 |
| ShearsItem | items/tool/ShearsItem.hpp | ✅ 完成 (剪刀，可剪羊毛、高效破坏蜘蛛网/树叶/羊毛，支持实体交互) |

**已注册工具物品**:
- 木工具 (镐、斧、锹、锄、剑)
- 石工具 (镐、斧、锹、锄、剑)
- 铁工具 (镐、斧、锹、锄、剑)
- 金工具 (镐、斧、锹、锄、剑)
- 钻石工具 (镐、斧、锹、锄、剑)
- 下界合金工具 (镐、斧、锹、锄、剑)

**工具属性修饰符 (MC 1.16.5 对齐)**:
- `ToolItem::getAttributeModifiers()` 返回攻击伤害和攻击速度修饰符
- `SwordItem::getAttributeModifiers()` 返回武器特定的属性修饰符
- `TieredItem::getIsRepairable()` 检查修复材料（木板、圆石、铁锭、金锭、钻石、下界合金锭）

### 附魔系统 (15%)

| 模块 | 文件 | 状态 |
|------|------|------|
| Enchantment | enchantment/Enchantment.hpp | ✅ 完成 (含完整 EnchantmentType 枚举) |
| EnchantmentContainer | enchantment/EnchantmentContainer.hpp | ✅ 完成 |
| EnchantmentHelper | enchantment/EnchantmentHelper.hpp | ✅ 完成 (含附魔回调分发) |
| FortuneEnchantment | enchantments/FortuneEnchantment.hpp | ✅ 完成 |
| SilkTouchEnchantment | enchantments/SilkTouchEnchantment.hpp | ✅ 完成 |
| BaneOfArthropodsEnchantment | enchantments/weapon/BaneOfArthropodsEnchantment.cpp | ✅ 完成 (含缓慢效果回调) |
| ThornsEnchantment | enchantments/protection/ThornsEnchantment.cpp | ✅ 完成 (含反伤回调) |
| 其他附魔 (32个) | - | ✅ 完成基础实现 |

**附魔回调系统 (MC 1.16.5 对齐)**:
- `onEntityDamaged()` - 攻击目标时调用（节肢杀手施加缓慢）
- `onUserHurt()` - 受伤时调用（荆棘反伤）
- `EnchantmentHelper::applyArthropodEnchantmentDamage()` - 分发攻击回调
- `EnchantmentHelper::applyThornsEnchantments()` - 分发荆棘回调
- `LivingEntity::onAttackEntity()` - 攻击时触发附魔回调
- `LivingEntity::actuallyHurt()` - 受伤时触发荆棘回调

**EnchantmentType 枚举 (MC 1.16.5 对齐)**:
- Armor (全护甲), ArmorFeet (靴子), ArmorLegs (护腿), ArmorHead (头盔), ArmorChest (胸甲)
- Weapon (剑), Digger (挖掘工具), FishingRod (钓鱼竿), Breakable (可破坏物品)
- Bow (弓), Wearable (可穿戴), Crossbow (弩), Trident (三叉戟), Vanishable (可消失物品), All (所有物品)

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
   - TridentItem (三叉戟) - 激流冲刺待完善
   - ShieldItem (盾牌) - 格挡逻辑待完善
   - ArrowItem (箭矢)

2. **投掷物品** (items/throw/)
   - SnowballItem (雪球)
   - EggItem (鸡蛋)
   - EnderPearlItem (末影珍珠) - 冷却系统待实现
   - ExperienceBottleItem (附魔之瓶)

3. **核心附魔** (enchantment/enchantments/)
   - UnbreakingEnchantment (耐久)
   - EfficiencyEnchantment (效率)
   - SharpnessEnchantment (锋利)
   - ProtectionEnchantment (保护系列)

### 优先级中

4. **ItemStack属性修饰符系统** (attribute/) ✅ 完成
   - ArmorItem属性修饰符注册（护甲值、韧性、击退抗性）✅ 完成
   - ItemStack与属性修饰符集成
   - 动态属性修饰符（附魔影响）
   - NBT序列化

5. **特殊物品** (items/special/)
   - SpawnEggItem (生成蛋)
   - CompassItem (指南针)
   - ClockItem (时钟)
   - FishingRodItem (钓鱼竿) ✅ 完成
   - EnchantedBookItem (附魔书) ✅ 完成
   - FlintAndSteelItem (打火石) ✅ 完成 (点燃营火、放置火焰)
   - BoneMealItem (骨粉) ✅ 完成
   - ShearsItem (剪刀) ✅ 完成 (见工具系统，支持实体交互剪羊毛)

6. **桶类物品** (items/special/)
   - BucketItem (空桶) ✅ 完成 (支持对牛挤奶)
   - WaterBucketItem (水桶) ✅ 完成
   - LavaBucketItem (熔岩桶) ✅ 完成
   - FishBucketItem (鱼桶) ✅ 完成 (鳕鱼、鲑鱼、河豚、热带鱼)
   - MilkBucketItem (牛奶桶) ✅ 完成 (清除药水效果)

7. **药水物品** (items/potion/)
   - PotionItem (药水) - 饮用型药水
   - ThrowablePotionItem (可投掷药水基类) ✅ 新增
   - SplashPotionItem (喷溅药水) ✅ 完成 - 继承ThrowablePotionItem
   - LingeringPotionItem (滞留药水) ✅ 完成 - 继承ThrowablePotionItem
   - GlassBottleItem (玻璃瓶)

8. **药水箭** (items/weapon/)
   - ArrowItem (普通箭矢)
   - TippedArrowItem (药水箭) ✅ 完成 - 带药水效果的箭矢

### 系统性待实现项

| 功能 | 状态 | 依赖组件 |
|------|------|----------|
| ShieldItem 格挡逻辑 | 部分实现 | Player::canBlockDamageSource、盾牌冷却 |
| TridentItem 激流冲刺 | 部分实现 | Player::startSpinAttack、SpinAttack状态管理 |
| EnderPearlItem 冷却 | 未实现 | CooldownTracker系统 |
| ArmorItem属性修饰符注册 | ✅ 完成 | ItemAttributeModifiers、AttributeModifierUUIDs |

### ArmorItem属性修饰符注册说明 ✅ 已完成

MC 1.16.5中，盔甲物品需要在构造函数中注册属性修饰符：
- `generic.armor` - 护甲值（每个盔甲部件的防御值）✅
- `generic.armor_toughness` - 护甲韧性（钻石/下界合金为2.0）✅
- `generic.knockback_resistance` - 击退抗性（下界合金为0.1）✅

**已实现**：
1. ✅ 在`ArmorItem.hpp`中添加`m_attributeModifiers`成员和`getAttributeModifiers()`方法
2. ✅ 在`ArmorItem`构造函数中通过`buildAttributeModifiers()`创建属性修饰符
3. ✅ 使用预定义UUID常量（`ARMOR_MODIFIER_UUID_FEET/LEGS/CHEST/HEAD`）
4. ✅ 与`ItemAttributeModifiers`系统集成

**参考**：`net.minecraft.item.ArmorItem`构造函数中的`ARMOR_MODIFIERS`数组 |

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

## 容易踩的坑

### 1. 物品堆栈大小限制

不同物品有不同的堆栈大小限制（通常为 64，部分物品为 16 或 1）。创建 `ItemStack` 时需要检查 `maxStackSize`。

### 2. 盔甲自动装备槽位冲突

盔甲物品现在支持右键自动装备对应槽位；如果目标槽位已被占用，则保持原物品不变并返回透传结果。

### 3. DyeableArmorItem 颜色标签管理

**问题**：`DyeableArmorItem` 将颜色存储在 `ItemStack` 的结构化标签树中，清除颜色时未清除空的 `display` 标签会导致元数据相等性发散。

**解决方案**：清除颜色时也必须清除空的 `display` 标签，否则盔甲堆将停止按预期合并。

### 4. GlassBottleItem 水源检测

**问题**：`GlassBottleItem` 在决定瓶子是否可以装满之前，沿玩家视线进行采样，液体方块不提供可用的碰撞形状，纯命中测试不足以检测水源。

**解决方案**：需要正确检测水源方块，不能仅依赖碰撞形状。

### 5. CreativeInventory 初始化顺序

**问题**：`CreativeInventory` 相关测试和启动代码如果初始化顺序错误，创造物品库会出现空列表或缺失方块物品。

**解决方案**：必须按 `VanillaBlocks::initialize()` -> `Items::initialize()` -> `BlockItemRegistry::instance().initializeVanillaBlockItems()` 的顺序初始化。

### 6. CraftingMenu 菜单验证

**问题**：`CraftingMenu::stillValid()` 需要正确检查玩家到工作台的距离。

**解决方案**：保持工作台可访问性绑定到方块实体位置，使容器有效性匹配预期的交互范围。

### 7. ChestContainer 和 FurnaceContainer 需要真正的 PlayerInventory

**问题**：通过遗留的 `Container` 路由创建箱子/熔炉 GUI 会导致功能不完整。

**解决方案**：`ChestContainer` 和 `FurnaceContainer` 现在需要真正的 `PlayerInventory` 并位于 `AbstractContainerMenu` 下，使用共享菜单工厂/打开容器钩子。

### 8. InventoryManager 背包同步回调

**问题**：服务器侧背包变更如果走 `inventoryManager()`，但没有设置回调，客户端不会收到更新。

**解决方案**：`InventoryManager::setOnInventoryUpdate()` 在 `MinecraftServer::initializeInteractionManagers()` 里已经接好，服务器侧背包变更如果走 `inventoryManager()`，就要依赖这条回调刷新客户端，不要再手写一套新的同步分支。

### 9. BlockItem 放置上下文需要非 const IWorld

**问题**：`BlockItemUseContext` 需要修改世界状态（放置方块、消耗物品等），因此需要非 const 的 `IWorld&` 引用。

**解决方案**：`ItemUseContext` 和 `BlockItemUseContext` 使用非 const `IWorld&` 引用，支持 `setBlockState` 等修改操作。

### 10. WallOrFloorItem 用于可挂墙物品

**问题**：告示牌、旗帜、头颅等物品既可以放在地上也可以贴在墙上，需要特殊处理。

**解决方案**：使用 `WallOrFloorItem` 类，根据玩家视线方向自动选择放置地板方块或墙壁方块。

## 测试文件

相关测试文件位于 `tests/common/item/` 目录：

| 测试文件 | 测试内容 |
|---------|---------|
| `test_item.cpp` | Item/ItemStack 基础功能 |
| `NewItemTest.cpp` | 新物品测试 |
| `tool/ToolTests.cpp` | 工具类测试 |
| `tool/ShearsItemTest.cpp` | 剪刀与羊剪羊毛交互测试 |
| `special/BucketItemTest.cpp` | 桶类物品与牛挤奶交互测试 |
| `weapon/ThrowableItemTest.cpp` | 可投掷物品测试 |
| `weapon/WeaponItemTest.cpp` | 武器物品测试 |
| `potion/GlassBottleItemTest.cpp` | 玻璃瓶物品测试 |
| `enchantment/EnchantmentTest.cpp` | 附魔系统测试 |
| `enchantment/EnchantmentCallbackTest.cpp` | 附魔回调测试 |
| `crafting/RecipeLoaderTest.cpp` | 配方加载测试 |
| `crafting/ShapedRecipeTest.cpp` | 有序合成测试 |
| `crafting/ShapelessRecipeTest.cpp` | 无序合成测试 |
