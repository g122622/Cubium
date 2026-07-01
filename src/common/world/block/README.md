#Block 模块

方块系统的核心模块，定义了 Minecraft 中所有方块的基础架构和实现。

    ##目录结构

``` block /
├── Block.hpp /
    cpp #方块基类，定义核心属性和行为（含 updateFromNeighbourShapes 静态方法、UPDATE_SHAPE_ORDER 常量、isExceptionForConnection 连接例外判断）
├── BlockState.hpp / cpp #方块状态类，不可变状态对象
├── BlockPos.hpp #方块位置坐标类
├── BlockRegistry.hpp / cpp #方块注册表（单例）
├── BlockSoundType.hpp / cpp #方块声音类型定义
├── BlockTags.hpp /
    cpp #方块标签系统（分组判断，含 WITHER_IMMUNE、DRAGON_IMMUNE、DRAGON_TRANSPARENT、MUSHROOM_GROW_BLOCK、OCCLUDES_VIBRATION_SIGNALS、DAMPENS_VIBRATIONS、STAIRS、SLABS、WALLS、BARS、SHULKER_BOXES、WALL_POST_OVERRIDE、COMBINATION_STEP_SOUND_BLOCKS、INSIDE_STEP_SOUND_BLOCKS、CAMPFIRES、GUARDED_BY_PIGLINS、HOGLIN_REPELLENTS、PIGLIN_REPELLENTS、DOES_NOT_BLOCK_HOPPERS
        等）
├── FireInfoRegistry.hpp /
    cpp #火焰信息注册表（燃烧 / 蔓延属性）
├── GameMasterBlock.hpp #游戏管理员方块标记接口（命令方块、结构方块等）
├── HarvestTool.hpp #挖掘工具类型定义
├── IBeaconBeamColorProvider.hpp #信标光束颜色提供者接口
├── IBlockAnimateContext.hpp #方块动画 tick 上下文接口（客户端粒子 / 音效）
├── IBucketPickupHandler.hpp #桶提取接口（支持 pickupFluid 流体拾取和 pickupItem 非流体拾取双路径）
├── IGrowable.hpp #可生长方块接口（含 BoneMealType 枚举、getParticlePos 方法）
├── ILiquidContainer.hpp #液体容器接口
├── IWaterLoggable.hpp / cpp #含水方块接口
├── Material.hpp / cpp #材质系统（物理属性）
├── PlantType.hpp #植物类型定义
├── WaterLoggableHelpers.hpp #含水方块工具函数
├── FenceGateHelpers.hpp #栅栏门连接检测工具函数
├── dispense / #发射器行为系统
│   ├── IDispenseItemBehavior.hpp / cpp #发射行为接口和基类
│   ├── DispenseItemBehaviorRegistry.hpp / cpp #发射行为注册表
│   └── README.md
├── registry / #原版方块注册
│   ├── VanillaBlocks.hpp / cpp #主入口，所有原版方块静态引用
│   ├── BaseBlocks.hpp / cpp #基础方块、矿石、矿物、原木、木板
│   ├── BuildingBlocks.hpp / cpp #建筑、功能方块、石砖、石英、海晶
│   ├── BuildingVariantBlocks.hpp / cpp #楼梯 / 台阶 / 墙 / 门 / 栅栏门 / 活板门
│   ├── ColoredBlocks.hpp / cpp #染色方块（羊毛、地毯、玻璃、混凝土）
│   ├── NaturalBlocks.hpp / cpp #自然方块（冰变种、珊瑚、海洋方块）
│   ├── NetherBlocks.hpp / cpp #下界方块、末地方块
│   ├── RedstoneBlocks.hpp / cpp #红石方块、铁轨方块
│   ├── SignBannerBlocks.hpp / cpp #告示牌、旗帜
│   ├── VegetationBlocks.hpp / cpp #植被（草、花、蘑菇、树苗）
│   └── README.md
└── blocks / #具体方块实现（详见 blocks / README.md）
    ├── AirBlock.hpp / cpp #空气方块
    ├── LiquidBlock.hpp / cpp #液体方块
    ├── RotatedPillarBlock.hpp / cpp #旋转柱状方块
    ├── SimpleBlock.hpp / cpp #简单方块基类
    ├── FallingBlock.hpp / cpp #可下落方块基类
    ├── ChestBlock.hpp / cpp #箱子方块
    ├── HopperBlock.hpp / cpp #漏斗方块
    ├── DoorBlock.hpp / cpp #门方块
    ├── FenceGateBlock.hpp / cpp #栅栏门方块
    ├── CauldronBlock.hpp / cpp #炼药锅方块（水位0-3，支持水桶/玻璃瓶/皮革盔甲清洗/旗帜清洗/盾牌清洗交互）
    ├── EnchantingTableBlock.hpp / cpp #附魔台方块
    ├── SignBlock.hpp / cpp #告示牌方块
    ├── HangingSignBlock.hpp / cpp #悬挂告示牌
    ├── ShulkerBoxBlock.hpp / cpp #潜影盒方块
    ├── LightningRodBlock.hpp / cpp #避雷针方块
    ├── DirectionalBlock.hpp / cpp #有朝向的方块基类
    ├── HorizontalBlock.hpp / cpp #水平朝向方块基类
    ├── AbstractFurnaceBlock.hpp / cpp #熔炉基类
    ├── FurnaceBlock.hpp / cpp #普通熔炉
    ├── BlastFurnaceBlock.hpp / cpp #高炉
    ├── SmokerBlock.hpp / cpp #烟熏炉
    ├── TrappedChestBlock.hpp / cpp #陷阱箱
    ├── agricultural / #农业方块（农作物、农田）
    │   ├── CropBlock.hpp / cpp #作物基类
    │   ├── WheatBlock.hpp / cpp #小麦
    │   ├── CarrotBlock.hpp / cpp #胡萝卜
    │   ├── PotatoBlock.hpp / cpp #土豆
    │   ├── BeetrootBlock.hpp / cpp #甜菜根
    │   ├── FarmlandBlock.hpp / cpp #农田
    │   ├── StemBlock.hpp / cpp #茎（南瓜 / 西瓜茎）
    │   ├── CocoaBlock.hpp / cpp #可可豆
    │   └── MelonPumpkinBlocks.hpp / cpp #南瓜 / 西瓜
    ├── building / #建筑方块（详见 building / README.md）
    │   ├── StairsBlock.hpp / cpp #楼梯
    │   ├── SlabBlock.hpp / cpp #台阶
    │   ├── WallBlock.hpp / cpp #墙
    │   ├── FenceBlock.hpp / cpp #栅栏
    │   └── TrapDoorBlock.hpp / cpp #活板门
    ├── cave / #洞穴方块（紫水晶等）
    │   └── AmethystBlock.hpp / cpp
    ├── copper / #铜方块
    ├── coral / #珊瑚方块（详见 coral / README.md）
    │   └── CoralBlock.hpp / cpp
    ├── decorative / #装饰方块（详见 decorative / README.md）
    │   ├── BannerBlock.hpp / cpp #旗帜
    │   ├── CampfireBlock.hpp / cpp #营火
    │   ├── CarpetBlock.hpp / cpp #地毯
    │   ├── ChainBlock.hpp / cpp #锁链
    │   ├── FlowerPotBlock.hpp / cpp #花盆
    │   ├── LanternBlock.hpp / cpp #灯笼
    │   ├── LadderBlock.hpp / cpp #梯子
    │   ├── PaneBlock.hpp / cpp #玻璃板 / 铁栏杆
    │   ├── ScaffoldingBlock.hpp / cpp #脚手架
    │   └── StainedGlassBlock.hpp / cpp #染色玻璃
    ├── dirt / #泥土类方块
    ├── end / #末地方块（详见 end / README.md）
    │   └── EndPortalBlock.hpp / cpp
    ├── functional / #功能方块（详见 functional / README.md）
    │   ├── BarrelBlock.hpp / cpp #木桶
    │   ├── BeaconBlock.hpp / cpp #信标
    │   ├── BedBlock.hpp / cpp #床
    │   ├── BellBlock.hpp / cpp #钟
    │   ├── BrewingStandBlock.hpp / cpp #酿造台
    │   ├── CakeBlock.hpp / cpp #蛋糕
    │   ├── ComposterBlock.hpp / cpp #堆肥桶
    │   ├── GrindstoneBlock.hpp / cpp #砂轮
    │   ├── JukeboxBlock.hpp / cpp #唱片机
    │   ├── LecternBlock.hpp / cpp #讲台
    │   └── RespawnAnchorBlock.hpp / cpp #重生锚
    ├── ice / #冰方块
    ├── mangrove / #红树林方块
    ├── mob / #生物相关方块
    ├── nether / #下界方块（详见 nether / README.md）
    │   ├── FireBlock.hpp / cpp #火焰
    │   ├── SoulFireBlock.hpp / cpp #灵魂火
    │   ├── MagmaBlock.hpp / cpp #岩浆块
    │   └── NetherPortalBlock.hpp / cpp #下界传送门
    ├── ocean / #海洋方块（详见 ocean / README.md）
    │   ├── BubbleColumnBlock.hpp / cpp #气泡柱
    │   ├── ConduitBlock.hpp / cpp #潮涌核心
    │   ├── KelpBlock.hpp / cpp #海带
    │   └── SeagrassBlock.hpp / cpp #海草
    ├── pale_garden / #苍白花园方块
    ├── redstone / #红石方块（详见 redstone / README.md）
    │   ├── AbstractButtonBlock.hpp / cpp #按钮基类
    │   ├── AbstractPressurePlateBlock.hpp / cpp #压力板基类
    │   ├── AbstractRailBlock.hpp / cpp #铁轨基类
    │   ├── RedstoneWireBlock.hpp / cpp #红石线
    │   ├── RedstoneTorchBlock.hpp / cpp #红石火把
    │   ├── RedstoneRepeaterBlock.hpp / cpp #红石中继器
    │   ├── RedstoneComparatorBlock.hpp / cpp #红石比较器
    │   ├── ObserverBlock.hpp / cpp #侦测器
    │   ├── PistonBlock.hpp / cpp #活塞
    │   ├── DispenserBlock.hpp / cpp #发射器
    │   └── ... 更多红石方块
    ├── sculk / #幽匿方块
    ├── special / #特殊方块（详见 special / README.md）
    │   └── SpecialBlocks.hpp / cpp #海绵、屏障、命令方块等
    ├── trial / #试炼密室方块（TrialSpawnerBlock、VaultBlock、CrafterBlock）
    └── vegetation / #植被方块（详见 vegetation / README.md）
        ├── BambooBlock.hpp / cpp #竹子
        ├── CactusBlock.hpp / cpp #仙人掌
        ├── FlowerBlock.hpp / cpp #花
        ├── LeavesBlock.hpp / cpp #树叶
        ├── SaplingBlock.hpp / cpp #树苗
        ├── VineBlock.hpp /
    cpp #藤蔓
        └── ... 更多植被
```

    ##内部模块关系

