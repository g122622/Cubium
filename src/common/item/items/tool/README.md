# 工具模块 (Tool Module)

本模块实现了 Minecraft 中的工具系统，包括镐、斧、锹、锄、剑等工具类物品。

## 目录结构

```
src/common/item/tool/
├── ToolType.hpp           # 工具类型枚举定义
├── ToolType.cpp           # 工具类型枚举实现
├── TieredItem.hpp         # 层级物品基类头文件
├── TieredItem.cpp         # 层级物品基类实现
├── ToolItem.hpp           # 挖掘工具基类头文件
├── ToolItem.cpp           # 挖掘工具基类实现
├── PickaxeItem.hpp        # 镐类工具头文件
├── PickaxeItem.cpp        # 镐类工具实现
├── AxeItem.hpp            # 斧类工具头文件
├── AxeItem.cpp            # 斧类工具实现
├── ShovelItem.hpp         # 锹类工具头文件
├── ShovelItem.cpp         # 锹类工具实现
├── HoeItem.hpp            # 锄类工具头文件
├── HoeItem.cpp            # 锄类工具实现
├── SwordItem.hpp          # 剑类武器头文件
└── SwordItem.cpp          # 剑类武器实现
```

## 类继承关系

```
Item
  └── TieredItem (层级物品基类)
        ├── ToolItem (挖掘工具基类)
        │     ├── PickaxeItem (镐)
        │     ├── AxeItem (斧)
        │     ├── ShovelItem (锹)
        │     └── HoeItem (锄)
        └── SwordItem (剑) - 注意：剑不是 ToolItem
```

## 文件详解

### ToolType.hpp / ToolType.cpp

**职责**：定义工具类型枚举，用于判断方块是否可以被特定工具有效挖掘。

**主要内容**：
```cpp
enum class ToolType : u8 {
    None = 0,       // 无需工具或非工具物品
    Pickaxe = 1,    // 镐 - 用于采矿（石头、矿石等）
    Axe = 2,        // 斧 - 用于伐木（原木、木板等）
    Shovel = 3,     // 锹 - 用于挖掘（泥土、沙子、雪等）
    Hoe = 4,        // 锄 - 用于耕作（干草、树叶等）
    Sword = 5,      // 剑 - 对蜘蛛网有效
    Shears = 6,     // 剪刀 - 用于剪羊毛、树叶等
};
```

**常量定义**：
- `TOOL_TYPE_NONE = 0`
- `TOOL_TYPE_PICKAXE = 1`
- `TOOL_TYPE_AXE = 2`
- `TOOL_TYPE_SHOVEL = 3`
- `TOOL_TYPE_HOE = 4`
- `TOOL_TYPE_SWORD = 5`
- `TOOL_TYPE_SHEARS = 6`

### TieredItem.hpp / TieredItem.cpp

**职责**：所有具有材质层级的物品（工具、剑、护甲）的基类，提供层级相关的基础属性。

**主要功能**：
- 管理工具层级引用（木、石、铁、金、钻石、下界合金）
- 自动从层级获取耐久度设置
- 提供附魔能力查询
- 支持铁砧修复机制

**关键方法**：
```cpp
const IItemTier& getTier() const;              // 获取工具层级
i32 getItemEnchantability() const override;    // 获取附魔能力
bool isRepairable(const ItemStack& toRepair,   // 检查修复材料
                  const ItemStack& repair) const;
```

### ToolItem.hpp / ToolItem.cpp

**职责**：所有挖掘工具（镐、斧、锹、锄）的基类，提供挖掘速度计算、耐久度消耗、有效方块判断等功能。

**关键属性**：
- `m_effectiveBlocks`: 有效方块集合
- `m_toolType`: 工具类型
- `m_attackDamage`: 总攻击伤害（基础伤害 + 层级加成）
- `m_attackSpeed`: 攻击速度修正
- `m_efficiency`: 挖掘效率值

**关键方法**：
```cpp
f32 getDestroySpeed(const ItemStack& stack,      // 计算挖掘速度（含效率附魔加成）
                    const BlockState& state) const override;
bool canHarvestBlock(const BlockState& state) const override;  // 判断采集能力
bool hitEntity(ItemStack& stack, ...);           // 攻击实体（消耗2耐久）
bool onBlockDestroyed(ItemStack& stack, ...);    // 破坏方块（消耗1耐久）
```

**效率附魔集成**（MC 1.16.5）：
```cpp
f32 ToolItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    // 1. 检查材质有效性
    f32 speed = 1.0f;
    if (isEffectiveMaterial(state.getMaterial())) {
        speed = m_efficiency;
    } else if (isEffectiveBlock(state.owner())) {
        speed = m_efficiency;
    }

    // 2. 应用效率附魔加成（只在工具有效时）
    if (speed > 1.0f) {
        i32 efficiencyLevel = EnchantmentHelper::getEfficiencyLevel(stack);
        if (efficiencyLevel > 0) {
            speed += EfficiencyEnchantment::getMiningSpeedBonus(efficiencyLevel);
            // 公式: level^2 + 1, 即 I=2, II=5, III=10, IV=17, V=26
        }
    }

    return speed;
}
```

