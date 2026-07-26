# 流体系统 (Fluid System)

## 概述

流体系统实现了 Minecraft 1.16.5 风格的流体物理模拟，包括水和岩浆的流动、源头形成、流体替换等核心机制。当前实现已经把岩浆 tick、衰减和扩散距离与世界维度上下文绑定，并通过液体方块把随机 tick 继续传递到流体层。

## 目录结构

```
src/common/world/fluid/
├── Fluid.hpp/cpp              # 流体基类、流体状态和流体常量（SOURCE_LEVEL, MAX_AMOUNT）
├── FlowingFluid.hpp/cpp       # 流动流体抽象类（核心流动算法）
├── FluidRegistry.hpp/cpp      # 流体注册表（单例）
├── Fluids.hpp/cpp             # 内置流体静态访问器（类似 VanillaBlocks）
├── FluidTags.hpp/cpp          # 流体标签系统
├── README.md                  # 本文档
└── fluids/                    # 具体流体实现
    ├── EmptyFluid.hpp/cpp     # 空流体（表示无流体状态）
    ├── WaterFluid.hpp/cpp     # 水流体（源头和流动）
    └── LavaFluid.hpp/cpp      # 岩浆流体（源头和流动）
```

## 内部模块关系

```
Fluid（基类）
  └── FlowingFluid（流动算法）
        ├── WaterFluid（水）── WaterSourceFluid / WaterFlowingFluid
        └── LavaFluid（岩浆）── LavaSourceFluid / LavaFlowingFluid

FluidRegistry ←── Fluids（静态访问器）
FluidTags ←── Fluid（标签分类）
```

**核心数据流**：
- `FluidRegistry` 管理所有流体注册，`Fluids` 提供快速静态访问
- `FluidState` 是不可变状态对象，通过 `StateHolder` 管理属性
- `FlowingFluid` 实现流动算法，具体流体（水/岩浆）实现维度差异化行为
- `FluidTags` 提供水/岩浆标签，用于快速类型判断

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

## 上下游外部依赖关系

### 被谁依赖（上游调用方）

| 模块 | 使用方式 |
|------|----------|
| `LiquidBlock` | 液体方块持有流体引用，转发 tick 和随机 tick |
| `WaterLoggableHelpers` | 水logged 方块状态管理 |
| `BucketItem`, `FishBucketItem` | 水桶物品放置流体 |
| `ServerWorld` | 流体 tick 调度、世界设置方块时触发流体更新 |
| `TickManager` | `scheduleFluidTick()` 调度接口 |
| `ChunkMesher` | 读取流体状态进行网格渲染 |
| `Entity` (BoatEntity, StriderEntity) | 实体与流体交互（浮力、行走） |
| `Explosion` | 爆炸时检测流体抗性 |
| `IceBlock`, `CoralBlock`, `BubbleColumnBlock` | 特殊方块与流体交互 |

### 依赖哪些模块（下游依赖）

| 模块 | 用途 |
|------|------|
| `IWorld` | 方块/流体状态查询、调度 tick |
| `Block`, `BlockState`, `Material` | 判断流体能否流动、替换方块 |
| `CollisionShape` | 检测方块间空隙 |
| `StateContainer`, `FluidProperties` | 流体属性系统（LEVEL_1_8, FALLING） |
| `ResourceLocation` | 流体资源位置标识 |
| `Direction` | 流动方向枚举 |
| `VanillaBlocks` | 获取 WATER/LAVA 方块状态 |

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

岩浆在不同维度有不同行为：

| 维度 | Tick 延迟 | 衰减 | 斜坡搜索距离 |
|------|-----------|------|----------|
| 主世界 | 30 tick | 2 | 2 格 |
| 下界 | 10 tick | 1 | 4 格 |

### 6. 碰撞形状检测

`doesSideHaveHoles()` 使用简化实现，未来需要完整的 VoxelShape 检测。

### 7. 注册顺序依赖

`WaterSourceFluid::getFlowing()` 和 `WaterFlowingFluid::getStill()` 相互引用，需要在注册后才能正确缓存。

### 8. VanillaBlocks 依赖

`getBlockState()` 依赖 `VanillaBlocks::WATER` 和 `VanillaBlocks::LAVA`，需要在方块注册后才能正确工作。

### 9. 流体流动判定区分

**问题**：流体流动判定必须区分"目标流体状态"和"用于阻挡判断的流体类型"，否则容器方块和特殊替换规则会偏离原版语义。

**解决方案**：修改 `FlowingFluid::canFlow()` / `canFlowInto()` 时，不能把所有路径都硬塞成 `*this`。

### 10. 液体方块随机 tick 透传

**问题**：液体方块的随机 tick 必须透传给流体层，漏掉后会出现"方块看起来对了，但行为不触发"的假正确。

**解决方案**：`LiquidBlock::ticksRandomly()` 和 `LiquidBlock::randomTick()` 是岩浆火焰扩散的入口，必须正确实现。

### 11. 岩浆时序世界相关

**问题**：岩浆时序硬编码会导致主世界和下界行为不一致。

**解决方案**：岩浆时序是世界相关的。`ServerWorld::setBlockState()` 和流体 tick 调度要继续使用 `fluid.getTickDelay(*this)`，不要把主世界/下界差异重新硬编码回固定常量。

### 12. 取流体默认状态必须走 fluidId 路径

**问题**：`FluidState::stateId` 在各 Fluid 的 `StateContainer` 内**独立从 0 分配、不全局唯一**。曾经存在的 `Fluid::getFluidState(u32 stateId)` / `FluidRegistry::m_statesById` 按 stateId 反查，后注册流体会覆盖先注册的，`getFluidState(0)` 实际返回 flowing_lava 而非调用者以为的 EMPTY，是陷阱式 API。曾被 `PlayerMovementTest` 等大量测试桩误用，掩盖了"空流体处返回岩浆"的系统性 latent bug。

**解决方案**：取空/水/岩浆默认状态一律走 fluidId 路径：`&Fluids::EMPTY()->defaultState()` / `&Fluids::WATER()->defaultState()` / `&Fluids::LAVA()->defaultState()`。stateId 反查 API（`Fluid::getFluidState(u32)` / `FluidRegistry::getFluidState(u32)` / `m_statesById` / `fluidStateCount()`）已删除，按 stateId 反查会编译失败。