``` Block（基类）
├── SimpleBlock（无状态静态方块）
├── AirBlock（空气方块）
├── FallingBlock（可下落方块）
├── RotatedPillarBlock（旋转柱状方块）
├── LiquidBlock（液体方块，关联 Fluid 系统）
├── HorizontalBlock /
    DirectionalBlock（朝向方块基类）
├── ChestBlock,
    HopperBlock,
    DoorBlock 等功能方块
└── 各子目录中的具体方块实现

        BlockState（状态对象）
└── StateHolder<Block, BlockState>（支持 O(1) 状态转换）

    BlockRegistry（单例注册表）
├── 管理所有 Block 实例
└── 提供 ID
    /
    资源位置查找

    Material（材质定义）
└── 描述方块物理属性（固体、透明、可燃等）

    IWaterLoggable（含水接口）
├── 继承 ILiquidContainer 和 IBucketPickupHandler
└── 被 StairsBlock,
    SlabBlock,
    WallBlock 等 19 +
        种方块实现

            GameMasterBlock（管理员方块标记接口）
├── CommandBlock、StructureBlock、JigsawBlock 实现此接口并重写 Block::isGameMaster() 返回 true
├── Block::isGameMaster() 虚方法默认返回 false，用于替代 dynamic_cast 做权限检查
└── BlockInteractionManager::_canBreakBlock() 和 GameMasterBlockItem::getStateForPlacement() 检查此权限

            IBlockAnimateContext（方块动画 tick 上下文）
├── 轻量级接口，为 Block::animateTick 提供客户端操作能力
├── addAnimateParticle() → 生成粒子效果
├── playLocalSound() → 播放本地音效
├── getBlockState() → 查询方块状态
└── ClientWorld 实现此接口，在 animateTick 调度时传入自身

            dispense
            / 子模块
├── IDispenseItemBehavior（发射行为接口）
├── DispenseItemBehaviorRegistry（行为注册表）
└── 各种具体发射行为（投射物、船、桶、打火石、骨粉等）
        - BucketDispenseBehavior：装满流体的桶放置流体，成功后替换为空桶
        - EmptyBucketDispenseBehavior：空桶收集流体，成功后替换为满桶
        - FlintAndSteelDispenseBehavior：打火石点燃方块 / 引燃TNT，消耗耐久 -
        BonemealDispenseBehavior：骨粉催熟方块 /
            水中海草，消耗数量
```

            ##上下游依赖关系

            ## #上游依赖（本模块依赖）

    | 模块 | 用途 | | -- -- --| -- -- --| | `core / Types.hpp` | 基础类型定义 | | `core / Constants.hpp` | 游戏常量 |
    | `util / Direction.hpp` | 方向枚举 | | `util / property /` | 状态属性系统 | | `physics / collision /` | 碰撞形状 |
    | `world / fluid /` | 流体系统（LiquidBlock 关联） | | `entity / loot /` | 掉落表系统 |
    | `util / math / random /` | 随机数接口 |

    ## #下游依赖（谁依赖本模块）

    | 模块 | 用途 | | -- -- --| -- -- --| | `VanillaBlocks.hpp` | 原版方块静态引用 |
    | `world / chunk /` | 区块存储方块状态 | | `world / gen /` | 世界生成放置方块 | | `renderer /` | 方块渲染 |
    | `entity /` | 实体与方块交互 | | `item /` | 物品与方块对应 | | `network /` | 方块状态同步 |

    ##容易踩的坑

    ## #1. 状态不可变性

