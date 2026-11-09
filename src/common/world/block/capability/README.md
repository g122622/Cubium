# 方块可选能力和跨行为族复用的实现

## 目录结构树

```text
capability/
├── oxidation/  # 跨方块族的氧化识别与阶段变化
│   ├── IOxidizableBlock.hpp  # 氧化等级识别与曼哈顿距离氧化算法接口（实现在同名 .cpp）
│   └── WeatheringCopperBlock.hpp  # 铜系方块基类：Block + IOxidizableBlock，另含涂蜡变体
├── GameMasterBlock.hpp  # 管理员方块标记接口（无成员）
├── IBeaconBeamColorProvider.hpp  # 信标光束取色接口 + 染料到 RGB 的转换
├── IBlockAnimateContext.hpp  # 传给 Block::animateTick 的客户端上下文（非方块能力）
├── IBucketPickupHandler.hpp  # 空桶拾取（pickupFluid / pickupItem / getPickupSound）
├── IGrowable.hpp  # 骨粉催熟（canGrow / canUseBonemeal / grow，含骨粉粒子类型）
├── ILiquidContainer.hpp  # 液体容器（canContainFluid / receiveFluid / containsFluid）
├── ILiquidSealed.cpp  # 密封容器实现（恒不接收、恒不包含）
├── ILiquidSealed.hpp  # 永不接收流体、也不被流体替换的容器接口
├── IWaterLoggable.cpp  # 含水接口的默认实现
├── IWaterLoggable.hpp  # 含水方块：IBucketPickupHandler + ILiquidContainer 的组合接口
└── WaterLoggableHelpers.hpp  # 含水辅助（水流体缓存、水源判定、调度流体 tick）
```

## 内部模块关系

本目录是**运行期可选行为**层：接口均为纯抽象基类（无数据成员、虚析构），由方块子类按需多继承；
消费方不查询任何标志位，直接 `dynamic_cast<接口*>(&block)`，未实现即得到 `nullptr`。因此新增能力不需要
改 `Block` 基类，也不会给不支持的方块带来开销。

派生关系：`IWaterLoggable : IBucketPickupHandler, ILiquidContainer`，实现它即同时获得桶舀与流体接收能力；
`ILiquidSealed : ILiquidContainer`，与 `IWaterLoggable` 构成"永不接收 / 总是接收"的对立实现。
`IBlockAnimateContext` 是唯一方向相反的接口：它由 `ClientWorld` 实现、传给 `Block::animateTick`，不是方块 mixin。
`oxidation/` 是"跨方块族复用实现"的样板：接口只提供识别与算法，具体族基类 `WeatheringCopperBlock` 多继承
`Block + IOxidizableBlock`。

与 `../properties/` 的分工：properties 是注册期摊平进 Block 的静态配置（方块"是什么"），capability 是运行期
查询的动态行为（方块"能做什么"）；两者无继承或包含关系，唯一交叉点是 `BlockProperties::suffocating()` /
`viewBlocking()` 接收 `StatePredicate`。

## 上下游外部依赖关系

消费方：`BoneMealItem`（IGrowable）、`BucketItem`（IBucketPickupHandler / ILiquidContainer）、
`FlowingFluid`（ILiquidContainer）、`AxeItem`（IOxidizableBlock）、破坏权限逻辑（GameMasterBlock）、
信标渲染（IBeaconBeamColorProvider）。被实现的方块族位于 `../blocks/`；接口本身只依赖 `IWorld`、`BlockState`、
`BlockPos` 等前向声明，不反向依赖具体方块。

## 容易踩的坑

- **含水/密封必须走接口**：`FlowingFluid::isBlocked` 对未实现 `ILiquidContainer` 的方块只看
  `canBeReplacedByFluid()`；水下植物（kelp / seagrass 等）注册为 `.noCollision().notSolid()` 会被当成可替换方块，
  水流每个 tick 冲毁一次并生成掉落物。必须实现 `ILiquidSealed`。
- **`FlowingFluid::flowInto` 的 `ILiquidContainer` 分支必须独占接管**：`receiveFluid` 后直接返回，
  否则会落到"替换方块 + 生成掉落物"路径。
- **`IWaterLoggable` 已组合好两个父接口**：实现它即同时获得桶舀与流体接收能力，不必分别继承。
- **`IBlockAnimateContext` 不是方块能力接口**（方向相反），不要当成"可动画方块"标记去继承。
- **接口命名空间不统一**：`IBeaconBeamColorProvider` 在 `mc::block`，`IOxidizableBlock` 在 `mc::blocks`，
  其余在 `mc`。写 `dynamic_cast` 前先确认目标接口的实际命名空间。
- **`GameMasterBlock` 是无成员纯标记接口**：判断管理员方块应使用 `Block::isGameMaster()` 虚方法，
  不要用 `dynamic_cast<GameMasterBlock>` 作为主判据。