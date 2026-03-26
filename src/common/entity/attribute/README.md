# Attribute 模块

实体属性系统，用于管理实体的各种可修改属性值（如生命值、移动速度、攻击力等）。

参考 MC 1.16.5 的 Attribute 系统。

## 目录结构

```
src/common/entity/attribute/
├── Attribute.hpp          # 属性基类定义
├── AttributeModifier.hpp  # 属性修改器和操作类型
├── AttributeInstance.hpp  # 属性实例（管理单个属性的值和修改器）
├── AttributeMap.hpp       # 属性映射表（管理实体的所有属性）
└── Attributes.hpp         # 标准属性定义（游戏内置属性）
```

## 文件详解

### Attribute.hpp

**职责**：定义属性的基类，作为所有属性的蓝图。

**主要内容**：
- `Attribute` 类：属性定义类
  - `registryName`：属性注册名称（如 "generic.max_health"）
  - `defaultValue`：默认值
  - `minValue`：最小值
  - `maxValue`：最大值
  - `shouldSync()`：是否同步到客户端（虚函数，默认返回 true）
  - `clone()`：克隆属性（虚函数）

**核心功能**：
- 定义属性的类型信息
- 提供属性值的范围约束
- 支持属性的比较和克隆

### AttributeModifier.hpp

**职责**：定义属性修改器和修改操作类型。

**主要内容**：
- `Operation` 枚举：修改器操作类型
  - `Addition`：加法操作，直接加到基础值上
  - `MultiplyBase`：基础乘法，基于基础值计算后累加
  - `MultiplyTotal`：总计乘法，基于当前总计值计算

- `AttributeModifier` 类：属性修改器
  - `id`：修改器唯一标识符
  - `name`：修改器显示名称
  - `amount`：修改量
  - `operation`：操作类型
  - `setAmount()`：动态修改量

**操作类型计算示例**：
```
基础值 = 10
Addition +5:         10 + 5 = 15
MultiplyBase 0.5:    10 + (10 * 0.5) = 15
MultiplyTotal 0.5:   10 * 1.5 = 15
```

### AttributeInstance.hpp

**职责**：管理单个属性的基础值和所有修改器，负责计算最终属性值。

**主要内容**：
- `AttributeInstance` 类：属性实例
  - `m_attribute`：关联的属性定义
  - `m_baseValue`：基础值
  - `m_modifiers`：修改器列表
  - `m_dirty`：脏标记（用于缓存）
  - `m_cachedValue`：缓存值
  - `m_mutex`：线程安全互斥锁

**核心功能**：
- `getValue()`：计算最终属性值（带缓存）
- `setBaseValue()`：设置基础值（自动 clamp）
- `addModifier()`/`removeModifier()`：管理修改器
- `clearModifiers()`：清除所有修改器
- `hasModifier()`/`getModifier()`：查询修改器
- `isDirty()`/`markSynced()`：脏标记管理

**计算流程**（computeValue）：
1. 从基础值开始
2. 应用所有 `Addition` 操作（加法）
3. 应用所有 `MultiplyBase` 操作（基础乘法）
4. 应用所有 `MultiplyTotal` 操作（总计乘法）
5. 将结果 clamp 到属性范围

**缓存机制**：
- 使用 `m_dirty` 标记是否需要重新计算
- `getValue()` 自动处理缓存更新
- 任何修改操作都会设置 `m_dirty = true`

### AttributeMap.hpp

**职责**：管理实体的所有属性实例，提供统一的属性管理接口。

**主要内容**：
- `AttributeMap` 类：属性映射表
  - `m_instances`：属性名称 → 属性实例的映射
  - `m_mutex`：线程安全互斥锁

**核心功能**：
- `registerAttribute()`：注册新属性
- `getInstance()`：获取属性实例
- `getValue()`/`getBaseValue()`：获取属性值
- `setBaseValue()`：设置属性基础值
- `addModifier()`/`removeModifier()`：管理修改器
- `hasAttribute()`：检查属性是否存在
- `copyFrom()`：从另一个映射表复制属性值
- `clear()`：清除所有属性

**线程安全**：
- 所有公共方法都使用互斥锁保护
- `copyFrom()` 使用 `std::scoped_lock` 避免死锁

