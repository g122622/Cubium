# Enchantment 附魔系统

本目录实现了 Minecraft 1.16.5 风格的附魔系统。

## 目录结构

```
enchantment/
├── Enchantment.hpp              # 附魔基类定义
├── Enchantment.cpp              # 附魔基类实现
├── EnchantmentContainer.hpp     # 附魔容器（存储物品上的附魔）
├── EnchantmentContainer.cpp     # 附魔容器实现
├── EnchantmentHelper.hpp        # 附魔查询工具类
├── EnchantmentHelper.cpp        # 附魔查询工具类实现
├── EnchantmentRegistry.hpp      # 附魔注册表
├── EnchantmentRegistry.cpp      # 附魔注册表实现
├── README.md                    # 本文件
└── enchantments/                # 具体附魔实现
    ├── FortuneEnchantment.hpp   # 时运附魔
    ├── FortuneEnchantment.cpp
    ├── SilkTouchEnchantment.hpp # 精准采集附魔
    └── SilkTouchEnchantment.cpp
```

## 文件详解

### 核心文件

#### Enchantment.hpp / Enchantment.cpp

附魔基类，定义所有附魔的通用接口和属性。

**核心枚举：**
- `EnchantmentType`：附魔类型，定义附魔可应用的物品类型
  - `Armor`：护甲（头盔、胸甲、护腿、靴子）
  - `ArmorFeet`：仅靴子
  - `ArmorHead`：仅头盔
  - `ArmorChest`：仅胸甲
  - `Weapon`：武器（剑）
  - `Digger`：挖掘工具（镐、斧、铲、锄）
  - `FishingRod`：钓鱼竿
  - `Breakable`：可破坏物品
  - `Bow`：弓
  - `Wearable`：可穿戴物品
  - `Crossbow`：弩
  - `Vanishable`：可消失物品
  - `All`：所有物品

- `EnchantmentRarity`：附魔稀有度，影响附魔台出现概率
  - `Common`（权重10）：保护、锋利等
  - `Uncommon`（权重5）：冲击、火焰附加等
  - `Rare`（权重2）：时运等
  - `VeryRare`（权重1）：精准采集、经验修补等

**核心接口：**
```cpp
class Enchantment {
public:
    // 标识
    virtual String id() const = 0;
    virtual String getNameKey(i32 level = 1) const;
    
    // 等级
    virtual i32 minLevel() const;
    virtual i32 maxLevel() const;
    
    // 类型与稀有度
    virtual EnchantmentType type() const = 0;
    virtual EnchantmentRarity rarity() const;
    
    // 适用性
    virtual bool canApplyTo(u32 itemType) const;
    virtual bool isCompatibleWith(const Enchantment& other) const;
    
    // 附魔台成本
    virtual i32 getMinCost(i32 level) const;
    virtual i32 getMaxCost(i32 level) const;
    virtual i32 getMinEnchantability(i32 level) const;
    virtual i32 getMaxEnchantability(i32 level) const;
    
    // 修饰符
    virtual f32 getDamageBonus(i32 level, u32 entityType) const;
    virtual i32 getDamageProtection(i32 level, u32 damageType) const;
    
    // 静态方法
    static i32 getRarityWeight(EnchantmentRarity rarity);
};
```

#### EnchantmentContainer.hpp / EnchantmentContainer.cpp

附魔容器，存储物品上的所有附魔实例。

**核心类型：**
```cpp
struct EnchantmentInstance {
    String enchantmentId;   // 附魔ID
    i32 level;              // 附魔等级
    const Enchantment* getEnchantment() const;
};
```

**核心接口：**
```cpp
class EnchantmentContainer {
public:
    // 查询
    bool isEmpty() const;
    size_t size() const;
    i32 getLevel(const String& enchantmentId) const;
    bool has(const String& enchantmentId) const;
    bool hasType(EnchantmentType type) const;
    const std::vector<EnchantmentInstance>& getAll() const;
    
    // 修改
    void set(const String& enchantmentId, i32 level);
    bool remove(const String& enchantmentId);
    void clear();
    
    // 兼容性检查
    bool canAdd(const String& enchantmentId) const;
    
    // 序列化
    void serialize(network::PacketSerializer& ser) const;
    static Result<EnchantmentContainer> deserialize(network::PacketDeserializer& deser);
};
```