`BlockState::with()` 返回新状态，不修改原状态：
```cpp
    // 错误：没有使用返回值
    state.with(property, value); // 状态未改变！
// 正确：使用返回的新状态
const BlockState& newState = state.with(property, value);
```

    ## #2. 材质比较必须用地址比较

```cpp
    // 正确：使用预定义材质引用
    if (block.material() == Material::ROCK)
{}

// 错误：创建新实例比较永远为 false
Material myRock = MaterialBuilder().solid().opaque().build();
if (block.material() == myRock) {} // 始终 false！
```

    ## #3. 空气方块 ID 固定为 0

`minecraft : air` 始终获得 ID 0，其他方块从 ID 1 开始。协议编码和存储时需注意。

              ## #4. BlockState 缓存不可变

`BlockState` 构造时缓存属性值，方块属性在构造后不可修改。

              ## #5. 状态ID vs 方块ID

              -
              方块ID：标识方块类型 -
              状态ID：标识方块的特定状态（包含属性值）

```cpp u32 blockId = block.blockId(); // 方块ID
u32 stateId = state.stateId();         // 状态ID
```

    ## #6. 光照透明度 vs 天空光传播

`opacity` 和 `propagatesSkylightDown` 是两个独立属性： -
    玻璃：`opacity = 0` 但 `propagatesSkylightDown = false` - 树叶：`opacity = 0` 且 `propagatesSkylightDown = true`

    ## #7. LiquidBlock 等级映射

        方块 level 与流体 level 映射： -
    方块 level = 0 → 流体 level = 8（源头） - 方块 level = 1 - 7 → 流体 level = 1 - 7 - 方块 level =
                                                                                    8 - 15 → 流体 level = 8,
          falling = true

    ## #8. 挖掘工具类型同步

`HarvestTool` 命名空间中的常量必须与 `item::tool::ToolType` 枚举值保持同步。

    ## #9. 树苗和树木生成支撑方块一致性

`SaplingBlock` 和 `TreeFeature` 必须就根支撑方块达成一致，否则会出现 "可以放置但不能生长"的不匹配。

## #10. getBlock() vs getBlockMutable()

