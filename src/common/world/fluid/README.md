# 流体系统 (Fluid System)

## 概述

流体系统实现了 Minecraft 1.16.5 风格的流体物理模拟，包括水和岩浆的流动、源头形成、流体替换等核心机制。当前实现已经把岩浆 tick、衰减和扩散距离与世界维度上下文绑定，并通过液体方块把随机 tick 继续传递到流体层。

## 目录结构

```
src/common/world/fluid/
├── Fluid.hpp/cpp              # 流体基类和流体状态
├── FlowingFluid.hpp/cpp       # 流动流体抽象类（核心流动算法）
├── FluidRegistry.hpp/cpp      # 流体注册表（单例）
├── FluidTags.hpp/cpp          # 流体标签系统
├── FLUID_TODO.md              # 待办事项清单
└── fluids/                    # 具体流体实现
    ├── EmptyFluid.hpp/cpp     # 空流体（表示无流体状态）
    ├── WaterFluid.hpp/cpp     # 水流体（源头和流动）
    └── LavaFluid.hpp/cpp      # 岩浆流体（源头和流动）
```

## 文件详解

### Fluid.hpp/cpp - 流体基类

**职责**：定义流体系统的核心抽象，包括 `Fluid` 基类和 `FluidState` 状态类。

**主要内容**：
- `FluidState`：不可变的流体状态对象，继承自 `StateHolder<Fluid, FluidState>`
  - `isSource()` - 是否为源头
  - `getLevel()` - 获取等级（1-8）
  - `isFalling()` - 是否正在下落
  - `getHeight()` - 获取渲染高度
  - `getActualHeight()` - 获取实际高度（考虑上方流体）
  - `isEmpty()` - 是否为空流体
  - `getFlow()` - 计算流动方向向量

- `Fluid`：所有流体的抽象基类
  - 静态访问器：`getFluid(id)`, `getFluid(location)`, `getFluidState(stateId)`
  - 纯虚函数：`isSource()`, `getLevel()`, `getTickDelay()`, `canSourcesMultiply()`, `getBlockState()`, `getExplosionResistance()`
  - 世界相关函数：`getTickDelay(IWorld&)`，用于岩浆这类依赖维度的流体时序
  - 虚函数：`tick()`, `randomTick()`, `ticksRandomly()`, `isEquivalentTo()`, `isEmpty()`, `canDisplace()`, `getFlow()`, `getShape()`

**关键设计**：
- 流体状态使用属性系统（`StateContainer`），支持 `LEVEL_1_8` 和 `FALLING` 属性
- 状态对象是不可变的，状态转换返回新对象（O(1)复杂度）

### FlowingFluid.hpp/cpp - 流动流体

**职责**：实现流体流动的核心算法，包括向下流动、水平扩散、源头形成等。

**主要方法**：
- `tick()` - 流体 tick 主逻辑
- `flowAround()` - 执行流动扩散
- `spreadHorizontally()` - 水平扩散
- `calculateCorrectFlowingState()` - 计算正确状态（含源头形成检测）
- `getFlowDirections()` - 获取流动方向（按优先级排序）
- `canFlow()` / `canFlowInto()` / `canFlowDown()` - 流动可达性判定和阻挡检查
- `canFormSource()` - 检查是否可形成源头
- `getHorizontalSourceCount()` - 统计相邻源头数量
- `doesSideHaveHoles()` - 检查方块间是否有空隙

**流动规则**：
1. 优先向下流动
2. 水平扩散，距离由 `getSpreadDistance()` 决定，孔洞检测依赖相邻方块碰撞形状和阻挡规则
3. 水可以形成无限源（2+ 相邻源头且下方为固体）
4. 岩浆不能形成无限源，且主世界/末地与下界的 tick、衰减和扩散距离不同

**纯虚函数（子类实现）**：
- `getFlowing()` - 获取流动版本
- `getStill()` - 获取源头版本
- `getLevelDecrease()` - 每格高度衰减
- `getSpreadDistance()` - 最大流动距离
- `beforeReplacingBlock()` - 替换方块前的处理

### FluidRegistry.hpp/cpp - 流体注册表

**职责**：管理所有流体的注册和查找，单例模式。

