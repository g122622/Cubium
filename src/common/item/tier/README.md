# 工具层级模块 (Item Tier Module)

## 目录结构

```
src/common/item/tier/
├── IItemTier.hpp      # 工具层级接口定义
├── IItemTier.cpp      # 空实现文件（接口无实现）
├── ItemTiers.hpp      # 原版层级定义
└── ItemTiers.cpp      # 原版层级实现
```

## 文件详细介绍

### IItemTier.hpp

**职责**: 定义工具层级接口 (`IItemTier`)，作为所有工具材质层级的抽象基类。

**主要内容**:
- 定义纯虚函数接口，描述工具材质的核心属性
- 前向声明 `crafting::Ingredient` 类，避免头文件循环依赖

**接口方法**:

| 方法 | 返回类型 | 说明 |
|------|----------|------|
| `getMaxUses()` | `i32` | 获取最大耐久度，工具能承受的伤害值 |
| `getEfficiency()` | `f32` | 获取挖掘效率倍率，影响挖掘速度 |
| `getAttackDamage()` | `f32` | 获取基础攻击伤害加成 |
| `getHarvestLevel()` | `i32` | 获取挖掘等级 (0-4)，决定能采集的方块类型 |
| `getEnchantability()` | `i32` | 获取附魔能力，越高附魔效果越好 |
| `getRepairMaterial()` | `const Ingredient&` | 获取修复材料，用于铁砧修复 |

**挖掘等级说明**:
- **0**: 木/金 - 可采集煤矿、石头
- **1**: 石 - 可采集铁矿
- **2**: 铁 - 可采集钻石矿、金矿
- **3**: 钻石 - 可采集黑曜石
- **4**: 下界合金 - 可采集远古残骸

---

### IItemTier.cpp

**职责**: 空实现文件，仅包含注释说明具体实现在 `ItemTiers.cpp` 中。

**主要内容**:
- 由于 `IItemTier` 是纯接口，所有方法都是纯虚函数，此文件为空

---

### ItemTiers.hpp

**职责**: 定义原版 Minecraft 的六种工具材质层级，提供静态访问方法。

**主要内容**:
- `ItemTiers` 类：静态工厂类，管理所有原版层级实例
- 六种原版层级的静态访问方法
- 初始化状态检查

**类方法**:

| 方法 | 说明 |
|------|------|
| `initialize()` | 初始化所有层级（必须在 `Items::initialize()` 之后调用） |
| `isInitialized()` | 检查是否已初始化 |
| `WOOD()` | 获取木制工具层级 |
| `STONE()` | 获取石制工具层级 |
| `IRON()` | 获取铁制工具层级 |
| `DIAMOND()` | 获取钻石工具层级 |
| `GOLD()` | 获取金制工具层级 |
| `NETHERITE()` | 获取下界合金工具层级 |

---

### ItemTiers.cpp

**职责**: 实现原版六种材质层级的具体数据。

**主要内容**:
- 内部实现类 `ItemTierImpl`：继承 `IItemTier` 接口的具体实现
- 六种原版层级的属性数据定义
- 修复材料的配置（使用 `crafting::Ingredient`）

**层级属性对照表**:

| 层级 | 挖掘等级 | 耐久度 | 效率 | 攻击伤害 | 附魔值 | 修复材料 |
|------|---------|--------|------|---------|--------|----------|
| WOOD | 0 | 59 | 2.0 | 0.0 | 15 | 各类木板 |
| STONE | 1 | 131 | 4.0 | 1.0 | 5 | 圆石 |
| IRON | 2 | 250 | 6.0 | 2.0 | 14 | 铁锭 |
| DIAMOND | 3 | 1561 | 8.0 | 3.0 | 10 | 钻石 |
| GOLD | 0 | 32 | 12.0 | 0.0 | 22 | 金锭 |
| NETHERITE | 4 | 2031 | 9.0 | 4.0 | 15 | 下界合金锭 |

**修复材料详解**:
- **WOOD**: 所有六种木板（橡木、云杉、白桦、丛林、金合欢、深色橡木）
- **STONE**: 圆石
- **IRON**: 铁锭
- **DIAMOND**: 钻石
- **GOLD**: 金锭
- **NETHERITE**: 下界合金锭