`BlockState::getBlock()` 返回 `const Block&`，适用于只读访问。当需要调用非 const 方法（如 `tick`、`neighborChanged`、`onBlockRemoved`、`scheduleBlockTick` 等）时，使用 `getBlockMutable()` 获取 `Block&`：

```cpp
// 只读访问
const Block& block = state->getBlock();

// 需要非const引用时（如调用scheduleBlockTick、onBlockRemoved等）
Block& block = state->getBlockMutable();
```

    ## #10. 冰块融化与破坏路径分离

`IceBlock::randomTick()` 只负责融化，`onBlockRemoved()` 只负责破坏后的替换。不要让随机刻回调 `onBlockRemoved()`。

    ## #11. 作物骨粉增长随机数

        骨粉增长必须从世界种子和方块位置派生随机数，不能使用全局 `rand()`。

    ## #12. 农田降雨补湿条件

`FarmlandBlock` 的降雨补湿要同时检查 `isRaining()` 和 `canRainAt(pos.up())`，否则测试会出现伪阳性。

    ## #13. 天气降水判定

`WeatherUtils::
        canRainAt()` / `canSnowAt()` 需要结合生物群系的 `hasPrecipitation()` 布尔值以及温度阈值一起判断。沙漠、蘑菇岛、恶地等无降水生物群系必须在注册数据里显式设置 `hasPrecipitation` 为 `false`（通过 `setHasPrecipitation(
            false)`）。

    ## #14. PaneBlock 连接形状

`PaneBlock` 连接形状按 4 位掩码缓存并使用规范化坐标，不要回退到单个中心形状占位符。

    ## #15. 重复注册静默返回已存在方块

        重复注册同一资源位置的方块会返回已存在的方块，新属性被忽略。

    ## #16. AirBlock 碰撞特殊性

        AirBlock 的 `isSolid()` 返回 `false`，碰撞检测时需同时检查 `isAir()`：
```cpp if (!state.isAir() && state.isSolid())
{
    // 执行碰撞检测
}
```

        ## #17. RotatedPillarBlock 默认轴向

            默认轴向是 `Axis::X`（枚举第一个值），但大多数原木默认应该是 `Axis::Y`。注册时需要设置默认状态。

        ## #18. 门方块双方块结构

            DoorBlock 使用 `HALF` 属性区分上下半部分，操作时需同时处理两个方块位置。

        ## #19. 炼药锅无方块实体

            CauldronBlock 使用 `LEVEL_0_3` 属性存储水位（0 -
        3），交互操作直接修改方块状态，不需要方块实体。

        ## #20. 含水方块实现步骤

            实现 `IWaterLoggable` 接口需要： 1. 添加 `WATERLOGGED` 属性到状态容器 2. 在 `getStateForPlacement()` 中检测水
        3. 在 `updatePostPlacement()` 中调度流体 tick 4. 实现 `getFluidState()` 返回水流体状态

        ## #21. FireInfoRegistry 火焰参数系统

`Block::getFlammability()` 和 `Block::
            getFireSpreadSpeed()` 的默认实现已改为查询 `FireInfoRegistry`，无需子类重写即可获得正确的燃烧参数。

        - `FireInfoRegistry::initializeVanillaFireInfos()` 在 `VanillaBlocks::initialize()` 末尾自动调用
        - 仅注册 MC 原版 `FireBlock.bootStrap()` 中注册的可燃方块，未注册的方块火焰不会蔓延到其上
        - 新增可燃方块时，在 `FireInfoRegistry::initializeVanillaFireInfos()` 中注册即可
        - 子类仍可通过重写 `getFlammability()`/`getFireSpreadSpeed()` 提供自定义值，会覆盖注册表值 -
        部分方块（如 SHELF）尚待对应方块指针注册后补充

                ** 火焰蔓延 vs 岩浆点燃**：火焰蔓延（FireBlock）仅依赖本注册表，不检查
            Material。岩浆点燃（LavaFluid）通过 `Material::
                isFlammable()` 判断，是独立系统。因此，告示牌、树苗等虽然未在本注册表中注册（与原版一致），但由于使用
            WOOD
            /
            PLANT 材质，仍可被岩浆点燃。

            ## #22. canBeReplaced
            /
            canBeReplacedByFluid 语义

`BlockState` 提供两个可替换性查询方法：

        -
        **`canBeReplaced()`**：对应 MC 的 `BlockState.canBeReplaced()` 无参版，缓存自 `Block
              .m_isReplaceable`。空气、水、岩浆、花草、火、雪层等返回 `true`；石头、泥土等实心方块返回 `false`。等价于 `isAir() ||
    getMaterial().isReplaceable()` 但性能更优（缓存值）。 -
        **`canBeReplacedByFluid()`**：对应 MC 的 `BlockBehaviour.canBeReplaced(
            BlockState, Fluid)`，实现为 `canBeReplaced() ||
    !isSolid()`。非固体但不可替换的方块（门、告示牌等）也返回 `true`，允许流体流入。

        | 场景 | 使用哪个方法 | | -- -- --| -- -- -- -- -- --| | 世界生成（ReplaceablePredicate）、掉落方块判断
        | `canBeReplaced()` | | 方块放置替换
        | `canBeReplaced()`（上下文感知版应使用 `Block::isReplaceable(state, context)`） | | 流体流动 / 桶放置流体
        | `canBeReplacedByFluid()` |

        **注意 * *：不要再用 `isAir() ||
    getMaterial().isReplaceable()` 手动判断可替换性，统一使用 `canBeReplaced()`。

                ## #23. GameMasterBlock 权限检查

                管理员方块（CommandBlock、StructureBlock、JigsawBlock）有三层权限防护：

                1. *
                *放置限制 *
                *：使用 `GameMasterBlockItem`（继承
                 BlockItem），重写 `getStateForPlacement()` 检查 `player.canUseGameMasterBlocks()`（创造模式
            +
            OP≥2） 2. * *破坏限制 *
                *：`BlockInteractionManager::_canBreakBlock()` 检查 `block.isGameMaster()`，无权限阻止破坏 3. *
                *交互限制 *
                *：各方块 `onBlockActivated()` 中检查 `player
                     .canUseGameMasterBlocks()`

                 判断是否为管理员方块应使用 `block
                     .isGameMaster()` 虚方法，而非 `dynamic_cast<GameMasterBlock>`（性能更好）。新增管理员方块时需：
                 1. 继承 `GameMasterBlock` 标记接口
                 2. 重写 `isGameMaster()` 返回 `true` 3. 在 `BlockItemRegistry` 中使用 `GameMasterBlockItem` 注册
                 4. 在 `onBlockActivated()` 中添加权限检查

                 ## #24. animateTick 系统与 IBlockAnimateContext

