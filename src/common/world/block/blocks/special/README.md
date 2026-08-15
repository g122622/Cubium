# 特殊方块模块 (Special Blocks)

特殊方块模块提供管理、装饰和特殊功能方块的实现，包括屏障、结构方块、命令方块、粘液块、蜂蜜块、海绵、蜘蛛网等。这些方块不归入其他功能/装饰/红石类别，多用于地图制作、命令系统、世界生成调试或特殊物理交互。

## 目录结构

```
special/
├── README.md                  # 本文档
├── BarrierBlock.hpp/cpp       # 屏障方块（不可见不可破坏，创造可见轮廓）
├── StructureVoidBlock.hpp/cpp # 结构空位（结构方块使用，不替换现有方块）
├── StructureBlock.hpp/cpp     # 结构方块（保存/加载结构，GameMasterBlock，含 MODE 属性）
├── JigsawBlock.hpp/cpp        # 拼图方块（结构生成，GameMasterBlock，含 ORIENTATION 属性）
├── CommandBlock.hpp/cpp       # 命令方块基类（GameMasterBlock，含 FACING/CONDITIONAL/POWERED 属性）
├── RepeatingCommandBlock.hpp/cpp # 循环命令方块（每 tick 执行，继承 CommandBlock）
├── ChainCommandBlock.hpp/cpp  # 连锁命令方块（链式触发，继承 CommandBlock）
├── SlimeBlock.hpp/cpp         # 粘液块（弹性 + 粘连）
├── HoneyBlock.hpp/cpp         # 蜂蜜块（减速 + 粘连，碰撞箱 15/16）
├── SpongeBlock.hpp/cpp        # 海绵（BFS 吸水，变湿海绵）
├── WetSpongeBlock.hpp/cpp     # 湿润海绵（下界变干 + 蒸汽效果）
└── WebBlock.hpp/cpp           # 蜘蛛网（无碰撞，减速 97.5%）
```

## 内部模块关系

所有特殊方块定义在 `mc::blocks` 命名空间下，继承自 `Block` 基类：

```
Block (基类)
├── BarrierBlock
├── StructureVoidBlock
├── StructureBlock + GameMasterBlock
├── JigsawBlock + GameMasterBlock
├── CommandBlock + GameMasterBlock
│   ├── RepeatingCommandBlock
│   └── ChainCommandBlock
├── SlimeBlock
├── HoneyBlock
├── SpongeBlock
├── WetSpongeBlock
└── WebBlock
```

类之间关系：
- `CommandBlock` 是基类（含 `protected execute()/executeChain()`），`RepeatingCommandBlock` 和 `ChainCommandBlock` 继承它并通过 `createBlockEntity()` 注入不同 `CommandBlockMode`。
- `SpongeBlock` 和 `WetSpongeBlock` 是两种独立方块，吸水后通过 `VanillaBlocks::WET_SPONGE` 切换，干燥后通过 `VanillaBlocks::SPONGE` 切换。
- `SlimeBlock` 和 `HoneyBlock` 都实现粘性方块接口（`isStickyBlock`、`canStickTo`），但粘连规则不同。

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 模块 | 用途 |
| --- | --- |
| `world/block/Block`、`world/block/GameMasterBlock`、`world/block/Material` | 方块基类与材质 |
| `world/IWorld`、`world/WorldEvent` | 世界接口与世界事件常量（BREAK_BLOCK_EFFECTS、WET_SPONGE_DRY） |
| `world/fluid/FluidTags` | 流体标签检测（水标签） |
| `world/block/IBucketPickupHandler` | 水源舀取接口 |
| `world/block/blocks/LiquidBlock` | 液体方块类型检测 |
| `world/blockentity/BlockEntity`、`world/blockentity/redstone/CommandBlockEntity` | 方块实体与命令方块实体 |
| `world/dimension/DimensionType` | 维度类型检测（isUltraWarm） |
| `world/redstone/RedstonePower`、`world/redstone/RedstoneSystem` | 红石信号检测与比较器更新 |
| `world/tick/manager/TickManager` | tick 调度 |
| `world/gen/jigsaw/JigsawOrientation` | 拼图方块方向（rotate/mirror 自由函数） |
| `util/Direction` | 方向遍历与旋转 |
| `physics/PhysicsConstants` | 物理常量（弹跳系数、减速系数、滑度） |
| `physics/collision/CollisionShape` | 碰撞形状 |
| `util/property/Properties` | `BlockStateProperties` 方块状态属性 |