#### EnchantmentHelper.hpp / EnchantmentHelper.cpp

附魔查询工具类，提供静态方法查询物品附魔。

**核心接口：**
```cpp
class EnchantmentHelper {
public:
    // 基础查询
    static i32 getEnchantmentLevel(const ItemStack& stack, const String& enchantmentId);
    static i32 getEnchantmentLevel(const ItemStack& stack, const Enchantment* enchantment);
    static bool hasEnchantment(const ItemStack& stack, const String& enchantmentId);
    static bool hasEnchantmentType(const ItemStack& stack, EnchantmentType type);
    static bool hasEnchantments(const ItemStack& stack);
    static std::vector<std::pair<const Enchantment*, i32>> getEnchantments(const ItemStack& stack);
    
    // 便捷方法
    static bool hasSilkTouch(const ItemStack& stack);
    static i32 getFortuneLevel(const ItemStack& stack);
    static i32 getSharpnessLevel(const ItemStack& stack);
    static i32 getUnbreakingLevel(const ItemStack& stack);
    
    // 计算
    static i32 getTotalProtection(const ItemStack& stack, u32 damageType);
    static f32 getTotalDamageBonus(const ItemStack& stack, u32 entityType);
};
```

#### EnchantmentRegistry.hpp / EnchantmentRegistry.cpp

附魔注册表，管理所有已注册的附魔。

**核心接口：**
```cpp
class EnchantmentRegistry {
public:
    static void initialize();                              // 初始化原版附魔
    static bool registerEnchantment(std::unique_ptr<Enchantment> enchantment);
    static const Enchantment* get(const String& id);       // 按ID获取
    static bool has(const String& id);                     // 检查是否存在
    static const std::unordered_map<String, std::unique_ptr<Enchantment>>& all();
    static std::vector<const Enchantment*> getByType(EnchantmentType type);
    static std::vector<const Enchantment*> getAvailableForItem(u32 itemType);
    static void clear();                                   // 清除所有
    static bool isInitialized();
};
```

### 具体附魔实现

#### enchantments/FortuneEnchantment.hpp / FortuneEnchantment.cpp

时运附魔，增加方块掉落物数量。

**效果：**
- Fortune I：33%概率掉落+1
- Fortune II：25%概率掉落+1，25%概率掉落+2
- Fortune III：20%概率掉落+1，20%概率掉落+2，20%概率掉落+3

**适用物品：** 镐、铲、斧、锄

**不兼容：** 精准采集

**静态方法：**
```cpp
static i32 applyBonus(i32 baseCount, i32 level, math::Random& random);      // 时运加成
static i32 applyUniformBonus(i32 level, math::Random& random);              // 均匀分布加成
static i32 applyOreDropBonus(i32 baseMin, i32 baseMax, i32 level, math::Random& random); // 矿石掉落加成
```

#### enchantments/SilkTouchEnchantment.hpp / SilkTouchEnchantment.cpp

精准采集附魔，采集方块本身而非掉落物。

**效果：**
- 采集矿石时掉落矿石本身而非矿物
- 采集玻璃时掉落玻璃
- 采集草方块时掉落草方块
- 采集树叶时掉落树叶

**适用物品：** 镐、铲、斧、锄、剪刀

**不兼容：** 时运

## 模块职责

### 整体职责

附魔系统负责：
1. **定义附魔基类**：提供所有附魔的通用接口和属性
2. **管理附魔注册**：全局注册表，支持按ID、类型查询
3. **存储物品附魔**：容器类支持添加、移除、查询、序列化
4. **提供便捷查询**：工具类封装常用操作
5. **实现具体附魔**：每个附魔有独立的逻辑实现

### 输入和输出