**主要内容**：
- 内置流体 ID 常量：
  - `EMPTY_ID = 0` - 空流体
  - `WATER_ID = 1` - 水源头
  - `FLOWING_WATER_ID = 2` - 流动水
  - `LAVA_ID = 3` - 岩浆源头
  - `FLOWING_LAVA_ID = 4` - 流动岩浆

- 主要方法：
  - `initialize()` - 初始化并注册所有内置流体
  - `registerFluid<T>()` - 模板方法注册流体
  - `getFluid(id)` - 按 ID 查找
  - `getFluid(location)` - 按资源位置查找
  - `getFluidState(stateId)` - 按状态 ID 查找
  - `forEachFluid()`, `forEachFluidState()` - 遍历

**使用示例**：
```cpp
auto& registry = FluidRegistry::instance();
registry.initialize();

Fluid* water = Fluid::getFluid(ResourceLocation("minecraft:water"));
const FluidState& state = water->defaultState();
```

### FluidTags.hpp/cpp - 流体标签系统

**职责**：提供流体分类标签，用于快速判断流体类型。

**主要内容**：
- `FluidTag` 类 - 流体标签
  - `contains(fluid)` - 检查流体是否在标签中
  - `add()`, `addAll()` - 添加流体到标签

- `FluidTags` 类 - 内置标签集合
  - `WATER()` - 水标签（包含 water 和 flowing_water）
  - `LAVA()` - 岩浆标签（包含 lava 和 flowing_lava）
  - 当前在原版方块注册阶段初始化，避免水/岩浆判定依赖错误的加载顺序

**使用示例**：
```cpp
if (fluid.isIn(FluidTags::WATER())) {
    // 处理水相关逻辑
}
```

### fluids/EmptyFluid.hpp/cpp - 空流体

**职责**：表示无流体状态，作为默认值和占位符。

**特性**：
- `isEmpty()` 返回 `true`
- `getLevel()` 返回 0
- `isSource()` 返回 `false`
- `getBlockState()` 返回空气方块状态
- 不执行 tick

### fluids/WaterFluid.hpp/cpp - 水流体

**职责**：实现水的流动行为。

**类结构**：
- `WaterFluid` - 水流体基类
- `WaterSourceFluid` - 水源头（level=8，isSource=true）
- `WaterFlowingFluid` - 流动水（有 LEVEL_1_8 和 FALLING 属性）

**特性**：
- Tick 延迟：5 tick
- 每格衰减：1 级
- 最大距离：8 格
- 可形成无限源：是
- 爆炸抗性：100.0

**关键实现**：
- `isEquivalentTo()` - 水和流动水视为等效
- `getBlockState()` - 流体等级映射到方块等级

### fluids/LavaFluid.hpp/cpp - 岩浆流体

**职责**：实现岩浆的流动行为，包括与水的交互。

**类结构**：
- `LavaFluid` - 岩浆流体基类
- `LavaSourceFluid` - 岩浆源头
- `LavaFlowingFluid` - 流动岩浆

**特性**：
- Tick 延迟：主世界/末地 30 tick，下界 10 tick
- 每格衰减：主世界/末地 2 级，下界 1 级
- 最大距离：主世界/末地 4 格，下界 6 格
- 可形成无限源：否
- 爆炸抗性：100.0
- 随机 tick：通过 `LiquidBlock` 透传到流体层，可能引燃周围方块

**关键实现**：
- `getTickDelay(IWorld&)` - 根据世界维度返回岩浆 tick 间隔
- `canDisplace()` - 控制岩浆与水的替换规则
- `randomTick()` - 随机引燃周围可燃方块
- `checkForMixing()` - 检测与水的交互
- `flowInto()` - 重写以处理岩浆遇水生成石头/黑曜石
- `beforeReplacingBlock()` - 触发烟雾和嘶嘶声效果

## 模块职责

### 整体职责

流体模块负责：
1. 定义流体类型和状态（水、岩浆、空流体）
2. 实现流体流动物理（向下流动、水平扩散、源头形成）
3. 管理流体注册和查找
4. 提供流体标签分类系统

### 输入

