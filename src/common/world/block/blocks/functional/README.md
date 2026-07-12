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
├── ComposterBlock.hpp / cpp #堆肥桶（8层填充，产出骨粉，碰撞形状为外壁拼接：底板+四面墙壁，ISidedInventoryProvider支持漏斗自动化交互）
├── CakeBlock.hpp / cpp #蛋糕（可食用，7片）
├── BeaconBlock.hpp / cpp #信标（增益效果，金字塔基座）
├── BarrelBlock.hpp / cpp #木桶（存储容器，6方向放置）
├── LecternBlock.hpp / cpp #讲台（书籍展示，红石脉冲+比较器信号，翻页2tick脉冲）
├── GrindstoneBlock.hpp / cpp #砂轮（修复 / 祛魔，3种附着面）
├── StonecutterBlock.hpp / cpp #切石机（石材切割配方）
├── LoomBlock.hpp / cpp #织布机（旗帜图案制作）
├── BellBlock.hpp / cpp #钟（声音/动画，4种附着方式，摇晃动画+共振机制+灾厄村民发光，方块实体驱动）
├── JukeboxBlock.hpp / cpp #唱片机（音乐播放）
├── RespawnAnchorBlock.hpp / cpp #重生锚（下界重生点，4级充能）
├── LodestoneBlock.hpp / cpp #磁石（指南针绑定）
├── CartographyTableBlock.hpp / cpp #制图台（地图复制 / 扩展）
├── CraftingTableBlock.hpp / cpp #工作台（3x3合成界面，INTERACT_WITH_CRAFTING_TABLE统计）
├── FletchingTableBlock.hpp / cpp #制箭台（箭矢制作）
├── SmithingTableBlock.hpp / cpp #锻造台（装备升级）
├── TrailsBlocks.hpp / cpp #考古方块（雕纹书架、饰纹陶罐、可疑沙 /
            砾、嗅探兽蛋）
├── CandleCakeBlock.hpp / cpp #蜡烛蛋糕（蜡烛插在蛋糕上，点燃/熄灭，食用后掉落蜡烛）
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
│   ├── ISidedInventoryProvider(漏斗自动化交互)
│   │   ├── InputContainer(等级0-6：仅允许从上方输入可堆肥物品)
│   │   ├── EmptyContainer(等级7：不允许任何交互，等待20tick转变)
│   │   └── OutputContainer(等级8：仅允许从下方提取骨粉)
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
│   ├── 4种附着方式（Floor/Ceiling/SingleWall/DoubleWall）
│   ├── 状态属性 HORIZONTAL_FACING + BELL_ATTACHMENT + POWERED
│   ├── 16种状态碰撞箱（4朝向×4附着）
│   ├── onBlockActivated/onProjectileHit/neighborChanged 敲钟入口
│   ├── attemptToRing/onHit/_isProperHit 敲击判定
│   └── 关联 BellBlockEntity（摇晃动画+共振+发光）
├── JukeboxBlock
│   └── 音乐播放
├── RespawnAnchorBlock
│   └── 充能系统
├── TrailsBlocks
│   ├── ChiseledBookshelfBlock(红石比较器检测)
│   ├── DecoratedPotBlock(IWaterLoggable, DecoratedPotBlockEntity方块实体)
│   │   ├── onBlockActivated: 手持物品放入罐中(+1，创造模式不消耗)/空手触发负摇晃动画
│   │   ├── playerWillDestroy: 手持BREAKS_DECORATED_POTS标签物品(工具/武器)时设为CRACKED状态(精准采集附魔可阻止碎裂)
│   │   ├── onProjectileHit: 投射物命中时总是设为CRACKED状态并破坏(不受精准采集保护)
│   │   ├── onBlockRemoved: CRACKED状态掉落4个陶片/砖块物品；非CRACKED状态掉落罐内存储物品(陶罐物品由战利品表处理)
│   │   ├── getCloneItemStack: 中键选取返回带sherds数据的陶罐物品
│   │   ├── getComparatorInputOverride: 红石比较器信号输出
│   │   ├── onBlockActivated: 物品插入后通过RedstoneSystem通知比较器更新
│   │   └── 摇晃动画(Positive=放入/Negative=空手)触发
│   ├── BrushableBlock(FallingBlock子类，构造接收 brushSound/brushCompletedSound 音效绑定)
│   └── SnifferEggBlock(onBlockAdded+scheduleTick孵化，hatchBoost加速)
├── CandleCakeBlock(→ AbstractCandleBlock)
│   ├── 关联蜡烛方块（m_candleBlock，食用蛋糕后放置对应蜡烛）
│   ├── LIT 属性（仅点燃状态，无 CANDLES/BUITES）
│   ├── onBlockActivated: 空手点击上半部熄灭 / 其他情况吃蛋糕→转为 CakeBlock + 掉落蜡烛物品
│   ├── 食用蛋糕增加饥饿值（foodStats().addStats(2, 0.1f)），与 MC Java 一致
│   ├── 比较器输出固定14（hasComparatorInputOverride）
│   └── 亮度固定3（单根蜡烛）
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

        ## #1. BedBlock 双方块结构与关键方法