### Attributes.hpp

**职责**：定义游戏中的标准属性类型。

**主要内容**：
- 标准属性工厂函数（14 种）：
  - `maxHealth()`：最大生命值（默认 20.0，范围 0-1024）
  - `followRange()`：跟随范围（默认 32.0，范围 0-2048）
  - `knockbackResistance()`：击退抗性（默认 0.0，范围 0-1）
  - `movementSpeed()`：移动速度（默认 0.7，范围 0-1024）
  - `flyingSpeed()`：飞行速度（默认 0.4，范围 0-1024）
  - `attackDamage()`：攻击伤害（默认 2.0，范围 0-2048）
  - `attackKnockback()`：攻击击退（默认 0.0，范围 0-5）
  - `attackSpeed()`：攻击速度（默认 4.0，范围 0-1024）
  - `armor()`：护甲值（默认 0.0，范围 0-30）
  - `armorToughness()`：护甲韧性（默认 0.0，范围 0-20）
  - `luck()`：幸运（默认 0.0，范围 -1024-1024）
  - `maxAbsorption()`：最大吸收值（默认 0.0，范围 0-2048）
  - `breathMax()`：水下呼吸时间（默认 300 ticks，范围 0-6000）
  - `jumpBoost()`：跳跃高度（默认 0.42，范围 0-8）

- 属性名称常量（14 个）：
  - `MAX_HEALTH`, `FOLLOW_RANGE`, `KNOCKBACK_RESISTANCE`
  - `MOVEMENT_SPEED`, `FLYING_SPEED`, `ATTACK_DAMAGE`
  - `ATTACK_KNOCKBACK`, `ATTACK_SPEED`, `ARMOR`
  - `ARMOR_TOUGHNESS`, `LUCK`, `MAX_ABSORPTION`
  - `BREATH_MAX`, `JUMP_BOOST`

## 文件关系图

```
                    ┌──────────────────┐
                    │   Attribute.hpp  │  ← 属性定义
                    │    (基类)        │
                    └────────┬─────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
┌───────────────────┐ ┌─────────────────────┐
│AttributeModifier  │ │ AttributeInstance   │
│     .hpp          │ │      .hpp           │
│  (修改器定义)      │ │  (实例管理+计算)    │
└───────────────────┘ └──────────┬──────────┘
                                 │
                                 ▼
                      ┌─────────────────────┐
                      │  AttributeMap.hpp   │
                      │   (属性集合管理)    │
                      └─────────────────────┘
                                 │
                                 ▼
                      ┌─────────────────────┐
                      │   Attributes.hpp    │
                      │  (标准属性定义)     │
                      └─────────────────────┘
```

## 模块职责

### 整体职责

1. **属性定义**：定义游戏中可修改的属性类型
2. **属性修改**：支持多种修改器操作类型（加法、乘法）
3. **属性计算**：按特定顺序计算最终属性值
4. **属性管理**：管理实体的所有属性实例
5. **线程安全**：支持多线程环境下的安全访问

### 输入

- 属性定义（名称、默认值、范围）
- 基础值设置
- 属性修改器（ID、名称、值、操作类型）

### 输出

- 计算后的最终属性值
- 属性实例状态（脏标记、修改器列表）

### 依赖项

- `common/core/Types.hpp`：基础类型定义（String, f64, u8 等）
- `<mutex>`：线程同步
- `<memory>`：智能指针
- `<unordered_map>`：哈希映射
- `<vector>`：动态数组
- `<algorithm>`：算法函数

## 使用方法

### 1. 创建和注册属性

```cpp
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"

using namespace mc::entity::attribute;

// 创建属性映射表
AttributeMap attributes;

// 注册标准属性
attributes.registerAttribute(*Attributes::maxHealth());
attributes.registerAttribute(*Attributes::movementSpeed());
attributes.registerAttribute(*Attributes::attackDamage());

// 注册自定义属性
Attribute customAttr("custom.power", 100.0, 0.0, 1000.0);
attributes.registerAttribute(customAttr);
```

### 2. 获取和设置属性值