**设计模式**：
- 模板方法模式：`isEffectiveMaterial()` 由子类重写定义各自有效的材质

### PickaxeItem.hpp / PickaxeItem.cpp

**职责**：镐类工具实现，用于挖掘石头、矿石等方块。

**有效材质**：
- `Material::ROCK` - 石头类
- `Material::IRON` - 铁类
- `Material::ANVIL` - 铁砧类

**有效方块**（部分列表）：
- 基础石头类：石头、圆石、苔石
- 矿石类：煤矿、铁矿、金矿、钻石矿、绿宝石矿、青金石矿、红石矿、铜矿
- 下界矿石：下界石英矿、下界金矿、远古残骸
- 石砖系列、矿物方块、黑曜石、末地石、玄武岩、黑石等
- 台阶类：石台阶、圆石台阶、石砖台阶、苔石砖台阶、海晶石台阶、海晶砖台阶、暗海晶石台阶
- 其他：梯子、脚手架、活塞头

**挖掘等级要求**：
| 层级 | 等级 | 可采集方块 |
|------|------|-----------|
| 木/金 | 0 | 煤矿、石头 |
| 石 | 1 | 铁矿、青金石矿 |
| 铁 | 2 | 钻石矿、金矿、红石矿 |
| 钻石 | 3 | 黑曜石 |
| 下界合金 | 4 | 远古残骸 |

### AxeItem.hpp / AxeItem.cpp

**职责**：斧类工具实现，用于砍伐木头、木板等方块。

**有效材质**：
- `Material::WOOD` - 木头
- `Material::NETHER_WOOD` - 下界木头
- `Material::PLANT` - 植物
- `Material::GOURD` - 葫芦（南瓜、西瓜）
- `Material::BAMBOO` - 竹子

**有效方块**（部分列表）：
- 木板：橡木、云杉、白桦、丛林、金合欢、深色橡木
- 原木：各种原木
- 树叶：各种树叶
- 其他：书架、工作台等

**特殊功能（MC 1.16.5 已实现）**：
- **去皮功能**：右键原木可去皮（log -> stripped_log）
  - 支持6种主世界原木/木头：橡木、云杉、白桦、丛林、金合欢、深色橡木
  - 消耗1点耐久度
  - 播放去皮音效（ITEM_AXE_STRIP）
  - 使用方法：`onItemUse()` / `getStrippedBlock()`

### ShovelItem.hpp / ShovelItem.cpp

**职责**：锹类工具实现，用于挖掘泥土、沙子、雪等方块。

**有效材质**：
- `Material::EARTH` - 泥土类
- `Material::SAND` - 沙子类
- `Material::SNOW` - 雪类

**有效方块**：
- 泥土类：泥土、草方块、砂土、灰化土、土径、菌丝
- 沙子类：沙子、沙砾
- 雪类：雪
- 其他：粘土、灵魂沙、灵魂土

**特殊功能（MC 1.16.5 已实现）**：
- **土径创建**：右键草方块可创建土径（grass_block -> grass_path）
  - 条件：点击面不能是底面，上方必须是空气
  - 消耗1点耐久度
  - 播放音效（ITEM_SHOVEL_FLATTEN）
  - 使用方法：`onItemUse()` / `getPathBlock()`

### HoeItem.hpp / HoeItem.cpp

**职责**：锄类工具实现，用于耕作和清理树叶、干草等方块。

**有效材质**：
- `Material::LEAVES` - 树叶
- `Material::MOSS` - 苔藓

**有效方块**：
- 干草块、海绵、湿海绵
- 各种树叶
- 地狱疣块

**特殊功能（MC 1.16.5 已实现）**：
- **耕地创建**：右键泥土/草地可创建耕地
  - 草方块 -> 耕地
  - 土径 -> 耕地
  - 泥土 -> 耕地
  - 砂土 -> 泥土（需要再锄一次变成耕地）
  - 条件：点击面不能是底面，上方必须是空气
  - 消耗1点耐久度
  - 播放音效（ITEM_HOE_TILL）
  - 使用方法：`onItemUse()` / `getTilledBlock()`

### SwordItem.hpp / SwordItem.cpp

**职责**：剑类武器实现，用于战斗。

**注意**：剑继承自 `TieredItem` 而非 `ToolItem`，具有不同的行为模式。

**特殊行为**：
- 对蜘蛛网挖掘效率极高（15.0）
- 对植物有轻微效率（1.5）
- 攻击敌人消耗 1 耐久（工具消耗 2）
- 破坏方块消耗 2 耐久（工具消耗 1）
- 只能采集蜘蛛网

