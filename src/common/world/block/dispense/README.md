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

| 物品 | 行为 | 速度 | 偏差 |
|------|------|------|------|
| minecraft:arrow | 发射箭矢 | 1.1 | 6.0 |
| minecraft:spectral_arrow | 发射光灵箭 | 1.1 | 6.0 |
| minecraft:tipped_arrow | 发射药水箭 | 1.1 | 6.0 |
| minecraft:snowball | 发射雪球 | 1.1 | 6.0 |
| minecraft:egg | 发射鸡蛋 | 1.1 | 6.0 |
| minecraft:ender_pearl | 发射末影珍珠 | 1.1 | 6.0 |
| minecraft:experience_bottle | 发射附魔之瓶 | 1.1 | 3.0（更精确）|
| minecraft:splash_potion | 发射喷溅药水 | 1.1 | 6.0 |
| minecraft:lingering_potion | 发射滞留药水 | 1.1 | 6.0 |

### 待实现的行为

以下发射行为需要额外系统支持：

| 物品 | 所需系统 |
|------|----------|
| minecraft:fire_charge | SmallFireballEntity、火焰放置逻辑 |
| minecraft:firework_rocket | FireworkRocketEntity 烟花数据读取 |
| minecraft:*_boat | BoatEntity、水面检测 |
| minecraft:*_bucket | FluidState、流体放置逻辑 |
| minecraft:flint_and_steel | OptionalDispenseBehavior、火焰放置逻辑 |
| minecraft:bone_meal | BonemealEvent、作物催熟逻辑 |
| minecraft:tnt | TNTEntity、点燃逻辑 |
| minecraft:shulker_box | OptionalDispenseBehavior、潜影盒放置 |
| minecraft:glass_bottle | 流体检测、药水瓶填充 |
| minecraft:glowstone | 重生锚充能逻辑 |
| minecraft:shears | 蜂巢采集逻辑 |
| minecraft:*_spawn_egg | 实体生成系统 |

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