```cpp
// 获取属性值
f64 health = attributes.getValue(Attributes::MAX_HEALTH);
f64 speed = attributes.getValue(Attributes::MOVEMENT_SPEED, 0.1);  // 带默认值

// 设置基础值
attributes.setBaseValue(Attributes::MAX_HEALTH, 30.0);

// 获取属性实例（用于更精细的控制）
AttributeInstance* instance = attributes.getInstance(Attributes::MAX_HEALTH);
if (instance) {
    instance->setBaseValue(40.0);
    f64 value = instance->getValue();
}
```

### 3. 使用修改器

```cpp
// 创建修改器
AttributeModifier buff("buff-1", "Health Boost", 10.0, Operation::Addition);
AttributeModifier speedBoost("buff-2", "Speed Boost", 0.3, Operation::MultiplyTotal);

// 添加修改器
attributes.addModifier(Attributes::MAX_HEALTH, buff);
attributes.addModifier(Attributes::MOVEMENT_SPEED, speedBoost);

// 移除修改器
attributes.removeModifier(Attributes::MAX_HEALTH, "buff-1");

// 检查修改器
AttributeInstance* instance = attributes.getInstance(Attributes::MAX_HEALTH);
if (instance && instance->hasModifier("buff-1")) {
    const AttributeModifier* mod = instance->getModifier("buff-1");
    // 使用修改器...
}
```

### 4. 复制属性

```cpp
// 从另一个属性映射表复制属性值
AttributeMap target;
target.registerAttribute(*Attributes::maxHealth());
target.copyFrom(sourceAttributes);
```

### 5. 同步和脏标记

```cpp
AttributeInstance* instance = attributes.getInstance(Attributes::MAX_HEALTH);
if (instance && instance->isDirty()) {
    // 属性已修改，需要同步到客户端
    syncAttributeToClient(instance->getValue());
    instance->markSynced();
}
```

## 容易踩的坑

### 1. 修改器操作顺序

**问题**：修改器的应用顺序会影响最终结果。

**正确理解**：
```cpp
// 修改器按操作类型分阶段应用，而非按添加顺序
// 阶段1：所有 Addition
// 阶段2：所有 MultiplyBase
// 阶段3：所有 MultiplyTotal

// 例如：基础值 = 10
// Addition +5 → 15
// MultiplyBase 0.5 → 15 + (10 * 0.5) = 20
// MultiplyTotal 0.1 → 20 * 1.1 = 22
```

### 2. 值范围限制

**问题**：设置基础值或计算结果会被自动 clamp 到属性范围。

**注意**：
```cpp
// 属性范围 0-100
Attribute attr("test", 50.0, 0.0, 100.0);
AttributeInstance instance(attr);

instance.setBaseValue(150.0);
// 实际值是 100.0（被限制到最大值）

instance.setBaseValue(-10.0);
// 实际值是 0.0（被限制到最小值）
```

### 3. 修改器 ID 唯一性

**问题**：修改器使用 ID 进行比较和查找，相同 ID 的修改器被视为相同。

**注意**：
```cpp
AttributeModifier mod1("id-1", "Name A", 5.0, Operation::Addition);
AttributeModifier mod2("id-1", "Name B", 10.0, Operation::MultiplyBase);

// mod1 == mod2（因为 ID 相同）
// 如果先添加 mod1，再添加 mod2，会有两个相同 ID 的修改器
// 移除时会移除第一个匹配的
```

### 4. 线程安全

**问题**：AttributeMap 和 AttributeInstance 都是线程安全的，但需要正确使用。

**注意**：
```cpp
// 正确：单次操作是原子的
attributes.addModifier(Attributes::MAX_HEALTH, mod);

// 错误：多次操作不是原子的
// 需要外部同步
auto* instance = attributes.getInstance(Attributes::MAX_HEALTH);
if (instance) {
    instance->setBaseValue(20.0);
    instance->addModifier(mod);  // 这两步之间可能被其他线程打断
}
```

### 5. 属性注册

**问题**：必须先注册属性才能使用。

**注意**：
```cpp
AttributeMap attributes;

// 错误：属性未注册
attributes.setBaseValue(Attributes::MAX_HEALTH, 20.0);  // 返回 false，无效果

// 正确：先注册
attributes.registerAttribute(*Attributes::maxHealth());
attributes.setBaseValue(Attributes::MAX_HEALTH, 20.0);  // 成功
```