**攻击伤害**：基础值 + 层级加成

## 模块职责

### 整体职责

本模块负责实现 Minecraft 中的工具系统，包括：

1. **工具类型定义**：定义工具分类（镐、斧、锹、锄、剑、剪刀）
2. **层级系统支持**：支持木、石、铁、金、钻石、下界合金六种材质层级
3. **挖掘机制**：计算不同工具对不同方块的挖掘速度
4. **采集判断**：判断工具是否能正确采集方块（等级检查）
5. **耐久度管理**：工具使用时的耐久度消耗
6. **攻击属性**：工具的攻击伤害和攻击速度

### 输入和输出

**输入**：
- `IItemTier` - 工具层级接口，提供材质属性
- `Block` / `BlockState` - 方块信息，用于判断有效性和挖掘速度
- `Material` - 材质信息，用于判断工具有效性
- `ItemProperties` - 物品基础属性

**输出**：
- 具体的工具类实例（注册到 `Items`）
- 挖掘速度值（`getDestroySpeed`）
- 采集能力判断（`canHarvestBlock`）
- 攻击属性（攻击伤害、攻击速度）

### 依赖项

**内部依赖**：
```
src/common/item/tool/
├── depends on src/common/item/Item.hpp
├── depends on src/common/item/ItemStack.hpp
├── depends on src/common/item/tier/IItemTier.hpp
├── depends on src/common/item/crafting/Ingredient.hpp
├── depends on src/common/world/block/Block.hpp
├── depends on src/common/world/block/BlockState.hpp
├── depends on src/common/world/block/Material.hpp
└── depends on src/common/world/block/VanillaBlocks.hpp
```

**外部依赖**：
- `<unordered_set>` - 存储有效方块集合
- `<unordered_map>` - 存储映射关系（如去皮映射）

## 使用方法

### 创建自定义工具层级

```cpp
#include "item/tier/IItemTier.hpp"
#include "item/crafting/Ingredient.hpp"

class CustomTier : public tier::IItemTier {
public:
    i32 getMaxUses() const override { return 1000; }
    f32 getEfficiency() const override { return 10.0f; }
    f32 getAttackDamage() const override { return 5.0f; }
    i32 getHarvestLevel() const override { return 3; }
    i32 getEnchantability() const override { return 15; }
    const crafting::Ingredient& getRepairMaterial() const override {
        return m_repairMaterial;
    }
private:
    crafting::Ingredient m_repairMaterial;
};
```

### 创建工具实例

```cpp
#include "item/items/tool/PickaxeItem.hpp"
#include "item/core/Item.hpp"
#include "item/tier/ItemTiers.hpp"

// 创建钻石镐
ItemProperties props;
auto diamondPickaxe = new PickaxeItem(
    ItemTiers::DIAMOND(),    // 层级
    1,                        // 基础攻击伤害
    -2.8f,                   // 攻击速度修正
    props                    // 物品属性
);
```

### 查询工具属性

```cpp
// 检查是否能采集方块
if (pickaxe->canHarvestBlock(blockState)) {
    // 可以采集
}

// 获取挖掘速度
ItemStack stack(*pickaxe, 1);
f32 speed = pickaxe->getDestroySpeed(stack, blockState);

// 获取攻击伤害
f32 damage = pickaxe->getAttackDamage();
```

## 容易踩的坑

### 1. 初始化顺序问题

**问题**：工具注册时需要有效的方块指针（`VanillaBlocks`），如果方块未初始化会导致空指针。

**解决方案**：确保初始化顺序正确：
```cpp
// 正确顺序
VanillaBlocks::initialize();  // 先初始化方块
Items::initialize();          // 再初始化物品（包括工具）
```

### 2. 剑不是 ToolItem

**问题**：剑继承自 `TieredItem` 而非 `ToolItem`，具有不同的耐久消耗机制。

**注意事项**：
- 剑攻击实体消耗 1 耐久（工具消耗 2）
- 剑破坏方块消耗 2 耐久（工具消耗 1）
- 不要将剑当作 `ToolItem` 使用

### 3. 挖掘等级判断

**问题**：不同工具的 `canHarvestBlock` 实现不同。

**镐的特殊逻辑**：
```cpp
// 镐对 ROCK, IRON, ANVIL 材质总是可以采集
// 即使不匹配工具类型
if (mat == Material::ROCK || mat == Material::IRON || mat == Material::ANVIL) {
    return true;
}
```

**锹的特殊逻辑**：
```cpp
// 锹对雪类方块总是可以采集
if (mat == Material::SNOW) {
    return true;
}
```

### 4. 有效方块集合的静态初始化

**问题**：`initializeEffectiveBlocks()` 在构造函数中调用，如果方块指针为 `nullptr`，会导致集合为空。

