# 苍白花园方块 (Pale Garden Blocks)

本目录包含苍白花园生物群系特有的方块实现。

## 目录结构

```
pale_garden/
├── CreakingHeartBlock.hpp       # 嘎枝之心方块
├── CreakingHeartBlock.cpp
├── PaleHangingMossBlock.hpp     # 苍白垂苔方块
├── PaleHangingMossBlock.cpp
├── EyeblossomBlock.hpp          # 眼眸花方块（开/合双状态 + 昼夜节律切换）
├── EyeblossomBlock.cpp
├── EyeblossomEnvironment.hpp    # 眼眸花环境属性查询工具（EYEBLOSSOM_OPEN 近似实现）
└── README.md                    # 本文件
```

## 内部模块关系

```
Block
├── RotatedPillarBlock
│   └── CreakingHeartBlock
└── BushBlock
    └── FlowerBlock
        └── EyeblossomBlock           # 状态切换核心
            └── EyeblossomEnvironment  # 环境属性查询（被 EyeblossomBlock 与 FlowerPotBlock 共用）
```

PaleHangingMossBlock 直接继承 Block。

## 外部依赖关系

- `Block`: 基础方块类
- `RotatedPillarBlock`: 轴向旋转方块基类
- `FlowerBlock`: 花朵基类
- `BlockStateProperties`: 方块状态属性定义
- `CollisionShape`: 碰撞形状
- `StateContainer`: 状态容器
- `IWorld::dayTimeOfDay()` / `DimensionType::hasFixedTime()`: 昼夜与维度判断
- `ServerWorld::addTrailParticle()`: 转换粒子的服务端广播
- `world.tickManager().scheduleBlockTick()`: 连锁触发的延迟 tick 调度
- `SoundEvents::BLOCK_EYEBLOSSOM_*`: 眼眸花切换音效
- `gameevent::GameEvents::BLOCK_CHANGE`: 切换时触发的游戏事件
- `FlowerPotBlock`: 复用 `EyeblossomEnvironment` 与 `EyeblossomBlock::spawnTransformParticle`

## 容易踩的坑

1. **状态容器初始化**: 使用 Builder 模式在构造函数中初始化，`fillStateContainer` 保持空实现（因为状态容器已在构造函数中创建）。

2. **PaleHangingMossBlock 的支撑逻辑**: `isValidPosition` 检查上方方块是否有向下的实心面（`isSolidSide`），或上方方块本身是苍白垂苔（允许链式悬挂）。`updatePostPlacement` 在支撑变化时调度tick销毁方块，并更新TIP属性。

3. **EyeblossomBlock 的光照**: 只有开放状态才发光（等级1），关闭状态不发光。光照等级通过 `Type::Open`/`Type::Closed` 判断，而非 BlockState。

4. **EyeblossomBlock 的状态切换与维度**：仅主世界（`hasFixedTime() == false`）的眼眸花会响应昼夜节律切换状态；下界/末地的 `EYEBLOSSOM_OPEN` 环境属性为 `TriState::Default`，`triStateToBoolean` 会回退到当前状态，因此不切换。务必在新增维度判断逻辑时复用 `EyeblossomEnvironment::getEyeblossomOpen`，避免直接调用 `dayTimeOfDay()`。

5. **EnvironmentAttributes 系统尚未完整实现**：当前 `EyeblossomEnvironment` 是 MC 1.21.11 `EnvironmentAttributes.EYEBLOSSOM_OPEN` + `Timelines.IN_OVERWORLD` 的轻量近似实现。未来完整移植 EnvironmentAttributes 系统（含 EnvironmentAttribute / EnvironmentAttributeMap / EnvironmentAttributeSystem / Timeline / KeyframeTrack 等）后，应替换为本项目的 `world.environmentAttributes().getValue(EnvironmentAttributes::EYEBLOSSOM_OPEN, pos)`。

6. **连锁触发范围是 3×2×3**：`tryChangingState` 中 `BlockPos.forEachBetween` 遍历的范围是 `pos + [-3,-2,-3] .. pos + [3,2,3]`（含两端），与 MC 1.21.11 一致。注意是闭区间，半径分别为 3/2/3，不要误写为 3/3/3。

7. **花盆版眼眸花不连锁触发**：`FlowerPotBlock::randomTick` 仅切换自身并播放长音效，不调用 `tryChangingState`、不调度周围 tick、不触发 `BLOCK_CHANGE` 游戏事件。这与 MC 1.21.11 行为一致。