| 输入项 | 来源 | 说明 |
|--------|------|------|
| IWorld | 世界系统 | 提供方块状态、流体状态查询和修改接口 |
| BlockPos | 世界系统 | 位置坐标 |
| BlockState | 方块系统 | 方块状态，用于判断流体能否流动 |
| Material | 方块系统 | 材质属性，判断可燃性、固体等 |
| CollisionShape | 物理系统 | 碰撞形状，用于判断流体能否穿过 |

### 输出

| 输出项 | 目标 | 说明 |
|--------|------|------|
| FluidState | 世界系统 | 当前位置的流体状态 |
| BlockState | 世界系统 | 流体对应的方块状态 |
| 流动调度 | 世界系统 | 通过 `scheduleFluidTick()` 调度下次流动 |

### 依赖项

```
流体模块依赖：
├── common/core/Types.hpp           # 基础类型
├── common/resource/ResourceLocation.hpp  # 资源位置
├── common/util/property/           # 属性系统
│   ├── StateContainer.hpp          # 状态容器
│   ├── FluidProperties.hpp         # 流体属性
│   └── Properties.hpp              # 方块属性
├── common/world/
│   ├── IWorld.hpp                  # 世界接口
│   ├── block/
│   │   ├── Block.hpp               # 方块基类
│   │   ├── BlockState.hpp          # 方块状态
│   │   ├── Material.hpp            # 材质定义
│   │   └── VanillaBlocks.hpp       # 原版方块
│   └── BlockPos.hpp                # 位置
├── common/physics/collision/
│   └── CollisionShape.hpp          # 碰撞形状
└── common/util/
    ├── Direction.hpp               # 方向枚举
    └── math/Vector3.hpp            # 向量
```

## 使用方法

### 初始化

```cpp
// 初始化流体注册表
FluidRegistry::instance().initialize();

// 初始化流体标签
FluidTags::initialize();
```

### 获取流体

```cpp
// 按 ID 获取
Fluid* water = Fluid::getFluid(FluidRegistry::WATER_ID);

// 按资源位置获取
Fluid* lava = Fluid::getFluid(ResourceLocation("minecraft:lava"));

// 获取默认状态
const FluidState& state = water->defaultState();
```

### 流体状态操作

```cpp
const FluidState& state = water->defaultState();

// 查询属性
bool isSource = state.isSource();     // true
i32 level = state.getLevel();         // 8
bool falling = state.isFalling();     // false
f32 height = state.getHeight();       // 8/9.0f

// 状态转换（流动水）
Fluid& flowingWater = Fluid::getFluid(ResourceLocation("minecraft:flowing_water"));
const FluidState& state5 = flowingWater.defaultState()
    .with(FluidProperties::LEVEL_1_8(), 5)
    .with(FluidProperties::FALLING(), false);
```

### 流体标签检查

```cpp
if (fluid.isIn(FluidTags::WATER())) {
    // 是水
}
if (fluid.isIn(FluidTags::LAVA())) {
    // 是岩浆
}
```

### 流体 tick

```cpp
// 流体 tick 由世界系统调用
void IWorld::scheduleFluidTick(const BlockPos& pos, Fluid& fluid, i32 delay);

// 在世界 tick 中执行
void World::tickFluids() {
    m_fluidTickList.tick(*this);
}
```

## 容易踩的坑

### 1. 流体与方块的等级映射

**问题**：流体等级和方块等级不同！

| 流体等级 | 方块等级 | 说明 |
|----------|----------|------|
| 8 | 0 | 源头 |
| 7 | 1 | |
| 6 | 2 | |
| ... | ... | |
| 1 | 7 | 最远端 |
| 8 (falling) | 8 | 下落流体 |

**转换公式**：`blockLevel = isSource ? (falling ? 8 : 0) : 8 - fluidLevel`

### 2. 源头与流动版本的区分

每种流体有两个类：
- `XxxSourceFluid` - 源头版本，没有 `LEVEL` 属性
- `XxxFlowingFluid` - 流动版本，有 `LEVEL_1_8` 和 `FALLING` 属性

使用 `isEquivalentTo()` 检查是否为同种流体（水和流动水视为等效）。

### 3. 流动状态计算

