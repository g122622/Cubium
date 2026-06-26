#功能方块模块(Functional Blocks)

本目录包含各种功能性方块的实现，这些方块提供了特定的游戏功能，如存储、合成、红石交互等。

##目录结构

``` functional /
├── AnvilBlock.hpp / cpp #铁砧（FallingBlock子类，X / Z不对称碰撞箱，GUI容器，下落伤害 +
        损坏降级，3个变体）
├── BedBlock.hpp / cpp #床方块（16色，双格结构，设置重生点，起床位置算法）
├── BrewingStandBlock.hpp / cpp #酿造台（药水酿造，3瓶槽位）
├── CauldronBlock.hpp / cpp #炼药锅（储水、物品清洗，4级水位）
├── CompostableItems.hpp / cpp #可堆肥物品注册表（ ~66种物品概率表）
├── ComposterBlock.hpp / cpp #堆肥桶（8层填充，产出骨粉，碰撞形状为外壁拼接：底板+四面墙壁）
├── CakeBlock.hpp / cpp #蛋糕（可食用，7片）
├── BeaconBlock.hpp / cpp #信标（增益效果，金字塔基座）
├── BarrelBlock.hpp / cpp #木桶（存储容器，6方向放置）
├── LecternBlock.hpp / cpp #讲台（书籍展示，红石脉冲+比较器信号，翻页2tick脉冲）
├── GrindstoneBlock.hpp / cpp #砂轮（修复 / 祛魔，3种附着面）
├── StonecutterBlock.hpp / cpp #切石机（石材切割配方）
├── LoomBlock.hpp / cpp #织布机（旗帜图案制作）
├── BellBlock.hpp / cpp #钟（声音 / 动画，多方向附着）
├── JukeboxBlock.hpp / cpp #唱片机（音乐播放）
├── RespawnAnchorBlock.hpp / cpp #重生锚（下界重生点，4级充能）
├── LodestoneBlock.hpp / cpp #磁石（指南针绑定）
├── CartographyTableBlock.hpp / cpp #制图台（地图复制 / 扩展）
├── CraftingTableBlock.hpp / cpp #工作台（3x3合成界面，INTERACT_WITH_CRAFTING_TABLE统计）
├── FletchingTableBlock.hpp / cpp #制箭台（箭矢制作）
├── SmithingTableBlock.hpp / cpp #锻造台（装备升级）
├── TrailsBlocks.hpp / cpp #考古方块（雕纹书架、饰纹陶罐、可疑沙 /
            砾、嗅探兽蛋）
└── README.md
```

            ##内部模块关系