床头和床脚是两个独立的方块，放置时需要正确处理 `BED_PART` 属性。构造函数接收 `DyeColor` 参数，16色床在 `ColoredBlocks` 中注册。

### onBlockPlacedBy（自动放置 HEAD 半部）

玩家放置床时，`BedItem.getStateForPlacement()` 返回 FOOT 部分的状态，然后 `BedBlock::onBlockPlacedBy()` 在脚部前方自动放置 HEAD 部分方块：

```cpp
void BedBlock::onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(stack);

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockPos headPos = pos.offset(facing);
    BlockState headState = state.with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);
    world.setBlockState(headPos, &headState, 3);
}
```

`onBlockPlacedBy` 的 `stack` 参数携带放置该方块的物品堆，便于方块实体从物品继承自定义名称等组件（参考 MC Java 的 `BaseContainerBlockEntity.applyImplicitComponents`）。床方块当前不使用此参数，但保留以便未来扩展。

### playerWillDestroy（创造模式移除 HEAD）

创造模式玩家破坏 FOOT 部分时，`playerWillDestroy()` 同时移除 HEAD 部分方块（防止产生掉落物）。生存模式下 HEAD 部分的移除和掉落物由 `onBlockRemoved` 处理。

### getStateForPlacement（检查头部位置可替换性）

`BedBlock::getStateForPlacement()` 检查头部位置是否可替换（`canBeReplaced()`），如果头部位置不可替换则返回默认状态（放置将失败）。`BedItem::getStateForPlacement()` 做同样的检查，但如果头部不可替换则返回 `nullptr`（直接阻止放置）。

### DyeColor 参数

每种颜色的床是独立的 `BedBlock` 实例，构造时传入 `DyeColor` 枚举值。颜色存储在 `m_color` 成员中，可通过 `getColor()` 获取。16色床在 `ColoredBlocks` 中注册，物品在 `Items::_registerBeds()` 中使用 `BedItem` 注册。

### 其他注意事项

