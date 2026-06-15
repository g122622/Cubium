# Dispense 模块

发射器行为系统，定义物品被发射器发射时的行为。

## 目录结构

```
dispense/
├── IDispenseItemBehavior.hpp      # 发射行为接口和基类定义
├── IDispenseItemBehavior.cpp      # 发射行为实现
├── DispenseItemBehaviorRegistry.hpp # 发射行为注册表头文件
├── DispenseItemBehaviorRegistry.cpp # 发射行为注册表实现（含所有默认行为注册）
└── README.md                       # 本文档
```

## 内部模块关系

```
DispenseItemBehaviorRegistry
    ├── 持有并管理所有 IDispenseItemBehavior 实例
    └── 提供 registerBehavior/getBehavior 接口

IDispenseItemBehavior (接口)
    └── DefaultDispenseItemBehavior (基类：物品投掷)
        ├── OptionalDispenseItemBehavior (可能失败的行为基类)
        │   ├── BucketDispenseBehavior (放置流体，成功后替换为空桶)
        │   ├── EmptyBucketDispenseBehavior (收集流体，成功后替换为满桶)
        │   ├── FlintAndSteelDispenseBehavior (点火/点燃/引燃TNT，消耗耐久)
        │   └── BonemealDispenseBehavior (骨粉催熟/水中海草，消耗数量)
        ├── ProjectileDispenseBehavior (投掷物发射)
        └── BoatDispenseBehavior (放置船)
```

## 上下游依赖关系

### 上游依赖（谁依赖了这个模块）

- `DispenserBlock` - 发射器方块，调用 `DispenseItemBehaviorRegistry::getBehavior()` 获取发射行为
- `MinecraftServer` / `ClientApplicationBootstrap` - 启动时调用 `initDefaultBehaviors()` 初始化默认行为

### 下游依赖（这个模块依赖谁）

**实体系统：**
- `ItemEntity` - 默认发射行为和 `_spawnItemEntity` 创建物品实体
- `ProjectileEntity` 及其子类 - 投掷物发射行为
- `BoatEntity` - 船发射行为

**世界接口：**
- `IWorld` - 世界操作（spawnEntity、playEvent、getFluidState、setBlockState、getBlockState 等）
- `TickManager` - 流体 tick 调度（BucketDispenseBehavior 放置流体后调度）

**物品系统：**
- `ItemStack` - 物品堆操作（shrink、split、attemptDamageItem、getContainerItem）
- `Items` - 物品注册表
- `BucketItem` - 桶物品（getFilledBucket、getEmptyBucket）
- `FlintAndSteelItem` - 打火石静态方法（canLightBlock、getFireForPlacement）
- `BoneMealItem` - 骨粉静态方法（applyBonemeal、growSeagrass）

**方块系统：**
- `BlockState` / `BlockStateProperties::FACING()` - 获取发射方向和 LIT 属性
- `Block` / `IBucketPickupHandler` / `ILiquidContainer` - 桶交互接口
- `TNTBlock` - TNT 引燃
- `FireBlock` / `VanillaBlocks` - 火焰放置和方块检测
- `IGrowable` - 骨粉催熟接口

**流体系统：**
- `Fluid` / `FluidState` / `FluidTags::WATER()` - 流体检测和放置
- `FluidRegistry` - 流体注册表

**工具：**
- `Direction` / `Directions` - 方向枚举和工具函数
- `Random` - 高斯随机速度计算
- `Vector3` / `BlockPos` - 位置计算
- `WorldEvents` - 世界事件 ID（音效、粒子）

## 容易踩的坑

1. **发射位置计算有 Y 轴偏移调整**：水平方向发射时 Y 偏移 `-0.15625`，垂直方向发射时 Y 偏移 `-0.125`，确保物品从发射口正确射出。

2. **速度计算使用高斯扰动**：默认速度参数 `speed=6.0` 不是直接速度，而是用于高斯扰动计算 `gaussian() * 0.0075 * speed`，实际速度由方向偏移和 `baseVelocity` 决定。

3. **ProjectileDispenseBehavior 的工厂函数可能返回 nullptr**：需要在 `dispense()` 中处理创建失败的情况，回退到默认行为。

4. **BoatDispenseBehavior 需要检测水体**：不仅检测目标位置，还要检测目标位置下方是否有水，若无水则回退到默认投掷行为。

5. **桶行为放置流体必须通过 `scheduleFluidTick` 调度**：不能直接调用 `Fluid::tick()`，因为流体流动有 tick 延迟（水5tick、岩浆10/30tick），直接调用会导致流体在同一 tick 内立即流动，违反 MC 的 tick 调度机制。正确做法是 `world.tickManager().scheduleFluidTick()`。

6. **桶行为物品替换逻辑（consumeWithRemainder）**：MC 中 `consumeWithRemainder` 会尝试将替换物品放回发射器库存。本项目中 `DefaultDispenseItemBehavior::consumeWithRemainder` 实现了此逻辑：原始物品减1后，若为空则直接返回替换物品写回原槽位，若有剩余则通过 `addToInventoryOrDispense` 尝试放回发射器库存其他槽位，放不下则弹出到世界中。`dispense` 接口新增了 `IInventory* dispenserInventory` 参数以支持此功能。

7. **打火石行为不消耗物品数量，只消耗耐久**：`attemptDamageItem(1)` 减少耐久度而非数量，耐久耗尽物品自动损坏消失。骨粉则用 `shrink(1)` 减少数量。

8. **世界事件 ID**：
   - `1000` - 发射成功音效
   - `1001` - 发射失败音效（OptionalDispenseItemBehavior 使用）
   - `1002` - 投掷物发射音效
   - `2000` - 发射烟雾粒子（数据为方向索引 0-5）
   - `2005` - 骨粉粒子效果（数据为粒子数量 15）
   - `1009` - 火焰熄灭音效（水桶在超热维度蒸发时使用）

9. **注册行为时使用物品 ID 字符串**：`registerBehavior("minecraft:arrow", ...)` 使用完整物品 ID，而非 `Items::ARROW` 枚举，确保与 ItemStack 查询一致。