---

## 文件之间的关系

```
┌─────────────────┐
│  IItemTier.hpp  │  ←── 接口定义
└────────┬────────┘
         │ 继承
         ▼
┌─────────────────┐
│ ItemTierImpl    │  ←── 内部实现类（在 ItemTiers.cpp 中定义）
│ (ItemTiers.cpp) │
└────────┬────────┘
         │ 由其创建
         ▼
┌─────────────────┐
│  ItemTiers.hpp  │  ←── 静态工厂类，持有六个 IItemTier 实例
└────────┬────────┘
         │ 被使用
         ▼
┌─────────────────┐
│   TieredItem    │  ←── 层级物品基类（src/common/item/tool/TieredItem.hpp）
│   ToolItem      │  ←── 工具基类（src/common/item/tool/ToolItem.hpp）
│   PickaxeItem   │  ←── 具体工具实现
│   AxeItem       │
│   ShovelItem    │
│   HoeItem       │
│   SwordItem     │
└─────────────────┘
```

**依赖关系**:
1. `ItemTiers.cpp` 依赖 `Items.hpp`（获取修复材料物品引用）
2. `ItemTiers.cpp` 依赖 `crafting/Ingredient.hpp`（创建修复材料配方成分）
3. `TieredItem` 和 `ToolItem` 依赖 `IItemTier.hpp`（接收层级引用）

---

## 模块整体说明

### 整体职责

此模块负责定义工具材质层级系统，为 Minecraft 原版的六种工具材质（木、石、铁、金、钻石、下界合金）提供统一的属性访问接口。工具层级决定了工具的耐久度、挖掘效率、攻击伤害、挖掘等级和附魔能力等核心属性。

### 输入和输出

**输入**:
- 无（层级数据是硬编码的原版数值）
- 依赖 `Items::initialize()` 提前调用（修复材料需要引用已注册的物品）

**输出**:
- 六个 `IItemTier` 接口引用，通过 `ItemTiers::XXX()` 静态方法获取
- 层级属性值供工具系统使用

### 依赖项

| 依赖项 | 类型 | 用途 |
|--------|------|------|
| `common/core/Types.hpp` | 内部依赖 | 基础类型定义（i32, f32 等） |
| `common/item/crafting/Ingredient.hpp` | 内部依赖 | 修复材料配方成分 |
| `common/item/Items.hpp` | 内部依赖 | 物品引用（修复材料） |

### 使用方法

```cpp
#include "common/item/tier/ItemTiers.hpp"
#include "common/item/items/tool/PickaxeItem.hpp"

// 1. 初始化（必须在 Items::initialize() 之后）
Items::initialize();
ItemTiers::initialize();

// 2. 获取层级引用
const auto& diamondTier = ItemTiers::DIAMOND();

// 3. 查询层级属性
i32 durability = diamondTier.getMaxUses();      // 1561
f32 efficiency = diamondTier.getEfficiency();    // 8.0f
i32 harvestLevel = diamondTier.getHarvestLevel(); // 3

// 4. 创建工具时使用
auto pickaxe = PickaxeItem(
    1.0f,  // 基础攻击伤害
    -2.8f, // 攻击速度修正
    ItemTiers::DIAMOND(),  // 层级
    effectiveBlocks,       // 有效方块
    ToolType::PICKAXE,     // 工具类型
    properties             // 物品属性
);
```

### 容易踩的坑

#### 1. 初始化顺序错误

**问题**: 如果在 `Items::initialize()` 之前调用 `ItemTiers::initialize()`，会导致崩溃或未定义行为，因为修复材料引用的物品指针尚未初始化。

**解决方案**: 确保按正确顺序初始化：
```cpp
// 正确顺序
Items::initialize();      // 先初始化物品注册表
ItemTiers::initialize();  // 再初始化层级（依赖物品引用）

// 错误顺序 - 会导致崩溃
// ItemTiers::initialize();  // 错误！此时物品还未注册
// Items::initialize();
```

**注意**: 在 `Items::initialize()` 内部已经调用了 `ItemTiers::initialize()`，因此通常不需要手动调用。

#### 2. 金工具的特殊性

**问题**: 金工具的挖掘等级是 0（与木相同），容易误以为可以挖掘铁矿等方块。