### 依赖本模块的外部模块

| 模块 | 用途 |
| --- | --- |
| `world/block/registry/BuildingBlocks` | 注册 SpongeBlock、WetSpongeBlock |
| `world/block/registry/BuildingVariantBlocks` | 注册 StructureBlock、StructureVoidBlock、JigsawBlock、BarrierBlock、CommandBlock 系列 |
| `world/block/registry/NaturalBlocks` | 注册 WebBlock |

## 容易踩的坑

### 海绵吸水
1. 吸水限制：最大搜索深度 6 格，最多吸收 65 个水方块，超过立即停止。
2. 海洋植物掉落：海绵吸收海带、海带茎、海草、高海草时，会先调用 `Block::dropResources()` 生成掉落物再移除方块。海泡菜不会被海绵吸收（只检查 KELP/KELP_PLANT/SEAGRASS/TALL_SEAGRASS 四种方块）。
3. 触发时机：`onBlockAdded()` 和 `neighborChanged()` 都会尝试吸水。

### 粘液块与蜂蜜块粘连
1. 粘液块可以粘住粘液块和蜂蜜块：`canStickTo` 返回 `other.isStickyBlock(other)`。
2. 蜂蜜块只能粘住蜂蜜块：使用 `other.is(VanillaBlocks::HONEY_BLOCK)` 检测，不能与粘液块粘连。
3. 活塞推动时需检查粘连关系：调用 `isStickyBlock()` 和 `canStickTo()` 判断。

### 命令方块
1. 三种模式通过 BlockEntity 区分：`CommandBlockEntity` 使用 `CommandBlockMode` 枚举（Redstone/Auto/Sequence），由子类的 `createBlockEntity()` 注入。
2. 脉冲模式才响应红石上升沿：循环模式和连锁模式不通过 `neighborChanged` 直接触发。
3. 连锁执行最大长度：`MAX_CHAIN_LENGTH = 65536`，防止无限循环。
4. `CommandBlock::executeChain` 中通过 `dynamic_cast<const ChainCommandBlock*>` 判断下一个方块是否连锁命令方块，因此 `CommandBlock.cpp` 必须 `#include "ChainCommandBlock.hpp"`（在 .cpp 中包含，避免头文件循环依赖）。

### 湿润海绵干燥
- 仅在超热维度生效：通过 `world.isUltraWarm()` 判断（下界返回 true）。
- 放置时立即变干：`onBlockAdded()` 中检测并转换，附带蒸汽效果与火焰熄灭音效。

### 蜘蛛网减速
- 水平减速：速度 × `physics::COBWEB_SLOWDOWN_XZ`（0.25，对齐 Java CobwebBlock）。
- 垂直减速：仅下落时减速，速度 × `physics::COBWEB_SLOWDOWN_Y`（0.05）。
- 无碰撞：`getCollisionShape()` 返回空形状。

### 蜂蜜块碰撞箱
- 碰撞箱高度为 15/16：`CollisionShape::box(0, 0, 0, 1, 0.9375f, 1)`。
- 滑度为默认值 0.6（MC 中蜂蜜块不修改 friction，减速通过 speedFactor=0.4 和 jumpFactor=0.5 实现）。

### StructureBlock MODE 属性
- 使用 `BlockStateProperties::STRUCTURE_MODE()`：不是自定义属性。
- 放置时默认 Data 模式：`getStateForPlacement()` 返回 `defaultState()`。

### 三个 GameMasterBlock 的"打开界面"TODO
- `StructureBlock`、`JigsawBlock`、`CommandBlock` 的 `onBlockActivated` 在权限检查通过后均留有 `// TODO: 打开 XXX 界面`，需配合 StructureBlockEntity/JigsawBlockEntity（尚未实现）、客户端 Screen、专用网络包与 Player 上的 openXxx 方法实现，非简单调用现有 `OpenContainerPacket` 链路。