**输入：**
- 附魔ID（如 `"minecraft:fortune"`）
- 物品堆（`ItemStack`）
- 附魔等级
- 随机数生成器（用于时运计算）

**输出：**
- 附魔定义（属性、等级范围、兼容性）
- 附魔等级
- 伤害加成、保护值
- 时运加成后的掉落数量

### 依赖项

```
Enchantment
    ├── common/core/Types.hpp
    ├── common/core/Result.hpp
    └── common/util/math/random/Random.hpp

EnchantmentContainer
    ├── Enchantment.hpp
    └── common/network/packet/PacketSerializer.hpp

EnchantmentHelper
    ├── Enchantment.hpp
    ├── EnchantmentRegistry.hpp
    └── common/item/ItemStack.hpp

EnchantmentRegistry
    ├── Enchantment.hpp
    └── enchantments/* (具体附魔实现)
```

### 使用方法

#### 初始化附魔系统

```cpp
// 游戏启动时初始化
mc::item::enchant::EnchantmentRegistry::initialize();
```

#### 查询附魔

```cpp
using namespace mc::item::enchant;

// 获取附魔定义
const Enchantment* fortune = EnchantmentRegistry::get("minecraft:fortune");
if (fortune) {
    i32 maxLevel = fortune->maxLevel();  // 3
    i32 minCost = fortune->getMinCost(1); // 15
}

// 检查兼容性
const Enchantment* silkTouch = EnchantmentRegistry::get("minecraft:silk_touch");
bool compatible = fortune->isCompatibleWith(*silkTouch);  // false
```

#### 使用附魔容器

```cpp
// 创建附魔容器
EnchantmentContainer container;

// 添加附魔
container.set("minecraft:fortune", 3);
container.set("minecraft:unbreaking", 2);

// 查询
i32 level = container.getLevel("minecraft:fortune");  // 3
bool has = container.has("minecraft:fortune");        // true

// 兼容性检查
bool canAdd = container.canAdd("minecraft:silk_touch");  // false（与时运互斥）

// 遍历
for (const auto& instance : container.getAll()) {
    const Enchantment* enchant = instance.getEnchantment();
    // ...
}
```

#### 使用工具类

```cpp
// 检查物品附魔
bool hasSilkTouch = EnchantmentHelper::hasSilkTouch(stack);
i32 fortuneLevel = EnchantmentHelper::getFortuneLevel(stack);

// 计算伤害加成
f32 damageBonus = EnchantmentHelper::getTotalDamageBonus(stack, entityType);

// 计算保护值
i32 protection = EnchantmentHelper::getTotalProtection(stack, damageType);
```

#### 计算时运加成

```cpp
math::Random random(seed);

// 钻石矿掉落（时运 III）
i32 diamonds = FortuneEnchantment::applyBonus(1, 3, random);  // 1-4

// 煤矿石掉落（均匀分布）
i32 coal = 1 + FortuneEnchantment::applyUniformBonus(3, random);  // 1-4

// 红石矿掉落（矿石类型）
i32 redstone = FortuneEnchantment::applyOreDropBonus(4, 5, 3, random);  // 4-8
```

### 容易踩的坑

#### 1. 忘记初始化注册表

```cpp
// 错误：使用前未初始化
const Enchantment* enchant = EnchantmentRegistry::get("minecraft:fortune");  // nullptr!

// 正确：先初始化
EnchantmentRegistry::initialize();
const Enchantment* enchant = EnchantmentRegistry::get("minecraft:fortune");  // OK
```

#### 2. 忽略兼容性检查

```cpp
// 错误：直接添加不检查兼容性
container.set("minecraft:fortune", 3);
container.set("minecraft:silk_touch", 1);  // 应该被阻止！

// 正确：先检查兼容性
if (container.canAdd("minecraft:silk_touch")) {
    container.set("minecraft:silk_touch", 1);
}
```

#### 3. 线程安全问题

