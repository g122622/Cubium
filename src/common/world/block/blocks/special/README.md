# 特殊方块模块 (Special Blocks)

特殊方块模块提供管理、装饰和特殊功能方块的实现，包括屏障、结构方块、命令方块、粘液块、蜂蜜块、海绵、蜘蛛网等。

## 目录结构

```
special/
├── README.md            # 本文档
├── SpecialBlocks.hpp    # 所有特殊方块声明（屏障、结构、命令、物理、功能方块）
└── SpecialBlocks.cpp    # 所有特殊方块实现
```

## 内部模块关系

所有特殊方块定义在 `mc::blocks` 命名空间下，继承自 `Block` 基类：

```
Block (基类)
├── BarrierBlock          # 屏障
├── StructureVoidBlock    # 结构空位
├── StructureBlock        # 结构方块
├── JigsawBlock           # 拼图方块
├── CommandBlock          # 命令方块基类
│   ├── RepeatingCommandBlock  # 循环命令方块
│   └── ChainCommandBlock      # 连锁命令方块
├── SlimeBlock            # 粘液块
├── HoneyBlock            # 蜂蜜块
├── SpongeBlock           # 海绵
├── WetSpongeBlock        # 湿润海绵
└── WebBlock              # 蜘蛛网
```

**类之间关系：**
- `CommandBlock` 是抽象基类，`RepeatingCommandBlock` 和 `ChainCommandBlock` 继承它
- `SpongeBlock` 和 `WetSpongeBlock` 是两种独立方块，吸水后通过 `VanillaBlocks::WET_SPONGE` 切换
- `SlimeBlock` 和 `HoneyBlock` 都实现粘性方块接口（`isStickyBlock`、`canStickTo`）

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `world/WorldEvents` | 世界事件常量（BREAK_BLOCK_EFFECTS、WET_SPONGE_DRY） |
| `world/fluid/FluidTags` | 流体标签检测（水标签） |
| `world/block/IBucketPickupHandler` | 水源舀取接口 |
| `world/block/blocks/LiquidBlock` | 液体方块类型检测 |
| `world/blockentity/CommandBlockEntity` | 命令方块实体 |
| `world/dimension/DimensionType` | 维度类型检测（isUltraWarm） |
| `world/redstone/RedstonePower` | 红石信号检测 |
| `world/tick/manager/TickManager` | tick 调度 |
| `world/gen/jigsaw/JigsawOrientation` | 拼图方块方向 |
| `util/Direction` | 方向遍历 |
| `physics/PhysicsConstants` | 物理常量（弹跳系数、减速系数） |
| `physics/collision/CollisionShape` | 碰撞形状 |

### 依赖本模块的外部模块

| 模块 | 用途 |
|------|------|
| `world/block/registry/VanillaBlocks` | 方块注册（所有特殊方块通过此注册） |
| `world/block/registry/BlockRegistry` | 方块注册系统 |

## 容易踩的坑

### 海绵吸水

1. **吸水限制**：最大搜索深度 6 格，最多吸收 65 个水方块，超过立即停止
2. **海洋植物掉落**：海绵吸收海带、海带茎、海草、高海草时，会先调用 `Block::dropResources()` 生成掉落物再移除方块。海泡菜不会被海绵吸收（MC 原版行为：只检查 KELP/KELP_PLANT/SEAGRASS/TALL_SEAGRASS 四种方块）
3. **触发时机**：`onBlockAdded()` 和 `neighborChanged()` 都会尝试吸水

### 粘液块与蜂蜜块粘连

1. **粘液块可以粘住粘液块和蜂蜜块**：`canStickTo` 返回 `other.isStickyBlock(other)`
2. **蜂蜜块只能粘住蜂蜜块**：不能与粘液块粘连，使用 `other.is(VanillaBlocks::HONEY_BLOCK)` 检测
3. **活塞推动时需检查粘连关系**：调用 `isStickyBlock()` 和 `canStickTo()` 判断

### 命令方块

1. **三种模式通过 BlockEntity 区分**：`CommandBlockEntity` 使用 `CommandBlockMode` 枚举（Redstone/Auto/Sequence）
2. **脉冲模式才响应红石上升沿**：循环模式和连锁模式不通过 `neighborChanged` 直接触发
3. **连锁执行最大长度**：`MAX_CHAIN_LENGTH = 65536`，防止无限循环

### 湿润海绵干燥

- **仅在超热维度生效**：通过 `world.isUltraWarm()` 判断（下界返回 true）
- **放置时立即变干**：`onBlockAdded()` 中检测并转换

### 蜘蛛网减速

- **水平减速**：速度 × `physics::COBWEB_SLOWDOWN_XZ`（0.025）
- **垂直减速**：仅下落时减速，速度 × `physics::COBWEB_SLOWDOWN_Y`（0.05）
- **无碰撞**：`getCollisionShape()` 返回空形状

### 蜂蜜块碰撞箱

- **碰撞箱高度为 15/16**：`CollisionShape::box(0, 0, 0, 1, 0.9375f, 1)`
- **滑度为默认值 0.6**（MC 中蜂蜜块不修改 friction，减速通过 speedFactor=0.4 和 jumpFactor=0.5 实现）

### StructureBlock MODE 属性

- **使用 BlockStateProperties::STRUCTURE_MODE()**：不是自定义属性
- **放置时默认 Data 模式**：`getStateForPlacement()` 返回 `defaultState()`