睡眠前需要检查：
- 床是否被占用（`OCCUPIED` 属性）
- 玩家距离床是否超过3格（水平）/2格（垂直）
- 床上方空间是否被阻挡
- 周围是否有怪物（非创造模式）
- 在下界/末地使用会爆炸（爆炸强度5.0）

            ## #2. ComposterBlock 堆肥延迟、碰撞形状与漏斗交互

            等级7→8的转换需要20 tick延迟，通过 `TickManager` 调度。不要在 `onBlockActivated` 中直接产出骨粉。

            碰撞形状注意事项：
            - `getShape()` 根据等级返回不同的外壁形状（底板 + 四面墙壁），底板高度随等级增加
            - `getCollisionShape()` 始终返回等级0的外壁形状（SHAPES[0]）
            - 碰撞形状不是完整方块，等级0时底板高2像素 + 四面墙壁
            - 形状使用 `CollisionShape::combine(OR)` 手动拼接，因为 `CombineOp::NOT` 尚未实现

            漏斗自动化交互（ISidedInventoryProvider）：
            - ComposterBlock 实现了 `ISidedInventoryProvider` 接口，允许漏斗自动与堆肥桶交互
            - `createInventory()` 根据等级返回不同的内部容器：
              - 等级 0-6: `InputContainer`（1槽位，仅允许从 Direction::Up 输入可堆肥物品）
              - 等级 7: `EmptyContainer`（0槽位，不允许任何交互，等待20tick转变）
              - 等级 8: `OutputContainer`（1槽位骨粉，仅允许从 Direction::Down 提取）
            - `InputContainer::setChanged()` 在物品放入后自动调用 `ComposterBlock::attemptCompost()` 处理堆肥
            - `OutputContainer::setChanged()` 在骨粉被提取后自动调用 `ComposterBlock::empty()` 重置堆肥桶
            - 漏斗通过 `HopperEntity::getInventoryAtPosition()` 检测 ISidedInventoryProvider，创建拥有型 InventoryRef
            - 与 BlockEntity 路径不同，ISidedInventoryProvider 返回的容器是临时的，每次交互都重新创建

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
    - `BrushableBlock` - 可刷方块（继承 FallingBlock，构造接收 `brushSound`/`brushCompletedSound` 音效绑定，通过 `getBrushSound()`/`getBrushCompletedSound()` 暴露给 `BrushItem::onUseTick`） - `SnifferEggBlock` -
    嗅探兽蛋（onBlockAdded + scheduleTick 孵化，hatchBoost 加速）

    **BrushableBlock 音效绑定与方块实体**：可疑沙在 `registerTrailsBlocks()` 中绑定 `SoundEvents::BRUSH_SAND`/`BRUSH_SAND_COMPLETED`，可疑沙砾绑定 `SoundEvents::BRUSH_GRAVEL`/`BRUSH_GRAVEL_COMPLETED`。`BrushItem::onUseTick` 命中 BrushableBlock 时通过 `dynamic_cast` 获取音效，命中其他方块时回退到 `BRUSH_GENERIC`。状态属性 `DUSTED(0-3)` 记录刷扫进度，由 `BrushableBlockEntity` 驱动（详见 `world/blockentity/interactive/README.md` 第 #18 条）。

    **BrushableBlock 构造参数 `turnsInto`**：构造时传入刷扫完成后转换的目标方块（可疑沙→`VanillaBlocks::SAND`，可疑沙砾→`VanillaBlocks::GRAVEL`），通过 `getTurnsInto()` 暴露给 `BrushableBlockEntity::brushingCompleted()` 用于方块替换。`tick()` 重写先调用 `BrushableBlockEntity::checkReset()` 处理刷扫计数重置，再委托 `FallingBlock::tick()` 执行下落检测。

    **SnifferEggBlock 孵化逻辑**：实现 MC 1.21.11 `SnifferEggBlock` 的完整孵化流程，采用 `onBlockAdded` + `scheduleTick` 调度（非 `randomTick`）：
    - **状态属性**：`HATCH_0_2`（0/1/2 三级孵化进度），默认 0
    - **放置调度**（`onBlockAdded`）：
      1. 客户端守卫：`if (world.isClientSide()) return;`
      2. 检测下方方块是否在 `BlockTags::SNIFFER_EGG_HATCH_BOOST` 标签中（`hatchBoost(world, pos)`）
      3. 加速时广播 `WorldEvents::EGG_CRACK (3009)` 粒子事件
      4. 孵化总时长：加速 12000 tick，常规 24000 tick；分三阶段，每段 `i/3 + [0, 300)` tick
      5. 发出 `GameEvents::BLOCK_PLACE` 游戏事件（通知附近幽匿感测体）
      6. `tickManager().scheduleBlockTick(pos, *this, delay, TickPriority::Normal)` 调度首个孵化 tick
    - **tick 回调**（计划刻驱动）：
      - **hatch < 2 分支**：播放 `SNIFFER_EGG_CRACK` 音效（音量 0.7，音高 0.9 + random*0.2），`HATCH` 等级 +1，重新查询 `hatchBoost` 并调度下一阶段 tick
      - **hatch = 2 分支（孵化完成）**：
        1. 播放 `SNIFFER_EGG_HATCH` 音效
        2. 销毁蛋方块（替换为 AIR）
        3. 创建 `SnifferEntity`，调用 `setChild(true)` 设置幼年期 -48000 tick（40 分钟）
        4. `setPosition(pos.center())` + `setRotation(wrapDegrees(random*360), 0)` 对齐 MC `snapTo`
        5. `finalizeSpawn(world, difficultyInstance, SpawnReason::Natural)` 进行基于难度的初始化
        6. `world.spawnEntity(std::move(sniffer))` 生成到世界
    - **客户端守卫**：`onBlockAdded` 与 `tick` 方法入口均显式 `if (world.isClientSide()) return;`，仅服务端执行
    - **ticksRandomly**：返回 false，不依赖 `randomTick` 随机孵化
    - **孵化加速**（`hatchBoost`）：检测下方方块是否在 `BlockTags::SNIFFER_EGG_HATCH_BOOST` 标签中（当前包含 `minecraft:moss_block`）。加速时孵化总时长 24000→12000 tick，并在放置时广播 `EGG_CRACK` 粒子事件
    - **调度差异说明**：MC 原版通过 `LevelChunk.setBlockState` 在每次 `setBlock` 时调用 `onPlace`，因此 HATCH 等级变化会重新触发 `onPlace` 自动调度下一阶段 tick。本项目 `ServerWorld::setBlockState` 仅在方块类型变化时触发 `onBlockAdded`，因此 `tick` 回调中需显式重新调度下一阶段 tick 以对齐 MC 的三阶段调度时序

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

      ## #9. CauldronBlock 三层架构

      炼药锅系统分为三类方块：CauldronBlock（空，无水位属性）、LayeredCauldronBlock（水/细雪，`LEVEL_1_3` 属性 1-3）、LavaCauldronBlock（岩浆，始终满）。水位变化通过方块替换实现（空→水/岩浆，水→空）。

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

          ## #12. CandleCakeBlock 蜡烛蛋糕交互逻辑

          CandleCakeBlock 继承自 AbstractCandleBlock（装饰性模块），但放置在功能模块中，因为它具有蛋糕食用交互功能。

          关键行为：
          - **不继承 CakeBlock**：CandleCakeBlock 没有 BITES 属性，只有 LIT 属性。食用后不是逐步减少咬数，而是直接替换为 CakeBlock（7片完整蛋糕）+ 掉落蜡烛物品
          - **关联蜡烛方块**：构造时传入 `candleBlock` 参数（`m_candleBlock`），食用蛋糕后在该位置放置对应颜色的蜡烛方块
          - **交互判定**：`onBlockActivated` 通过命中位置 Y 坐标判断点击区域——上半部为蜡烛区域（空手熄灭），下半部为蛋糕区域（食用）
          - **比较器信号**：固定输出14（`hasComparatorInputOverride` 返回 true，`getComparatorInputOverride` 返回14）
          - **亮度**：点燃时固定为3（单根蜡烛），与 CandleBlock 的 `3 * CANDLES` 公式不同
          - **标签**：属于 `CANDLE_CAKES` 标签而非 `CANDLES` 标签

          与 CakeBlock 的关系：
          - CandleCakeBlock 食用后 → 替换为 CakeBlock（BITES=0，完整7片） + 掉落蜡烛物品
          - 对 CandleCakeBlock 使用蜡烛物品 → 无效（不支持堆叠，这是 CandleBlock 的功能）
          - 对 CakeBlock 使用蜡烛物品 → 替换为对应颜色的 CandleCakeBlock

          ## #13. BellBlock 附着方式、敲击判定与红石触发

          BellBlock 是方块实体驱动型方块（`hasBlockEntity` 返回 true），关联 `BellBlockEntity` 实现摇晃动画、共振机制与灾厄村民发光。详见 `world/blockentity/interactive/README.md` 第 #14-#16 条。

          **状态属性**：`HORIZONTAL_FACING`（North/South/East/West）+ `BELL_ATTACHMENT`（Floor/Ceiling/SingleWall/DoubleWall）+ `POWERED`（红石充能状态）。共 4×4×2 = 32 种状态，碰撞箱按 4 朝向 × 4 附着 = 16 种组合预计算。

          **4 种附着方式**（`getStateForPlacement`）：
          - 点击顶面 → Floor（地面附着），朝向 = 玩家水平朝向
          - 点击底面 → Ceiling（天花板附着）
          - 点击侧面 → 检测同轴两侧是否都有固体面（双面墙条件），满足则 DoubleWall，否则 SingleWall；若墙附着不可用则回退 Floor/Ceiling
          - 所有路径检查 `Block::hasEnoughSolidSide` 支撑（对应 MC Java 的 `isFaceSturdy`）

          **支撑更新**（`updatePostPlacement`）：
          - 支撑失效（非 DoubleWall）→ AIR
          - DoubleWall 一侧失效 → SingleWall，朝向翻转
          - SingleWall 对侧出现支撑 → DoubleWall
          - 方向条件需严格对齐 MC Java：SingleWall→DoubleWall 检查 `FACING.opposite == facing`（即对侧更新），不是 `FACING == facing`

          **敲击判定**（`_isProperHit`）：
          - Y 轴点击或 `hitY > 0.8124` → 无效
          - Floor 附着：点击方向轴 == 朝向轴 → 有效
          - SingleWall/DoubleWall：点击方向轴 != 朝向轴 → 有效
          - Ceiling：任意水平方向 → 有效

          **敲钟入口**：
          - 玩家右键：`onBlockActivated` → `onHit(hit, player, isProjectile=false)`
          - 投射物击中：`onProjectileHit` → 从 `ProjectileEntity::getShooter()` 提取 Player → `onHit(hit, player, isProjectile=true)`
          - 红石上升沿：`neighborChanged` 检测 `RedstonePower::isPowered` 从 false→true → `attemptToRing(world, pos, FACING)`
          - `onHit` 判定 isProperHit 后调用 `attemptToRing`，成功则 `awardCustomStat(BELL_RING, 1)`

          **attemptToRing**（仅服务端）：获取 `BellBlockEntity` → `onHit(world, direction)` → 播放 `BLOCK_BELL_USE`（音量 2.0）→ 触发 `GameEvents::BLOCK_ACTIVATE` 游戏事件（通知附近幽匿感测体）

          **统计集成**：`onHit` 成功敲响后调用 `player.awardCustomStat(ResourceLocation(stats::BELL_RING), 1)`，常量定义于 `common/stats/Stats.hpp`。