```cpp
// 错误：多线程同时访问注册表
// 虽然注册表有内部锁，但频繁加锁影响性能

// 正确：缓存附魔指针
const Enchantment* fortune = EnchantmentRegistry::get("minecraft:fortune");
// 后续直接使用 fortune 指针，无需再次查询
```

#### 4. 未检查附魔是否存在

```cpp
// 错误：未检查返回值
const Enchantment* enchant = EnchantmentRegistry::get("minecraft:invalid");
i32 maxLevel = enchant->maxLevel();  // 崩溃！

// 正确：检查返回值
const Enchantment* enchant = EnchantmentRegistry::get("minecraft:fortune");
if (enchant) {
    i32 maxLevel = enchant->maxLevel();
}
```

#### 5. 时运计算公式混淆

```cpp
// applyBonus：用于钻石矿等，每级有概率+1
// applyUniformBonus：用于煤矿石等，0-level 均匀随机
// applyOreDropBonus：用于红石矿等，基础范围 + 额外加成

// 错误：用于错误的方块类型
i32 drop = FortuneEnchantment::applyBonus(4, 3, random);  // 红石不应该用这个

// 正确：根据方块类型选择方法
i32 coal = 1 + FortuneEnchantment::applyUniformBonus(level, random);     // 煤
i32 diamond = FortuneEnchantment::applyBonus(1, level, random);          // 钻石
i32 redstone = FortuneEnchantment::applyOreDropBonus(4, 5, level, random); // 红石
```

### 涉及的测试用例

测试文件：`tests/common/item/enchantment/EnchantmentTest.cpp`

**测试覆盖：**

| 测试套件 | 测试用例 |
|---------|---------|
| EnchantmentRegistryTest | InitializeRegistersEnchantments |
| | GetFortuneEnchantment |
| | GetSilkTouchEnchantment |
| | GetNonExistentEnchantment |
| | HasEnchantment |
| | DoubleInitializeIsSafe |
| FortuneEnchantmentTest | GetMinCost |
| | GetMaxCost |
| | IsIncompatibleWithSilkTouch |
| | ApplyBonus |
| | ApplyUniformBonus |
| | ApplyOreDropBonus |
| SilkTouchEnchantmentTest | Properties |
| | GetMinCost |
| EnchantmentContainerTest | EmptyContainer |
| | SetAndGetEnchantment |
| | UpdateEnchantment |
| | RemoveEnchantment |
| | ClearEnchantments |
| | MultipleEnchantments |
| | CompatibilityCheck |
| | HasEnchantmentType |
| EnchantmentTest | RarityWeight |
| | GetNameKey |
| EnchantmentInstanceTest | GetEnchantment |

**测试要点：**
1. 注册表初始化和查询
2. 附魔属性正确性（ID、等级范围、类型、稀有度）
3. 附魔互斥性（时运与精准采集不兼容）
4. 时运三种计算方法
5. 容器的增删改查和兼容性检查
6. 附魔实例与注册表的关联

## 扩展指南

### 添加新附魔

1. 在 `enchantments/` 目录创建新的附魔类：

```cpp
// enchantments/SharpnessEnchantment.hpp
#pragma once

#include "../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

class SharpnessEnchantment : public Enchantment {
public:
    [[nodiscard]] String id() const override {
        return "minecraft:sharpness";
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 5;
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Weapon;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Common;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 1 + (level - 1) * 11;
    }

    [[nodiscard]] f32 getDamageBonus(i32 level, u32 entityType) const override {
        return static_cast<f32>(level) * 1.25f;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
```

2. 在 `EnchantmentRegistry.cpp` 中注册：

```cpp
#include "enchantments/SharpnessEnchantment.hpp"

void EnchantmentRegistry::initialize() {
    // ...
    registerEnchantmentInternal(std::make_unique<SharpnessEnchantment>());
    // ...
}
```

3. 添加对应测试用例。

## 参考

- Minecraft Java Edition 1.16.5 源码：`net.minecraft.enchantment`
- Minecraft Wiki：https://minecraft.fandom.com/wiki/Enchanting