`Block::animateTick()` 是客户端方块动画 tick
                 方法，每 tick 由 `ClientWorld::animateTick()` 调度，用于生成粒子效果、播放环境音效等视觉效果。

            - **仅客户端执行 * *：animateTick 在服务端永远不会被调用 -
            **IBlockAnimateContext 接口 * *：为避免 Block 直接依赖完整的 IWorld，animateTick 接收 `IBlockAnimateContext
        &` 而非 `IWorld &`，仅提供 `addAnimateParticle()`、`playLocalSound()`、`getBlockState()` 三个轻量方法 -
            **签名 *
                *：`virtual void animateTick(IBlockAnimateContext & context,
                    const BlockPos& pos,
                    const BlockState& state,
                    math::IRandom& random) const;
` - **基类默认实现 **：空操作（无粒子、无音效） - **调度逻辑 **：ClientWorld 每帧执行 667 次迭代 × 2 范围 pass（16 +
    32），共 1334 次随机采样 - **已实现的方块 **：BubbleColumnBlock（气泡 / 漩涡粒子 +
    环境音）、SporeBlossomBlock（孢子花粒子）

        ####canBeReplacedByFluid 与 FlowingFluid::isBlocked 的关系

`canBeReplacedByFluid()` 仅描述方块的结构属性（是否允许流体占据），不包含 MC Java
        中的业务黑名单。`FlowingFluid::isBlocked()` 在 `canBeReplacedByFluid()` 之上叠加了额外的黑名单逻辑：

        1. *
        *ILiquidContainer 方块 **（含水方块等）→ 委托给 `canContainFluid()` 判断 2. *
        *路径黑名单 *
            *：`_door`、`_sign`、`ladder`、`sugar_cane`、`bubble_column` → 返回 `true`（阻挡流体），尽管这些方块的 `canBeReplacedByFluid()` 返回 `true` 3. *
        *材质黑名单 **：`Material::PORTAL`、`Material::STRUCTURE_VOID` → 返回 `true`（阻挡流体） 4. *
        *默认 **：`!canBeReplacedByFluid()` → 如果方块不可被流体替换，则阻挡

        这是 MC Java 中 `canHoldAnyFluid()` 的精确对应：黑名单方块虽然 `canBeReplaced(
            Fluid)` 返回 `true`，但 MC Java 明确排除了它们。

        ####液体方块的 canBeReplacedByFluid

        液体方块（水、岩浆）的 `canBeReplacedByFluid()` 返回 `true`（因为 `canBeReplaced() =
    true`）。这与 MC Java 一致：`BucketItem.emptyContents()` 允许在已有液体上放置桶装流体。旧代码中 `canBeReplaced() &&
    !isLiquid()` 的 `!isLiquid()` 检查是错误的——MC Java
                    不检查目标方块是否已是液体。放置在已有液体上的行为是 `setBlock` 替换同类型液体（无操作）或水
                    /
                    岩浆交互。

                    ## #25. MUSHROOM_GROW_BLOCK 标签与蘑菇放置

`BlockTags::MUSHROOM_GROW_BLOCK()` 标签包含菌丝（mycelium）、灰化土（podzol）、绯红菌岩（crimson_nylium）、诡异菌岩（warped_nylium），用于蘑菇放置判定。

                    蘑菇（`MushroomBlock`）的放置判定分两层： 1. *
                    *`Block::canSustainPlant()`*
                    *（`PlantType::Cave` 分支）：检查下方方块是否属于 `MUSHROOM_GROW_BLOCK` 标签，只有标签内的方块才返回
                    true 2. *
                    *`MushroomBlock::isValidPosition()`* *：在标签方块上无条件允许放置，在其他固体方块上需光照 <
                13

                    * *注意 *
                    *：不要在 `canSustainPlant` 的 `PlantType::Cave` 分支中添加光照检查——光照检查由 `MushroomBlock::
                         isValidPosition()` 独立完成。`canSustainPlant` 只负责判断土壤类型兼容性。

                     蘑菇注册使用 `blocks::MushroomBlock` 而非 `SimpleBlock`，巨型蘑菇方块使用 `blocks::
                         HugeMushroomBlock`（具有 6 方向布尔属性）。

                     ## #26. Block::pushEntitiesUp 实体推出