**实际情况**: 金工具虽然效率最高（12.0），但挖掘等级为 0，只能采集与木工具相同的方块类型。

#### 3. 修复材料引用生命周期

**问题**: `getRepairMaterial()` 返回的是 `const Ingredient&` 引用，该引用的生命周期由 `ItemTiers.cpp` 中的静态变量管理。

**解决方案**: 不要存储修复材料的指针或引用到长期存在的对象中，建议在使用时立即获取。

#### 4. 层级引用不能为空

**问题**: `ItemTiers::XXX()` 返回的是引用而非指针，不存在空层级。

**注意**: 这意味着所有工具必须有有效的层级，无法创建"无层级"的工具。

---

## 涉及的测试用例

测试文件位置: `tests/common/item/tool/ToolTests.cpp`

### 层级属性测试

| 测试名称 | 测试内容 |
|----------|----------|
| `ItemTierTest.WoodTierValues` | 验证木层级属性值（耐久59、效率2.0、伤害0、挖掘等级0、附魔值15） |
| `ItemTierTest.StoneTierValues` | 验证石层级属性值（耐久131、效率4.0、伤害1.0、挖掘等级1、附魔值5） |
| `ItemTierTest.IronTierValues` | 验证铁层级属性值（耐久250、效率6.0、伤害2.0、挖掘等级2、附魔值14） |
| `ItemTierTest.DiamondTierValues` | 验证钻石层级属性值（耐久1561、效率8.0、伤害3.0、挖掘等级3、附魔值10） |
| `ItemTierTest.GoldTierValues` | 验证金层级属性值（耐久32、效率12.0、伤害0、挖掘等级0、附魔值22） |
| `ItemTierTest.NetheriteTierValues` | 验证下界合金层级属性值（耐久2031、效率9.0、伤害4.0、挖掘等级4、附魔值15） |

### 工具耐久度测试

| 测试名称 | 测试内容 |
|----------|----------|
| `ToolItemTest.DiamondPickaxeDurability` | 钻石镐耐久度应为 1561 |
| `ToolItemTest.IronPickaxeDurability` | 铁镐耐久度应为 250 |
| `ToolItemTest.StonePickaxeDurability` | 石镐耐久度应为 131 |
| `ToolItemTest.WoodenPickaxeDurability` | 木镐耐久度应为 59 |
| `ToolItemTest.GoldenPickaxeDurability` | 金镐耐久度应为 32 |

### 附魔能力测试

| 测试名称 | 测试内容 |
|----------|----------|
| `ToolItemTest.PickaxeIsTieredItem` | 验证镐继承层级附魔值（钻石镐为10） |
| `ToolItemTest.PickaxeEnchantability` | 验证不同材质镐的附魔值（金22、钻石10、铁14） |

### 挖掘等级测试

| 测试名称 | 测试内容 |
|----------|----------|
| `ToolHarvestTest.WoodenPickaxeCannotHarvestDiamondOre` | 木镐（挖掘等级0）无法采集钻石矿（需要等级2） |
| `ToolHarvestTest.IronPickaxeCanHarvestDiamondOre` | 铁镐（挖掘等级2）可以采集钻石矿 |
| `ToolHarvestTest.DiamondPickaxeCanHarvestDiamondOre` | 钻石镐（挖掘等级3）可以采集钻石矿 |

### 效率测试

| 测试名称 | 测试内容 |
|----------|----------|
| `ToolHarvestTest.PickaxeSpeedOnStone` | 钻石镐在石头上的挖掘速度应为 8.0 |
| `ToolHarvestTest.PickaxeSpeedOnDirt` | 镐在泥土上（非有效方块）挖掘速度应为 1.0 |
| `ToolHarvestTest.ShovelSpeedOnDirt` | 钻石锹在泥土上的挖掘速度应为 8.0 |
| `ToolHarvestTest.AxeSpeedOnOakLog` | 钻石斧在橡木原木上的挖掘速度应为 8.0 |

---

## 参考

此模块参考了 Minecraft Java 1.16.5 的实现：
- `net.minecraft.item.IItemTier` - 工具层级接口
- `net.minecraft.item.ItemTier` - 原版层级枚举
