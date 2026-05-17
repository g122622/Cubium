# Dispense 模块

发射器行为系统，定义物品被发射器发射时的行为。

## 目录结构

```
dispense/
├── IDispenseItemBehavior.hpp      # 发射行为接口和基类
├── IDispenseItemBehavior.cpp      # 发射行为实现
├── DispenseItemBehaviorRegistry.hpp # 发射行为注册表头文件
├── DispenseItemBehaviorRegistry.cpp # 发射行为注册表实现
└── README.md                       # 本文档
```

## 文件详细介绍

### IDispenseItemBehavior.hpp

**职责**：定义发射行为的接口和基类。

**主要内容**：

#### IDispenseItemBehavior 接口

```cpp
class IDispenseItemBehavior {
public:
    virtual ~IDispenseItemBehavior() = default;

    // 执行发射行为
    virtual ItemStack dispense(IWorld& world, const BlockPos& pos,
                               const BlockState& state, ItemStack& stack) = 0;

    // 是否成功发射（某些行为可能失败）
    [[nodiscard]] virtual bool isSuccess() const { return true; }
};
```

#### DefaultDispenseItemBehavior 默认发射行为

从发射器中投掷物品到世界中，创建物品实体并设置速度。

**核心算法**（参考 MC 1.16.5）：
- 发射位置：方块中心 + 方向偏移 * 0.7
- Y轴调整：向上/向下时 -0.125，水平方向时 -0.15625
- 速度计算：
  - 基础速度：`random(0.1) + 0.2`，范围 [0.2, 0.3]
  - X/Z 方向：`gaussian() * 0.0075 * speed + direction.xOffset * baseVelocity`
  - Y 方向：`gaussian() * 0.0075 * speed + 0.2`

#### OptionalDispenseItemBehavior 可选发射行为

用于可能成功或失败的发射行为（如打火石点火、桶装满水等）。
- 成功时播放音效 1000
- 失败时播放音效 1001

#### ProjectileDispenseBehavior 投掷物发射行为

用于发射投掷物（箭矢、雪球、鸡蛋等）。

**特点**：
- 通过工厂函数创建投掷物实体
- 默认速度 1.1，偏差 6.0
- Y方向额外 +0.1 使投掷物稍向上
- 播放投掷物音效 1002

#### BoatDispenseBehavior 船发射行为

在水面放置船实体。如果目标位置不是水，则作为普通物品发射。

**特点**：
- 放置位置：方块中心 + 方向偏移 * 1.125
- 检测目标位置是否有水（通过 FluidTags::WATER）
- 支持 6 种木材类型的船

```cpp
// 注册船发射行为
registerBehavior("minecraft:oak_boat",
    std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::OAK));
```

#### BucketDispenseBehavior 桶发射行为

放置流体到世界中。继承自 OptionalDispenseItemBehavior。

**特点**：
- 需要流体系统支持（Fluid、FluidState）
- 成功时播放音效 1000，失败时播放音效 1001
- 当前为桩实现，待 IWorld 流体放置 API 完善

#### EmptyBucketDispenseBehavior 空桶发射行为

从世界中收集流体到桶中。继承自 OptionalDispenseItemBehavior。

**特点**：
- 需要检测目标位置的流体
- 成功时替换为对应流体桶
- 当前为桩实现，待 IWorld 流体收集 API 完善

#### FlintAndSteelDispenseBehavior 打火石发射行为

点燃发射器前方的方块。继承自 OptionalDispenseItemBehavior。

**特点**：
- 检查目标方块是否可点燃
- 消耗打火石耐久度
- 当前为桩实现，待方块点火 API 完善

#### BonemealDispenseBehavior 骨粉发射行为

对发射器前方的方块使用骨粉催熟效果。继承自 OptionalDispenseItemBehavior。

**特点**：
- 检查目标方块是否响应骨粉
- 成功时播放骨粉使用粒子
- 当前为桩实现，待骨粉催熟 API 完善

### DispenseItemBehaviorRegistry.hpp

**职责**：管理物品到发射行为的映射。

**主要内容**：

```cpp
class DispenseItemBehaviorRegistry {
public:
    static DispenseItemBehaviorRegistry& instance();

    // 注册发射行为
    void registerBehavior(const std::string& itemId, std::unique_ptr<IDispenseItemBehavior> behavior);

    // 获取发射行为
    [[nodiscard]] IDispenseItemBehavior* getBehavior(const ItemStack& stack) const;
    [[nodiscard]] IDispenseItemBehavior* getBehavior(const std::string& itemId) const;

    // 获取默认发射行为
    [[nodiscard]] IDispenseItemBehavior* getDefaultBehavior();

    // 初始化默认发射行为
    void initDefaultBehaviors();
};
```

## 已注册的发射行为

### 投掷物

| 物品 | 行为 | 速度 | 偏差 | 说明 |
|------|------|------|------|------|
| minecraft:arrow | 发射箭矢 | 1.1 | 6.0 | 普通箭矢，可拾取 |
| minecraft:spectral_arrow | 发射光灵箭 | 1.1 | 6.0 | 命中后使目标发光 |
| minecraft:tipped_arrow | 发射药水箭 | 1.1 | 6.0 | 从 ItemStack 读取药水效果并应用颜色 |
| minecraft:snowball | 发射雪球 | 1.1 | 6.0 | 对烈焰人造成 3 点伤害 |
| minecraft:egg | 发射鸡蛋 | 1.1 | 6.0 | 有概率孵化小鸡 |
| minecraft:ender_pearl | 发射末影珍珠 | 1.1 | 6.0 | 传送玩家至落点 |
| minecraft:experience_bottle | 发射附魔之瓶 | 1.1 | 3.0 | 更精确的投掷 |
| minecraft:splash_potion | 发射喷溅药水 | 1.1 | 6.0 | 设置 ItemStack 以便 onImpact() 读取效果 |
| minecraft:lingering_potion | 发射滞留药水 | 1.1 | 6.0 | 设置 ItemStack 以便 onImpact() 读取效果 |