`Block::pushEntitiesUp(oldState,
                         newState,
                         world,
                         pos)` 是一个静态工具方法，当方块碰撞形状增大时将嵌入方块内的实体向上推出。对应 MC Java
                     的 `Block.pushEntitiesUp()`。

                    * *工作原理 *
                    *： 1. 获取新方块状态的碰撞形状，如果为空则直接返回 2. 计算新形状的世界包围盒，获取旧形状的最大Y
                    3. 如果新形状最大Y没有增大（`maxY
                <= oldMaxY`），不需要推出实体 4. 使用整体包围盒查找其中的实体
                        5. 对每个与新形状相交的实体，计算需要向上推出的距离并移动

                        * *使用场景 * *： -
                    **雪层增加 * *：`tickPrecipitation()` 中雪层层数增加时调用 `pushEntitiesUp` 推出站在雪上的实体 -
                    **耕地变泥土 * *：`FarmlandBlock::turnToDirt()` 中耕地（15 / 16格高）变为泥土（1格高）时推出实体 -
                    **其他碰撞形状增大的场景 * *：任何方块状态变化导致碰撞形状增大的情况

                        * *签名 * *：`static const BlockState
            &
            pushEntitiesUp(const BlockState& oldState, const BlockState& newState, IWorld& world, const BlockPos& pos)`

                    * *注意 * *： -
                方法返回 `newState`，方便链式调用
                - 当前实现使用简化算法（整体包围盒），未来可迁移到 VoxelShape 布尔运算实现更精确的形状差异计算 -
                必须在 `setBlockState` * *之前 *
                    *调用，先推出实体再更新方块状态

                    ## #27. Block::handlePrecipitation 降水方块处理

`Block::handlePrecipitation(IWorld&,
                        const BlockPos&,
                        BiomeClimate::
                            Precipitation)` 是方块的降水处理虚方法，默认实现为空操作。方块可以重写此方法来响应降水：

                - **CauldronBlock * *：雨天 5 % 概率增加水位、雪天 10 % 概率增加水位，水位上限为 3 -
                **LightningRodBlock * *：雷暴天气且避雷针朝上时，通过 `onLightningStrike()` 激活避雷针

                    * *调用时机 *
                    *：`ServerWorld::tickPrecipitation()` 在每个降水 tick
                     中，对表面方块调用 `biome.getPrecipitationAt()` 确定降水类型后，调用 `block.handlePrecipitation(
                         world, pos, precipitation)`。

                    * *注意 * *： -
                此方法替代了旧的 `fillWithRain()` 方法，增加了降水类型参数（Rain / Snow / None）
                - 降水类型由 `Biome::getPrecipitationAt()` 确定，综合考虑生物群系降水设置和高度调整后的温度 -
                只有 `isRaining()` 为 true 时才会调用 `handlePrecipitation`（在 `tickPrecipitation` 中判断）

                    ## #28. Block::onFallenUpon 摔落伤害系统

`Block::onFallenUpon(IWorld&,
                        const BlockPos&,
                        const BlockState&,
                        Entity&,
                        f32 fallDistance)` 是方块响应实体摔落的虚方法。对应 MC Java 的 `Block.fallOn()`。

                    * *默认实现 *
                    *：调用 `entity.causeFallDamage(fallDistance, 1.0f, DamageSources::fall())` 施加普通摔落伤害。

                    * *调用链 * *：
``` Entity::move() → updateFallDistance() → _handleLandingOnBlock() → Block::onFallenUpon()
```

                    * *重要 *
                    *：`Entity::updateFallDistance()` 不再直接调用 `handleFallDamage()`，摔落伤害完全由 `Block::
                        onFallenUpon` 负责。方块子类通过重写此方法自定义摔落行为：

        | 方块 | onFallenUpon 行为 | 摔落伤害 | | -- -- --| -- -- -- -- -- -- -- -- -- -| -- -- -- -- -| | Block（基类）
        | 调用 `causeFallDamage(dist, 1.0, fall())` | 普通摔落伤害 | | PointedDripstoneBlock（石笋尖端）
        | 调用 `causeFallDamage(dist + 2.5, 2.0, stalagmite())`，不调用父类 | 增大石笋伤害，替代普通摔落 |
        | FarmlandBlock | 先执行踩踏逻辑，再调用 `Block::onFallenUpon` | 保留普通摔落伤害 | | TurtleEggBlock
        | 先执行踩破逻辑，再调用 `Block::onFallenUpon` | 保留普通摔落伤害 |

        **与 onLanded 的区别 *
                *： - `onLanded`：实体着地时修改运动向量（蜂蜜块取消摔落距离、史莱姆块弹跳），在 `updateFallDistance` 之前调用
            - `onFallenUpon`：实体着地后施加摔落伤害，由 `updateFallDistance` 内部调用

                * *乘客摔落伤害传播 *
                *：
`Entity::causeFallDamage` 会先将摔落伤害传播给所有乘客（`propagateFallToPassengers`），因此当载具（如马、船、矿车）受到摔落伤害时，乘客也会受到相同的摔落伤害。参考
                 MC 1.21.11 `Entity
                     .propagateFallToPassengers`。

                 ## #29. Block::playerWillDestroy 玩家即将破坏方块回调