### 6. 复制属性

**问题**：`copyFrom()` 只复制已注册的属性。

**注意**：
```cpp
AttributeMap source;
source.registerAttribute(*Attributes::maxHealth());
source.setBaseValue(Attributes::MAX_HEALTH, 30.0);

AttributeMap target;
// target 未注册属性，copyFrom 不会注册新属性
target.copyFrom(source);  // 无效果，因为 target 没有对应属性

// 正确：先注册
target.registerAttribute(*Attributes::maxHealth());
target.copyFrom(source);  // 现在会复制值
```

## 测试用例

测试文件位于 `tests/entity/AttributeTests.cpp`，包含以下测试：

### Attribute 测试（3 个）

| 测试名称 | 测试内容 |
|---------|---------|
| `Attribute.Construction` | 属性构造、获取属性值 |
| `Attribute.Comparison` | 属性比较（基于名称） |
| `Attribute.Clone` | 属性克隆 |

### AttributeModifier 测试（3 个）

| 测试名称 | 测试内容 |
|---------|---------|
| `AttributeModifier.Construction` | 修改器构造、获取属性 |
| `AttributeModifier.SetAmount` | 动态修改修改量 |
| `AttributeModifier.Comparison` | 修改器比较（基于 ID） |

### AttributeInstance 测试（12 个）

| 测试名称 | 测试内容 |
|---------|---------|
| `AttributeInstance.Construction` | 实例构造、默认值 |
| `AttributeInstance.SetBaseValue` | 设置基础值 |
| `AttributeInstance.BaseValueClamping` | 基础值范围限制 |
| `AttributeInstance.AdditionModifier` | 加法修改器 |
| `AttributeInstance.MultipleAdditionModifiers` | 多个加法修改器 |
| `AttributeInstance.MultiplyBaseModifier` | 基础乘法修改器 |
| `AttributeInstance.MultiplyTotalModifier` | 总计乘法修改器 |
| `AttributeInstance.MixedModifiers` | 混合修改器计算顺序 |
| `AttributeInstance.RemoveModifier` | 移除修改器 |
| `AttributeInstance.ClearModifiers` | 清除所有修改器 |
| `AttributeInstance.HasModifier` | 检查修改器存在 |
| `AttributeInstance.GetModifier` | 获取修改器指针 |
| `AttributeInstance.DirtyFlag` | 脏标记行为 |

### AttributeMap 测试（8 个）

| 测试名称 | 测试内容 |
|---------|---------|
| `AttributeMap.RegisterAttribute` | 注册属性、重复注册 |
| `AttributeMap.GetValue` | 获取属性值、默认值 |
| `AttributeMap.SetBaseValue` | 设置基础值 |
| `AttributeMap.AddModifier` | 添加修改器 |
| `AttributeMap.RemoveModifier` | 移除修改器 |
| `AttributeMap.HasAttribute` | 检查属性存在 |
| `AttributeMap.MultipleAttributes` | 多属性管理 |
| `AttributeMap.CopyFrom` | 属性复制 |

### Attributes 测试（4 个）

| 测试名称 | 测试内容 |
|---------|---------|
| `Attributes.MaxHealth` | 最大生命值属性定义 |
| `Attributes.MovementSpeed` | 移动速度属性定义 |
| `Attributes.AttackDamage` | 攻击伤害属性定义 |
| `Attributes.KnockbackResistance` | 击退抗性属性定义 |

**总测试数**：30 个测试用例

## 设计模式

- **值对象模式**：Attribute 和 AttributeModifier 是不可变的值对象
- **缓存模式**：AttributeInstance 使用脏标记缓存计算结果
- **策略模式**：Operation 枚举定义了不同的修改策略
- **组合模式**：AttributeMap 组合管理多个 AttributeInstance

## 参考

- Minecraft Java Edition 1.16.5 Attribute System
- `net.minecraft.entity.ai.attributes.Attribute`
- `net.minecraft.entity.ai.attributes.AttributeInstance`
- `net.minecraft.entity.ai.attributes.AttributeModifier`
- `net.minecraft.entity.ai.attributes.AttributeMap`
- `net.minecraft.entity.ai.attributes.Attributes`