### 火焰弹和烟花

| 物品 | 行为 | 速度 | 偏差 | 说明 |
|------|------|------|------|------|
| minecraft:fire_charge | 发射小火球 | 1.0 | 6.0 | 使用 SmallFireballEntity |
| minecraft:firework_rocket | 发射烟花火箭 | 0.5 | 1.0 | 使用 FireworkRocketEntity，速度较慢、偏差小 |

### 船

| 物品 | 行为 | 说明 |
|------|------|------|
| minecraft:oak_boat | 放置船 | BoatDispenseBehavior，检测水面放置 |
| minecraft:spruce_boat | 放置船 | 同上 |
| minecraft:birch_boat | 放置船 | 同上 |
| minecraft:jungle_boat | 放置船 | 同上 |
| minecraft:acacia_boat | 放置船 | 同上 |
| minecraft:dark_oak_boat | 放置船 | 同上 |

**BoatDispenseBehavior 算法**：
1. 计算船的放置位置：方块中心 + 方向偏移 * 1.125
2. 检查目标位置是否有水
3. 如果有水，在水面放置船实体
4. 如果无水，作为普通物品发射

### 桶

| 物品 | 行为 | 状态 |
|------|------|------|
| minecraft:water_bucket | 放置水 | 桩实现（待 IWorld 流体放置 API） |
| minecraft:lava_bucket | 放置岩浆 | 桩实现（待 IWorld 流体放置 API） |
| minecraft:bucket | 收集流体 | 桩实现（待 IWorld 流体收集 API） |

### 工具

| 物品 | 行为 | 状态 |
|------|------|------|
| minecraft:flint_and_steel | 点燃方块 | 桩实现（待方块点火 API） |
| minecraft:bone_meal | 催熟作物 | 桩实现（待骨粉催熟 API） |

### 其他

| 物品 | 行为 | 说明 |
|------|------|------|
| minecraft:tnt | 作为物品发射 | 使用 DefaultDispenseItemBehavior |

## 世界事件 ID

| ID | 描述 |
|----|------|
| 1000 | 发射成功音效 |
| 1001 | 发射失败音效 |
| 1002 | 投掷物发射音效 |
| 2000 | 发射烟雾粒子（数据为方向索引 0-5）|

## 使用示例

### 注册自定义发射行为

```cpp
#include "world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "world/block/dispense/IDispenseItemBehavior.hpp"

// 创建自定义发射行为
class CustomDispenseBehavior : public DefaultDispenseItemBehavior {
protected:
    ItemStack doDispense(IWorld& world, const BlockPos& pos, const BlockState& state,
                          ItemStack& stack, Direction direction,
                          f32 speed, f32 inaccuracy) override {
        // 自定义逻辑
        return DefaultDispenseItemBehavior::doDispense(world, pos, state, stack, direction, speed, inaccuracy);
    }
};

// 注册
DispenseItemBehaviorRegistry::instance().registerBehavior<CustomDispenseBehavior>(
    "minecraft:custom_item"
);
```

### 注册投掷物发射行为

```cpp
// 使用工厂函数注册投掷物
DispenseItemBehaviorRegistry::instance().registerBehavior<ProjectileDispenseBehavior>(
    "minecraft:snowball",
    [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<Entity> {
        auto entity = entity::SnowballEntity::create(&world);
        if (entity) {
            entity->setPosition(pos.x, pos.y, pos.z);
        }
        return entity;
    },
    1.1f,  // velocity
    6.0f   // inaccuracy
);
```

### 执行发射

```cpp
// 在 DispenserBlock 中
IDispenseItemBehavior* behavior = DispenseItemBehaviorRegistry::instance().getBehavior(stack);
if (!behavior) {
    behavior = DispenseItemBehaviorRegistry::instance().getDefaultBehavior();
}

ItemStack result = behavior->dispense(world, pos, state, stack);
```

## 依赖项

### 内部依赖

- `IWorld` - 世界接口（spawnEntity、playEvent、getRandom）
- `BlockPos` / `BlockState` - 方块位置和状态
- `Direction` - 方向枚举和工具函数
- `ItemStack` - 物品堆
- `ItemEntity` - 物品实体
- `ProjectileEntity` - 投掷物实体基类
- `WorldEvents` - 世界事件 ID

### 外部依赖

- `<memory>` - std::unique_ptr
- `<functional>` - std::function
- `<unordered_map>` - std::unordered_map

## 参考

- Minecraft 1.16.5: `net.minecraft.dispenser.IDispenseItemBehavior`
- Minecraft 1.16.5: `net.minecraft.dispenser.DefaultDispenseItemBehavior`
- Minecraft 1.16.5: `net.minecraft.dispenser.ProjectileDispenseBehavior`
- Minecraft 1.16.5: `net.minecraft.block.DispenserBlock`