`Block::playerWillDestroy(IWorld&,
                         const BlockPos&,
                         const BlockState&,
                         Player&)` 是玩家即将破坏方块时调用的虚方法。对应 MC Java 的 `Block.playerWillDestroy()`。

                * *默认实现 * *：空操作。需要特殊行为的方块应重写此方法。

                * *与 onBlockRemoved 的区别 *
                *： - `playerWillDestroy`：在方块被移除 * *之前 * *调用，接收玩家信息，可区分创造 / 生存模式
            - `onBlockRemoved`：在方块状态变更 * *之后 * *调用，不包含玩家上下文，由 `ServerWorld::setBlockState` 触发

                * *调用时机 *
                *：`BlockInteractionManager::
                    handleBlockBreak` 和 `StopDestroyBlock` 中，在生成掉落物和设置方块为空气之前调用

                * *已实现方块 *
                *： - `PistonHeadBlock`：创造模式下破坏活塞头时，级联销毁匹配的活塞基座且不产生掉落物；生存模式不执行操作（级联销毁和掉落物由 `onBlockRemoved` 处理）

                * *创造模式掉落物抑制 *
                *：`BlockInteractionManager` 在 `playerWillDestroy` 之后检查 `player
                     .isCreative()`，创造模式下跳过 `_generateBlockDrops`，与 MC Java 行为一致

                * *注意 * *：新增方块如需在破坏时区分创造 /
                生存模式行为，应重写 `playerWillDestroy` 而非在 `onBlockRemoved` 中判断

                ## #30. Block::updateFromNeighbourShapes 邻居形状更新

`Block::updateFromNeighbourShapes(
                    state, world, pos)` 是静态方法，按照 `UPDATE_SHAPE_ORDER`（WEST→EAST→NORTH→SOUTH→DOWN→UP
                轴对顺序）遍历6个方向，对每个方向调用 `updatePostPlacement` 累积更新方块状态。对应 MC Java 的 `Block
                    .updateFromNeighbourShapes()`。

                        **调用场景 *
                            *： - `ServerChunkManager::_postProcessChunk`：区块后处理生成，非液体方块形状更新（flags =
        276） - `PistonBlockEntity::clearPistonBlockEntity`：活塞完成移动后更新被移动方块形状
    - `EndermanPlaceBlockGoal::tick`：末影人放置方块后更新形状
    - `Template::placeInWorld`：结构模板放置后批量更新所有方块形状（flags = 276）

            **flags = 276 含义 * *：`SKIP_BLOCK_ENTITY_SIDEEFFECTS | KNOWN_SHAPE | INVISIBLE`（256 | 16 |
    4），区块后处理和结构放置不需要通知邻居和客户端。

        * *方向迭代顺序 * *：轴对排列（WEST→EAST,
                                                                          NORTH→SOUTH,
                                                                          DOWN→UP），同轴方向连续处理确保方块形状在轴向上一致收敛。这个顺序与 `ServerWorld::
                                                                              setBlockState` 中的邻居通知顺序不同，后者使用 `NEIGHBOR_DELTAS`（WEST→EAST→DOWN→UP→NORTH→SOUTH）。

                                                                                  **注意 *
                                                                                      *：`getBlockState` 返回 `nullptr` 时（区块未加载）安全跳过该方向；`updatePostPlacement` 是非
                                                                          const 虚方法，内部使用 `const_cast` 调用。

                                                                          ## #31. Block::dropResources 非玩家掉落生成

`Block::dropResources(IWorld&, const BlockPos&, const BlockState&)` 是静态方法，用于方块被非玩家方式破坏时（如海绵吸水、爆炸等）在世界中生成掉落物品。对应
                                                                          MC Java 的 `Block.dropResources(BlockState,
                                                                              LevelAccessor,
                                                                              BlockPos,
                                                                              BlockEntity)`。

                                                                              **与
                                                                          BlockDropHandler 的区别 *
                                                                              *： - `BlockDropHandler`：服务端专用，处理玩家破坏方块的掉落，携带工具和玩家上下文，支持时运
        / 精准采集加成
    - `Block::dropResources`：通用侧（common），不携带工具和玩家上下文，仅使用方块的掉落表生成物品

        **调用场景 **： - `SpongeBlock::absorb`：海绵吸收海带、海草等海洋植物时生成掉落物
    -
    爆炸等其他非玩家破坏场景

        **实现细节 **： 1. 检查 `world
            .lootTableManager()` 是否为空（客户端返回 nullptr，直接退出） 2. 获取方块的掉落表，如果为空则退出
    3. 构建 `LootContext`（使用世界共享随机 `world.getRandom()`，与 MC 原版 `ServerLevel.random` 一致）
    4. 设置必需参数 `BLOCK_STATE` 和 `BLOCK_POS`（通过 `const_cast` 转为非 const
    指针，与 `BlockDropHandler::buildLootContext` 一致） 5. 设置掉落表解析器和条件解析器
    6. 调用 `LootTable::generate()` 生成物品列表 7. 通过 `ItemDropHelper::spawnItemEntities()` 在世界中生成掉落物实体
    8. 调用 `Block::spawnAfterBreak()` 触发额外效果

        **客户端安全 **：`lootTableManager()` 在客户端返回 `nullptr`，方法直接返回，不生成任何掉落物

                    ## #32. Block::isExceptionForConnection 连接例外方块判断

`Block::isExceptionForConnection(const BlockState& state)` 是静态方法，判断方块是否属于"连接例外"——即虽然是固体方块但不应与栅栏、墙、玻璃板建立连接。对应 MC Java 的 `Block.isExceptionForConnection()`。

**连接例外方块列表**：
- `BlockTags::LEAVES()` — 所有树叶（橡树、云杉、白桦等）
- `BlockTags::SHULKER_BOXES()` — 所有潜影盒变体（16色 + 无色）
- `barrier` — 屏障方块
- `carved_pumpkin` — 雕刻南瓜
- `jack_o_lantern` — 南瓜灯
- `melon` — 西瓜
- `pumpkin` — 南瓜

**使用场景**：
- `FenceBlock::_canConnect()` — 固体方块连接时排除例外：`!Block::isExceptionForConnection(state) && isNeighborSolid`
- `WallBlock::_getWallHeight()` — 固体方块连接时排除例外：`!Block::isExceptionForConnection(state) && state.isSolid()`
- `PaneBlock::shouldConnectTo()` — 固体侧面连接时排除例外：`!Block::isExceptionForConnection(neighborState) && isSolidSide`

