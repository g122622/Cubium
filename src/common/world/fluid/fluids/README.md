# Fluids 模块

本目录包含 Minecraft 1.16.5 中所有流体类型的具体实现。

## 目录结构

```
fluids/
├── EmptyFluid.hpp/cpp     # 空流体（表示无流体状态）
├── WaterFluid.hpp/cpp     # 水流体（源头和流动两个类）
├── LavaFluid.hpp/cpp      # 岩浆流体（源头和流动两个类，含维度差异逻辑）
└── README.md
```

## 内部模块关系

```
Fluid (基类，定义在父目录)
    └── FlowingFluid (流动基类，定义在父目录)
            ├── WaterFluid (水基类)
            │       ├── WaterSourceFluid (水源)
            │       └── WaterFlowingFluid (流动水)
            └── LavaFluid (岩浆基类)
                    ├── LavaSourceFluid (岩浆源)
                    └── LavaFlowingFluid (流动岩浆)
    └── EmptyFluid (空流体，直接继承 Fluid)
```

## 上下游外部依赖

**上游依赖（本模块使用）：**
- `../Fluid.hpp` - 流体基类
- `../FlowingFluid.hpp` - 流动流体抽象类
- `../FluidRegistry.hpp` - 流体注册表
- `../../../util/property/FluidProperties.hpp` - 流体属性（LEVEL_1_8, FALLING）
- `../../block/VanillaBlocks.hpp` - 原版方块定义（AIR, WATER, LAVA）
- `../../IWorld.hpp` - 世界接口

**下游依赖（使用本模块）：**
- `FluidRegistry` - 注册所有内置流体
- `Fluids` - 静态访问器
- `LiquidBlock` - 液体方块，将流体状态映射为方块

## 容易踩的坑

### 1. 源头和流动流体是两个独立的类

每种流体分为源头和流动两个类：
- `XxxSourceFluid` - 源头版本，没有 LEVEL 属性，`isSource()` 返回 true
- `XxxFlowingFluid` - 流动版本，有 `LEVEL_1_8` 和 `FALLING` 属性

使用 `isEquivalentTo()` 检查是否为同种流体（水和流动水视为等效）。

### 2. 流体状态属性不同

```cpp
// 源头流体：只有 FALLING 属性（2 种状态）
WaterSourceFluid source;
source.stateContainer().stateCount();  // 2

// 流动流体：有 LEVEL_1_8 和 FALLING 属性（16 种状态）
WaterFlowingFluid flowing;
flowing.stateContainer().stateCount();  // 16
```

### 3. 方块 LEVEL 与流体 LEVEL 不同

| 流体等级 | 方块等级 | 说明 |
|----------|----------|------|
| 8 | 0 | 源头 |
| 7 | 1 | |
| ... | ... | |
| 1 | 7 | 最远端 |
| 8 (falling) | 8 | 下落流体 |

转换公式：`blockLevel = isSource ? (falling ? 8 : 0) : 8 - fluidLevel`

### 4. 岩浆与水的维度差异

岩浆在不同维度有不同行为（`LavaFluid::getTickDelay(IWorld&)` 自动处理）：

| 维度 | Tick 延迟 | 衰减 | 斜坡搜索距离 |
|------|-----------|------|----------|
| 主世界/末地 | 30 tick | 2 级 | 2 格 |
| 下界 | 10 tick | 1 级 | 4 格 |

水固定：5 tick 延迟，1 级衰减，4 格斜坡搜索距离。

### 5. getFlowing()/getStill() 缓存

这些方法使用缓存，第一次调用时会查找注册表。如果在注册表初始化之前调用，会返回 nullptr 导致崩溃。

### 6. 空流体的特殊处理

空流体没有 LEVEL 属性，`getLevel()` 始终返回 0。检查流体是否为空：
```cpp
if (state.isEmpty()) {
    // 无流体
}
```

### 7. 注册顺序依赖

`WaterSourceFluid::getFlowing()` 和 `WaterFlowingFluid::getStill()` 相互引用，需要在 `FluidRegistry::initialize()` 之后才能正确工作。

### 8. VanillaBlocks 依赖

`getBlockState()` 依赖 `VanillaBlocks::WATER` 和 `VanillaBlocks::LAVA`，需要在方块注册后才能正确工作。

### 9. 流动判定区分

流体流动判定必须区分"目标流体状态"和"用于阻挡判断的流体类型"，否则容器方块和特殊替换规则会偏离原版语义。修改 `FlowingFluid::canFlow()` / `canFlowInto()` 时不能把所有路径都硬塞成 `*this`。

### 10. 液体方块随机 tick 透传

`LiquidBlock::ticksRandomly()` 和 `LiquidBlock::randomTick()` 是岩浆火焰扩散的入口，必须正确透传给流体层。漏掉后会出现"方块看起来对了，但行为不触发"的假正确。

### 11. 岩浆时序是世界相关的

`ServerWorld::setBlockState()` 和流体 tick 调度要使用 `fluid.getTickDelay(*this)`，不要把主世界/下界差异重新硬编码回固定常量。