**解决方案**：使用条件检查：
```cpp
if (VanillaBlocks::STONE) blocks.insert(VanillaBlocks::STONE);
```

### 5. 材质检查 vs 方块检查

**问题**：工具的有效性判断有两种方式：材质检查和方块检查。

**优先级**：
1. 先检查材质（`isEffectiveMaterial`）
2. 再检查特定方块（`isEffectiveBlock`）

```cpp
f32 getDestroySpeed(...) const {
    // 1. 材质检查
    if (isEffectiveMaterial(state.getMaterial())) {
        return m_efficiency;
    }
    // 2. 方块检查
    if (isEffectiveBlock(state.owner())) {
        return m_efficiency;
    }
    return 1.0f;
}
```

### 6. 静态映射表的初始化顺序

**问题**：AxeItem、ShovelItem、HoeItem 的静态映射表（去皮、土径、耕地）在构造函数中初始化，但静态方法 `getStrippedBlock()`、`getPathBlock()`、`getTilledBlock()` 可能在任何工具实例创建前被调用（如测试代码），导致映射表为空。

**解决方案**：使用"construct on first use"模式（函数局部静态变量）：

```cpp
// 旧方式（有初始化顺序问题）
static std::unordered_map<const Block*, const Block*> s_strippingMap;
// 在构造函数中: if (s_strippingMap.empty()) s_strippingMap = initializeStrippingMap();

// 新方式（惰性初始化，首次调用时自动初始化）
static std::unordered_map<const Block*, const Block*>& getStrippingMap() {
    static std::unordered_map<const Block*, const Block*> map = []() {
        std::unordered_map<const Block*, const Block*> m;
        // 初始化映射...
        return m;
    }();
    return map;
}
```

这样静态方法可以安全调用：
```cpp
const Block* AxeItem::getStrippedBlock(const Block* original) {
    if (original == nullptr) return nullptr;
    auto& map = getStrippingMap();  // 自动初始化
    auto it = map.find(original);
    return (it != map.end()) ? it->second : nullptr;
}
```

## 测试用例

测试文件位于 `tests/common/item/tool/ToolTests.cpp`，包含以下测试：

### 层级测试 (ItemTierTest)

| 测试名称 | 测试内容 |
|---------|---------|
| WoodTierValues | 验证木层级属性（耐久59，效率2.0，等级0） |
| StoneTierValues | 验证石层级属性（耐久131，效率4.0，等级1） |
| IronTierValues | 验证铁层级属性（耐久250，效率6.0，等级2） |
| DiamondTierValues | 验证钻石层级属性（耐久1561，效率8.0，等级3） |
| GoldTierValues | 验证金层级属性（耐久32，效率12.0，等级0，附魔22） |
| NetheriteTierValues | 验证下界合金属性（耐久2031，效率9.0，等级4） |

### 工具物品测试 (ToolItemTest)

| 测试名称 | 测试内容 |
|---------|---------|
| DiamondPickaxeDurability | 钻石镐耐久度 1561 |
| IronPickaxeDurability | 铁镐耐久度 250 |
| StonePickaxeDurability | 石镐耐久度 131 |
| WoodenPickaxeDurability | 木镐耐久度 59 |
| GoldenPickaxeDurability | 金镐耐久度 32 |
| PickaxeIsTieredItem | 镐具有层级附魔能力 |
| SwordDamage | 钻石剑属性验证 |
| PickaxeEnchantability | 各材质附魔能力验证 |
| ToolTypeConstants | 工具类型常量值验证 |

### 采集测试 (ToolHarvestTest)

| 测试名称 | 测试内容 |
|---------|---------|
| PickaxeSpeedOnStone | 钻石镐对石头挖掘速度 8.0 |
| WoodenPickaxeCannotHarvestDiamondOre | 木镐无法采集钻石矿（等级不足） |
| IronPickaxeCanHarvestDiamondOre | 铁镐可以采集钻石矿（等级匹配） |
| DiamondPickaxeCanHarvestDiamondOre | 钻石镐可以采集钻石矿（等级足够） |
| PickaxeSpeedOnDirt | 镐对泥土挖掘速度 1.0（无效） |
| ShovelSpeedOnDirt | 钻石锹对泥土挖掘速度 8.0 |
| AxeSpeedOnOakLog | 钻石斧对橡木原木挖掘速度 8.0 |
| BlockStateHarvestProperties | 方块采集属性验证 |
| BlockStateRequiresTool | 方块工具需求验证 |
| ToolEffectiveCheck | 工具有效性检查验证 |

## 参考资料

- Minecraft Java 1.16.5 源码：`net.minecraft.item.ToolItem`, `net.minecraft.item.PickaxeItem` 等
- Minecraft Wiki：https://minecraft.fandom.com/wiki/Tools