``` Block(基类)
├── AnvilBlock(→ FallingBlock)
│   ├── 下落伤害（hurtEntities，每格2.0伤害，上限40）
│   ├── 损坏降级（anvil → chipped → damaged → 摧毁）
│   ├── HORIZONTAL_FACING 属性（放置时顺时针旋转90度）
│   ├── X / Z不对称碰撞箱（4段组成：底座12px、中段8×10px、窄颈4×8px、顶面10×16px）
│   ├── GUI交互（ContainerType::Anvil，无需方块实体）
│   └── 落地
            / 破碎音效（WorldEvents 1031 /
            1029）
├── BedBlock
│   ├── 双方块结构处理（HEAD/FOOT + OCCUPIED状态同步）
│   ├── getBedOrientation() — 获取床朝向
│   ├── getConnectedDirection() — 根据BED_PART获取连接方向
│   └── findStandUpPosition() — MC原版12候选位置起床算法（支持双层床、实体yaw偏航角优先级）
├── BrewingStandBlock
│   └── 容器方块实体
├── CauldronBlock
│   ├── 音效系统
│   └── 物品交互
├── ComposterBlock
│   ├── CompostableItems(可堆肥物品注册表)
│   ├── TickManager(tick调度)
│   └── ItemDropHelper(物品掉落)
├── CakeBlock
│   └── 可食用方块
├── BeaconBlock
│   └── 增益效果系统
├── BarrelBlock
│   └── 容器方块实体
├── LecternBlock
│   ├── 红石信号输出（updateBelow通知下方方块）
│   ├── 翻页脉冲（changePowered+tick 2tick脉冲）
│   └── 放书/移书红石更新（setHasBook→updateBelow）
├── GrindstoneBlock
│   └── 附着检测
├── BellBlock
│   ├── 多方向附着
│   └── 支撑检测
├── JukeboxBlock
│   └── 音乐播放
├── RespawnAnchorBlock
│   └── 充能系统
├── TrailsBlocks
│   ├── ChiseledBookshelfBlock(红石比较器检测)
│   ├── DecoratedPotBlock(IWaterLoggable)
│   ├── BrushableBlock(FallingBlock子类)
│   └── SnifferEggBlock(randomTick孵化)
└── 其他工作站方块
```

            ##上下游外部依赖关系

            ## #上游依赖（本模块依赖）

    | 依赖模块 | 用途 | | -- -- -- -- --| -- -- --| | `Block.hpp` | 方块基类 | | `BlockState.hpp` | 状态管理 |
    | `Material.hpp` | 材质定义 | | `CollisionShape.hpp` | 碰撞形状 | | `Properties.hpp` | 方块属性（HORIZONTAL_FACING,
    LEVEL_0_8等） | | `IWorld` | 世界接口 | | `BlockItemUseContext` | 放置上下文 | | `Player` | 玩家实体 |
    | `TickManager` | tick调度（ComposterBlock） | | `ItemDropHelper` | 物品掉落工具 |
    | `IWaterLoggable` | 含水接口（DecoratedPotBlock） | | `FallingBlock` | 下落方块基类（BrushableBlock） |

    ## #下游依赖（依赖本模块）

    | 模块 | 用途 | | -- -- --| -- -- --| | `VanillaBlocks.hpp` | 注册原版方块 | | `BlockRegistry` | 方块注册表 |
    | 世界生成 | 使用方块实例 | | 渲染系统 | 方块渲染 |

    ##容易踩的坑

        ## #1. BedBlock 双方块结构

        床头和床脚是两个独立的方块，放置时需要正确处理 `BED_PART` 属性。睡眠前需要检查：
        - 床是否被占用（`OCCUPIED` 属性） - 玩家距离床是否超过3格（水平） / 2格（垂直） - 床上方空间是否被阻挡
        - 周围是否有怪物（非创造模式） -
        在下界 /
            末地使用会爆炸（爆炸强度5.0）

            ## #2. ComposterBlock 堆肥延迟与碰撞形状

            等级7→8的转换需要20 tick延迟，通过 `TickManager` 调度。不要在 `onBlockActivated` 中直接产出骨粉。

            碰撞形状注意事项：
            - `getShape()` 根据等级返回不同的外壁形状（底板 + 四面墙壁），底板高度随等级增加
            - `getCollisionShape()` 始终返回等级0的外壁形状（SHAPES[0]）
            - 碰撞形状不是完整方块，等级0时底板高2像素 + 四面墙壁
            - 形状使用 `CollisionShape::combine(OR)` 手动拼接，因为 `CombineOp::NOT` 尚未实现

            ## #3. GrindstoneBlock 附着面

            支持3种附着面（FLOOR,
    CEILING,
    WALL），共12种VoxelShape（3附着面×4朝向）。放置时需要检测支撑方块是否存在，支撑失效时掉落。

        ## #4. RespawnAnchorBlock 维度检测

        只能在下界设置重生点。非下界使用会爆炸。使用 `Dimension::respawnAnchorWorks()` 判断当前维度是否允许重生锚。

        ## #5. TrailsBlocks 文件包含多个类