**注意**：不要在连接逻辑中仅检查 `isSolid()` 而忘记 `isExceptionForConnection()`，否则栅栏/墙/玻璃板会错误地连接到树叶、潜影盒等方块。

                    ## #33. BlockTags 新增标签说明

### BARS 标签
`BlockTags::BARS()` 包含铁栏杆（`iron_bars`），用于 WallBlock 的 `_getWallHeight()` 判断——铁栏杆与墙连接时返回 `WallHeight::Low`（低连接）。

### SHULKER_BOXES 标签
`BlockTags::SHULKER_BOXES()` 包含所有潜影盒变体（无色 + 16色），用于 `Block::isExceptionForConnection()` 判断——潜影盒虽然是固体，但不应与栅栏、墙、玻璃板建立连接。

### WALL_POST_OVERRIDE 标签
`BlockTags::WALL_POST_OVERRIDE()` 包含放置在墙上时强制显示墙柱的方块：火把、灵魂火把、红石火把、绊线、告示牌（站立/墙面所有变体）、旗帜（站立/墙面所有颜色）、压力板（木质/石质/铜质/金质/铁质所有变体）。用于 WallBlock 的 `_shouldRaisePost()` 判断——当直线 Tall 墙上方有 WALL_POST_OVERRIDE 标签方块时，强制升起墙柱（UP=true）。

### DOES_NOT_BLOCK_HOPPERS 标签
`BlockTags::DOES_NOT_BLOCK_HOPPERS()` 包含蜂巢(bee_nest)和蜂箱(beehive)，即与 BEEHIVES 标签相同的方块。用于 `HopperEntity::pullItems()` 中的漏斗吸取判断——即使上方方块碰撞形状为完整方块（`isFaceFull(Direction::Down)` 为 true），若该方块在此标签中，漏斗仍可吸取上方物品实体。这允许漏斗与蜂巢/蜂箱交互（吸取蜂蜜瓶/空瓶）。

                    ## #34. getEntityInsideCollisionShape 实体内部碰撞形状

`Block::getEntityInsideCollisionShape(const BlockState&)` 是虚方法，返回方块用于实体内部碰撞检测的形状。对应 MC 原版 `BlockBehaviour.getEntityInsideCollisionShape()`。

**默认行为**：基类返回 `VoxelShapes::fullCube()`（完整方块），与 MC 原版 `Shapes.block()` 一致。大多数方块不需要重写此方法。

**重写方块**：
| 方块 | 返回形状 | 说明 |
|------|----------|------|
| CauldronBlock | 水位0: `fullCube()`; 水位1-3: `m_filledShapes[level-1]` | 空炼药锅返回完整方块（继承默认），有水时返回外部形状∪内容区域 |
| LavaCauldronBlock | `m_filledShape` | 外部形状∪岩浆内容区域，始终满 |

**与 getCollisionShape 的区别**：
- `getCollisionShape()`：物理碰撞形状（实体推动、站立），炼药锅返回仅外部壁的形状
- `getEntityInsideCollisionShape()`：实体内部检测形状，决定实体何时被视为"在方块内部"以触发 `onEntityCollision` 回调

**调用位置**：`Entity::doBlockCollisions()` 使用此形状替代简单 AABB-vs-网格位置检测，实现形状感知的实体碰撞检测。只有实体 AABB 与方块的 `getEntityInsideCollisionShape` 返回形状相交时，才会调用 `onEntityCollision`。

**重要**：空炼药锅（水位0）返回 `fullCube()` 是 MC 原版行为——`AbstractCauldronBlock` 不重写 `getEntityInsideCollisionShape`，因此继承 `Shapes.block()`。这意味着实体只要在空炼药锅的方块格子内就会触发 `onEntityCollision`，但 `CauldronBlock::onEntityCollision` 内部检查水位为0时直接返回，所以空炼药锅不会灭火。

## #35. IBucketPickupHandler 双路径拾取（pickupFluid / pickupItem）

`IBucketPickupHandler` 接口支持两种拾取路径：

1. **流体拾取（pickupFluid）**：流体方块（水、岩浆）重写此方法返回对应的 `Fluid*`，空桶拾取后获得满桶物品（水桶/岩浆桶）。`BucketItem::onItemUse` 的空桶路径优先尝试此方法。

2. **非流体拾取（pickupItem）**：非流体方块（如细雪 PowderSnowBlock）重写此方法返回对应的 `const Item*`（如 `Items::POWDER_SNOW_BUCKET`），空桶拾取后获得细雪桶。当 `pickupFluid()` 返回 nullptr 时，`BucketItem::onItemUse` 会继续尝试 `pickupItem()`。默认实现返回 nullptr。

3. **拾取音效（getPickupSound）**：非流体方块可重写此方法返回拾取时播放的音效（如细雪返回 `SoundEvents::ITEM_BUCKET_FILL_POWDER_SNOW`）。流体方块使用 `BucketItem` 中硬编码的 `ITEM_BUCKET_FILL` 音效，无需重写此方法。默认实现返回 nullptr。

**实现规则**：
- 流体方块只需实现 `pickupFluid()`，`pickupItem()` 和 `getPickupSound()` 保持默认即可
- 非流体方块只需实现 `pickupItem()` 和 `getPickupSound()`，`pickupFluid()` 返回 nullptr
- `BucketItem::onItemUse` 的空桶路径先尝试 `pickupFluid()`，失败后再尝试 `pickupItem()`
- `EmptyBucketDispenseBehavior` 同样遵循此双路径逻辑