`calculateCorrectFlowingState()` 计算位置应有的流体状态：
- 检查四个水平方向的流体等级
- 检测源头形成条件（水需要 2+ 相邻源头）
- 检测上方下落流体
- 计算衰减后的等级

### 4. 流动优先级

流体按以下优先级流动：
1. 向下流动（总是优先）
2. 水平扩散（按衰减值最小方向优先）

### 5. 岩浆维度差异

岩浆在不同维度有不同行为（尚未完全实现）：

| 维度 | Tick 延迟 | 衰减 | 最大距离 |
|------|-----------|------|----------|
| 主世界 | 30 tick | 2 | 4 格 |
| 下界 | 10 tick | 1 | 6 格 |

### 6. 碰撞形状检测

`doesSideHaveHoles()` 使用简化实现，未来需要完整的 VoxelShape 检测。

### 7. 注册顺序依赖

`WaterSourceFluid::getFlowing()` 和 `WaterFlowingFluid::getStill()` 相互引用，需要在注册后才能正确缓存。

### 8. VanillaBlocks 依赖

`getBlockState()` 依赖 `VanillaBlocks::WATER` 和 `VanillaBlocks::LAVA`，需要在方块注册后才能正确工作。

## 涉及的测试用例

测试文件：`tests/common/world/fluid/FluidTest.cpp`

### 已有测试

| 测试套件 | 测试用例 | 说明 |
|----------|----------|------|
| FluidStateTest | EmptyFluidStateIsEmpty | 空流体状态检查 |
| FluidStateTest | GetFluidReturnsOwner | 状态归属检查 |
| FluidStateTest | FluidId | 流体 ID 检查 |
| FluidStateTest | StateWithProperties | 属性状态测试 |
| FluidRegistryTest | InitializeRegistersEmptyFluid | 注册表初始化 |
| FluidRegistryTest | GetFluidByInvalidIdReturnsNull | 无效 ID 返回空 |
| FluidRegistryTest | GetFluidByInvalidResourceLocationReturnsNull | 无效位置返回空 |
| FluidPropertiesTest | LevelPropertyHasCorrectRange | LEVEL 属性范围 |
| FluidPropertiesTest | FallingPropertyExists | FALLING 属性存在 |
| FluidTest | DefaultTickDoesNothing | 默认 tick 无操作 |
| FluidTest | DefaultRandomTickDoesNothing | 默认随机 tick 无操作 |
| FluidTest | DefaultTicksRandomlyReturnsFalse | 默认不随机 tick |
| FluidTest | IsEquivalentTo | 等效性检查 |
| ResourceLocationHashTest | CanBeUsedInUnorderedMap | 哈希使用 |

### 待补充测试

根据 `FLUID_TODO.md`，以下测试待实现：
1. **FlowingFluidTest** - 流动逻辑、源头形成、衰减计算
2. **WaterFluidTest** - 水流动、无限源形成
3. **LavaFluidTest** - 岩浆流动、火焰生成、水交互
4. **FluidTickListTest** - 调度、执行、优先级
5. **LiquidBlockTest** - 方块-流体映射、放置、移除

## 扩展指南

### 添加新流体

1. 继承 `FlowingFluid` 创建流动流体类
2. 创建源头和流动两个版本
3. 实现必要的纯虚函数
4. 在 `FluidRegistry::initialize()` 中注册

```cpp
class MyFluid : public FlowingFluid {
public:
    i32 getTickDelay() const override { return 10; }
    i32 getLevelDecrease(IWorld& world) const override { return 1; }
    i32 getSpreadDistance(IWorld& world) const override { return 6; }
    bool canSourcesMultiply() const override { return false; }
    // ...
};
```

### 添加新流体标签

```cpp
// 在 FluidTags 类中添加
static FluidTag& MY_TAG();

// 在 initialize() 中注册
MY_TAG().addAll({
    ResourceLocation("minecraft:my_fluid"),
    ResourceLocation("minecraft:flowing_my_fluid")
});
```

## 参考资料

- Minecraft 1.16.5 源码：`net.minecraft.fluid.*`
- 流动算法：`net.minecraft.fluid.FlowingFluid`
- 属性系统：`net.minecraft.state.*`