`TrailsBlocks.hpp
        / cpp` 包含4个方块类：
    - `ChiseledBookshelfBlock` - 雕纹书架 - `DecoratedPotBlock` - 饰纹陶罐（实现 IWaterLoggable）
    - `BrushableBlock` - 可刷方块（继承 FallingBlock） - `SnifferEggBlock` -
    嗅探兽蛋（randomTick 孵化）

    ## #6. LecternBlock 红石脉冲机制

    讲台的红石信号通过以下方法链实现：
    - `changePowered(world, pos, state, powered)` — 设置 POWERED 状态 + 通知下方方块
    - `pulse(world, pos, state)` — 调用 `changePowered(true)` + 调度2tick后tick重置
    - `updateBelow(world, pos, block)` — 通知 `pos.down()` 位置的所有相邻方块更新红石
    - `tick()` — 脉冲到期时调用 `changePowered(false)` 重置状态

    关键调用链：翻页 → `LecternEntity::_signalPageChange()` → `LecternBlock::pulse()` → `changePowered(true)` → 2tick后 `tick()` → `changePowered(false)`
    放书/移书 → `setHasBook()` → `updateBelow()`；方块移除（POWERED时） → `onBlockRemoved()` → `updateBelow()`
    红石更新通知 `pos.down()` 而非 `pos` 本身，因为讲台向上输出强信号（`getStrongPower` 仅 `Direction::Up` 返回15），下方方块是信号的主要接收者。

    ## #7. 方块实体注册

    部分方块需要方块实体支持：
    - `BrewingStandBlock` → `BrewingStandEntity` - `BarrelBlock` → `BarrelEntity` - `JukeboxBlock` → `JukeboxEntity` - `LecternBlock` → `LecternEntity` - `BeaconBlock` → `BeaconEntity`

    ## #8. 红石比较器信号

    多个方块支持比较器信号输出，需要正确实现 `hasComparatorInputOverride()` 和 `getComparatorInputOverride()`： - `ComposterBlock`
    : 输出 = 填充等级 - `BrewingStandBlock`: 输出基于槽位状态 - `BeaconBlock`: 输出基于效果激活状态 - `LecternBlock`
    : 输出基于书籍页面
      - `ChiseledBookshelfBlock`
    : 输出基于书籍数量

      ## #9. CauldronBlock 水位限制

      水位范围是0
      -
      3（`LEVEL_0_3`），不是0 -
      15。雨天自动填充需通过 `randomTick` 实现。

          ## #10. AnvilBlock 碰撞箱 X
          /
          Z 不对称

          铁砧碰撞箱沿朝向轴与垂直轴尺寸不同（MC原版 `Block.column` 使用直径参数，4参数版本指定 X
          / Z 独立直径）。 North / South 朝向（Z轴）和 East /
          West
          朝向（X轴）返回不同形状，通过 `_getAxisIndex()` 按 `HORIZONTAL_FACING` 的轴索引 `m_shapesByAxis`。 底座12×12对称，但中段
          / 窄颈 / 顶面沿朝向方向更宽（顶面满16像素宽）。

          ## #11. 方块交互统计（awardCustomStat）

          以下方块在 `onBlockActivated` 中调用 `player.awardCustomStat()` 记录交互统计，常量定义在 `common/stats/Stats.hpp`：

          | 方块 | 统计常量 | 状态 |
          |------|----------|------|
          | AnvilBlock | `INTERACT_WITH_ANVIL` | ✅ 已集成 |
          | BarrelBlock | `OPEN_BARREL` | ✅ 已集成 |
          | BlastFurnaceBlock | `INTERACT_WITH_BLAST_FURNACE` | ✅ 已集成 |
          | BrewingStandBlock | `INTERACT_WITH_BREWINGSTAND` | ✅ 已集成 |
          | CampfireBlock | `INTERACT_WITH_CAMPFIRE` | ✅ 已集成 |
          | CartographyTableBlock | `INTERACT_WITH_CARTOGRAPHY_TABLE` | ✅ 已集成 |
          | ChestBlock (普通) | `OPEN_CHEST` | ✅ 已集成 |
          | ChestBlock (陷阱箱) | `TRIGGER_TRAPPED_CHEST` | ✅ 已集成 |
          | FurnaceBlock | `INTERACT_WITH_FURNACE` | ✅ 已集成 |
          | LecternBlock | `INTERACT_WITH_LECTERN` | ✅ 已集成 |
          | LoomBlock | `INTERACT_WITH_LOOM` | ✅ 已集成 |
          | ShulkerBoxBlock | `OPEN_SHULKER_BOX` | ✅ 已集成 |
          | SmokerBlock | `INTERACT_WITH_SMOKER` | ✅ 已集成 |
          | BeaconBlock | `INTERACT_WITH_BEACON` | ✅ 已集成 |
          | GrindstoneBlock | `INTERACT_WITH_GRINDSTONE` | ✅ 已集成 |
          | SmithingTableBlock | `INTERACT_WITH_SMITHING_TABLE` | ✅ 已集成 |
          | StonecutterBlock | `INTERACT_WITH_STONECUTTER` | ✅ 已集成 |
          | ComposterBlock | — | MC Java 中无交互统计 |
          | CraftingTable (CraftingTableBlock) | `INTERACT_WITH_CRAFTING_TABLE` | ✅ 已集成 |
          | EnderChest (EnderChestBlock) | `OPEN_ENDERCHEST` | ✅ 已集成（容器打开待 PlayerEnderChestInventory 实现） |

          添加新的方块统计时，请确保：
          1. 在 `onBlockActivated` 成功打开容器/交互后调用 `player.awardCustomStat(ResourceLocation(stats::XXX), 1)`
          2. 包含头文件 `#include "common/stats/Stats.hpp"`
          3. 对应的统计常量已在 `common/stats/Stats.hpp` 和 `StatRegistry` 中注册
