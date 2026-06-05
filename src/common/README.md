# Common Module

公共模块 (`mc_common`) 是 Cubium 项目的核心共享库，包含客户端和服务端共用的所有代码。

## 当前重点

- `resource/` 里的 `ResourcePackList` 现在会被客户端主线程和音频线程共享。
- 资源包查询采用读写锁分离，查询结果尽量返回拷贝，避免长期持有内部元素地址。
- `AudioService` 依赖这套共享资源语义来读取 `sounds.json` 和音频文件。
- 共享资源体系的变更会通过 `onChange` 通知上层重新加载资源或声音定义。

## 目录结构

```
src/common/
├── core/                    # 核心类型和工具
│   ├── Types.hpp           # 基础类型定义 (i8, i16, i32, u8, std::string, etc.)
│   ├── Result.hpp          # 错误处理 (Result<T>, Error, ErrorCode)
│   ├── Constants.hpp       # 游戏常量
│   ├── EnumSet.hpp         # 枚举集合工具
│   ├── BlockRaycastResult.hpp # 方块射线检测结果
│   └── settings/           # 设置系统
│       ├── SettingsBase.hpp
│       ├── SettingsTypes.hpp
│       └── ResourcePackListOption.hpp
│
├── command/                 # 命令系统
│   ├── CommandDispatcher.hpp  # 命令分发器
│   ├── CommandContext.hpp     # 命令上下文
│   ├── CommandNode.hpp        # 命令节点
│   ├── CommandResult.hpp      # 命令结果
│   ├── CommandSource.hpp      # 命令源
│   ├── ICommandSource.hpp     # 命令源接口
│   ├── StringReader.hpp       # 字符串读取器
│   ├── arguments/             # 命令参数解析器
│   │   ├── ArgumentType.hpp
│   │   ├── EntityArgument.hpp
│   │   ├── GameModeArgument.hpp
│   │   └── ItemArgument.hpp
│   ├── exceptions/            # 命令异常
│   │   └── CommandExceptions.hpp
│   └── suggestions/           # Tab 补全建议
│       └── Suggestions.hpp
│
├── entity/                   # 实体系统
│   ├── Entity.hpp            # 实体基类
│   ├── EntityType.hpp        # 实体类型注册
│   ├── EntityClassification.hpp # 实体分类
│   ├── EntityDataManager.hpp # 实体数据管理
│   ├── EntityPose.hpp        # 实体姿态
│   ├── EntitySize.hpp        # 实体尺寸
│   ├── EntitySpawnPlacementRegistry.hpp # 生成位置规则
│   ├── EntityUtils.hpp       # 旧实体类型映射
│   ├── ItemEntity.hpp        # 物品实体
│   ├── Player.hpp            # 玩家实体
│   ├── PlayerManager.hpp     # 玩家管理器
│   ├── VanillaEntities.hpp   # 原版实体定义
│   ├── utils/                # 非模板实体工具
│   │   ├── ItemDropHelper.hpp/cpp # 物品掉落工具类
│   │   ├── EntityUtils.hpp/cpp    # LegacyEntityType映射
│   │   └── README.md              # 工具模块说明
│   ├── ai/                   # AI 系统
│   │   ├── controller/       # 控制器
│   │   │   ├── LookController.hpp
│   │   │   ├── MovementController.hpp
│   │   │   └── JumpController.hpp
│   │   ├── goal/             # 目标系统
│   │   │   ├── Goal.hpp
│   │   │   ├── GoalSelector.hpp
│   │   │   ├── PrioritizedGoal.hpp
│   │   │   └── goals/        # 具体目标
│   │   │       ├── SwimGoal.hpp
│   │   │       ├── RandomWalkingGoal.hpp
│   │   │       ├── LookAtGoal.hpp
│   │   │       ├── PanicGoal.hpp
│   │   │       ├── BreedGoal.hpp
│   │   │       ├── TemptGoal.hpp
│   │   │       ├── FollowParentGoal.hpp
│   │   │       ├── MeleeAttackGoal.hpp
│   │   │       └── AvoidEntityGoal.hpp
│   │   └── pathfinding/      # 寻路系统
│   │       ├── Path.hpp
│   │       ├── PathFinder.hpp
│   │       ├── PathNavigator.hpp
│   │       ├── PathPoint.hpp
│   │       ├── PathHeap.hpp
│   │       ├── PathNodeType.hpp
│   │       ├── NodeProcessor.hpp
│   │       ├── WalkNodeProcessor.hpp
│   │       └── Region.hpp
│   ├── animal/               # 动物实体
│   │   ├── AnimalEntity.hpp
│   │   ├── PigEntity.hpp
│   │   ├── CowEntity.hpp
│   │   ├── SheepEntity.hpp
│   │   └── ChickenEntity.hpp
│   ├── attribute/            # 属性系统
│   │   ├── Attribute.hpp
│   │   ├── AttributeInstance.hpp
│   │   ├── AttributeModifier.hpp
│   │   ├── AttributeMap.hpp
│   │   └── Attributes.hpp
│   ├── combat/               # 战斗系统
│   │   ├── AttackContext.hpp
│   │   └── PlayerAttackHelper.hpp
│   ├── damage/               # 伤害追踪
│   │   ├── CombatEntry.hpp
│   │   ├── CombatTracker.hpp
│   │   └── DamageSource.hpp
│   ├── inventory/            # 背包系统
│   │   ├── Container.hpp
│   │   ├── ContainerTypes.hpp
│   │   ├── CraftingInventory.hpp
│   │   ├── PlayerInventory.hpp
│   │   ├── CreativeInventory.hpp
│   │   ├── CreativeInventory.cpp
│   │   ├── IInventory.hpp
│   │   ├── Slot.hpp
│   │   └── AbstractContainerMenu.hpp
│   ├── living/               # 生物实体基类
│   │   └── LivingEntity.hpp
│   ├── loot/                 # 战利品表
│   │   ├── LootTable.hpp
│   │   ├── LootContext.hpp
│   │   ├── LootEntry.hpp
│   │   ├── LootPool.hpp
│   │   ├── LootConditions.hpp
│   │   └── LootFunctions.hpp
│   │   # (RandomRanges 已移至 util/math/random/)
│   ├── mob/                  # 生物实体
│   │   ├── MobEntity.hpp
│   │   ├── CreatureEntity.hpp
│   │   └── AgeableEntity.hpp
│   └── movement/             # 移动系统
│       └── AutoJump.hpp
│
├── item/                     # 物品系统
│   ├── Item.hpp              # 物品基类
│   ├── ItemStack.hpp         # 物品堆
│   ├── ItemRegistry.hpp      # 物品注册表
│   ├── Items.hpp             # 原版物品定义
│   ├── BlockItem.hpp         # 方块物品
│   ├── BlockItemRegistry.hpp # 方块物品注册表
│   ├── BlockItemUseContext.hpp
│   ├── ItemUseContext.hpp
│   ├── crafting/             # 合成系统
│   │   ├── IRecipe.hpp
│   │   ├── Ingredient.hpp
│   │   ├── RecipeManager.hpp
│   │   ├── RecipeLoader.hpp
│   │   ├── RecipeSerializers.hpp
│   │   ├── ShapedRecipe.hpp
│   │   └── ShapelessRecipe.hpp
│   ├── enchantment/          # 附魔系统
│   │   ├── Enchantment.hpp
│   │   ├── EnchantmentRegistry.hpp
│   │   ├── EnchantmentContainer.hpp
│   │   ├── EnchantmentHelper.hpp
│   │   └── enchantments/
│   │       ├── FortuneEnchantment.hpp
│   │       └── SilkTouchEnchantment.hpp
│   ├── tier/                 # 工具等级
│   │   ├── IItemTier.hpp
│   │   └── ItemTiers.hpp
│   └── tool/                 # 工具系统
│       ├── ToolType.hpp
│       ├── TieredItem.hpp
│       ├── ToolItem.hpp
│       ├── PickaxeItem.hpp
│       ├── AxeItem.hpp
│       ├── ShovelItem.hpp
│       ├── HoeItem.hpp
│       └── SwordItem.hpp
│
├── network/                  # 网络系统
│   ├── connection/           # 连接管理
│   │   ├── IServerConnection.hpp
│   │   ├── Connection.hpp
│   │   ├── LocalConnection.hpp      # 本地连接 (集成服务器)
│   │   └── LocalServerConnection.hpp
│   ├── packet/               # 数据包
│   │   ├── Packet.hpp
│   │   ├── PacketSerializer.hpp
│   │   ├── EntityPackets.hpp
│   │   ├── InventoryPackets.hpp # 背包/创造库存包
│   │   ├── RecipePackets.hpp
│   │   ├── ProtocolPackets.hpp
│   │   ├── GameStateChangePacket.hpp
│   │   ├── PlayerAbilitiesPacket.hpp
│   │   ├── BlockBreakAnimPacket.hpp
│   │   ├── ContainerPacketHandler.hpp
│   │   └── EntityMetadataSerializer.hpp
│   └── sync/                 # 同步系统
│       └── ChunkSync.hpp
│
├── perfetto/                 # 性能追踪
│   ├── PerfettoManager.hpp   # Perfetto 管理器
│   ├── PerfettoConfig.hpp    # 配置
│   ├── TraceEvents.hpp       # 追踪事件宏
│   ├── TraceCategories.hpp   # 追踪类别
│   └── CMakeLists.txt
│
├── physics/                  # 物理引擎
│   ├── PhysicsEngine.hpp     # 物理引擎
│   ├── PhysicsConstants.hpp  # 物理常量
│   ├── CollisionCache.hpp    # 碰撞缓存
│   └── collision/
│       └── CollisionShape.hpp
│
├── resource/                 # 资源包系统
│   ├── ResourceLocation.hpp  # 资源定位符
│   ├── IResourcePack.hpp     # 资源包接口
│   ├── FolderResourcePack.hpp # 文件夹资源包
│   ├── ZipResourcePack.hpp   # ZIP 资源包
│   ├── InMemoryResourcePack.hpp # 内存资源包
│   ├── ResourcePackList.hpp  # 资源包列表
│   ├── PackMetadata.hpp      # pack.mcmeta
│   ├── VanillaResources.hpp  # 原版资源
│
├── screen/                   # 屏幕类型
│   ├── IScreen.hpp
│   └── ScreenType.hpp
│
├── input/                    # 输入系统
│   └── KeyBinding.hpp
│
├── util/                     # 工具库
│   ├── Direction.hpp         # 方向枚举
│   ├── AxisAlignedBB.hpp     # AABB 碰撞箱
│   ├── NibbleArray.hpp       # 4位紧凑数组
│   ├── PlatformInfo.hpp      # 平台信息
│   ├── TimeUtils.hpp         # 时间工具
│   ├── assert/               # 断言库
│   │   ├── Assert.hpp
│   │   ├── AssertMacros.hpp
│   │   └── AssertAll.hpp
│   ├── cache/                # 缓存实现
│   │   ├── Long2IntLRUCache.hpp
│   │   └── OpenAddressingLRUCache.hpp
│   ├── math/                 # 数学工具
│   │   ├── MathUtils.hpp
│   │   ├── Vector2.hpp
│   │   ├── Vector3.hpp
│   │   ├── ray/
│   │   │   ├── Ray.hpp
│   │   │   └── Raycast.hpp
│   │   └── random/           # 随机数生成器
│   │       ├── IRandom.hpp
│   │       ├── Random.hpp
│   │       ├── Mt19937Random.hpp
│   │       ├── Xoroshiro128ppRandom.hpp
│   │       ├── Xoshiro256ppRandom.hpp
│   │       ├── LcgRandom.hpp
│   │       ├── UniformIntDistribution.hpp
│   │       └── UniformRealDistribution.hpp
│   ├── nbt/                  # NBT 序列化
│   │   └── Nbt.hpp
│   └── property/             # 属性系统
│       ├── IProperty.hpp
│       ├── Property.hpp
│       ├── BooleanProperty.hpp
│       ├── IntegerProperty.hpp
│       ├── EnumProperty.hpp
│       ├── DirectionProperty.hpp
│       ├── Properties.hpp
│       ├── FluidProperties.hpp
│       ├── StateContainer.hpp
│       └── StateHolder.hpp
│
└── world/                    # 世界系统
    ├── IWorld.hpp            # 世界接口
    ├── WorldConstants.hpp    # 世界常量
    ├── biome/                # 生物群系
    │   ├── Biome.hpp
    │   ├── BiomeId.hpp
    │   ├── Biomes.hpp
    │   ├── BiomeRegistry.hpp
    │   ├── BiomeProvider.hpp
    │   ├── BiomeGenerationSettings.hpp
    │   └── layer/            # 群系层生成
    │       ├── BiomeValues.hpp
    │       ├── LayerContext.hpp
    │       ├── LayerUtil.hpp
    │       ├── LayerCacheConfig.hpp
    │       └── transformers/
    │           ├── SourceLayers.hpp
    │           ├── ClimateLayers.hpp
    │           ├── EdgeLayers.hpp
    │           ├── ZoomLayers.hpp
    │           ├── BiomeLayers.hpp
    │           ├── MergeLayers.hpp
    │           └── TransformerTraits.hpp
    ├── block/                # 方块系统
    │   ├── Block.hpp
    │   ├── BlockState.hpp
    │   ├── BlockRegistry.hpp
    │   ├── Material.hpp
    │   ├── VanillaBlocks.hpp
    │   ├── ILiquidContainer.hpp
    │   └── blocks/
    │       ├── AirBlock.hpp
    │       ├── SimpleBlock.hpp
    │       ├── RotatedPillarBlock.hpp
    │       └── LiquidBlock.hpp
    ├── blockentity/          # 方块实体
    │   ├── BlockEntity.hpp
    │   ├── BlockEntityType.hpp
    │   ├── ContainerBlockEntity.hpp
    │   └── CraftingTableEntity.hpp
    ├── chunk/                # 区块系统
    │   ├── ChunkData.hpp
    │   ├── ChunkPos.hpp
    │   ├── ChunkStatus.hpp
    │   ├── ChunkPrimer.hpp
    │   ├── IChunk.hpp
    │   ├── ChunkLoadTicket.hpp
    │   ├── ChunkLoadTicketManager.hpp
    │   ├── ChunkDistanceGraph.hpp
    │   └── SingleChunkLifecycleManager.hpp
    ├── dimension/            # 维度
    │   └── DimensionRenderSettings.hpp
    ├── entity/               # 世界实体管理
    │   └── EntityManager.hpp
    ├── fluid/                # 流体系统
    │   ├── Fluid.hpp
    │   ├── FluidState.hpp
    │   ├── FlowingFluid.hpp
    │   ├── FluidRegistry.hpp
    │   ├── FluidTags.hpp
    │   └── fluids/
    │       ├── EmptyFluid.hpp
    │       ├── WaterFluid.hpp
    │       └── LavaFluid.hpp
    ├── gen/                  # 世界生成
    │   ├── noise/            # 噪声生成器
    │   │   ├── INoiseGenerator.hpp
    │   │   ├── ImprovedNoiseGenerator.hpp
    │   │   └── OctavesNoiseGenerator.hpp
    │   ├── chunk/            # 区块生成器
    │   │   ├── IChunkGenerator.hpp
    │   │   └── NoiseChunkGenerator.hpp
    │   ├── settings/         # 生成设置
    │   │   ├── DimensionSettings.hpp
    │   │   ├── NoiseSettings.hpp
    │   │   ├── ScalingSettings.hpp
    │   │   └── SlideSettings.hpp
    │   ├── surface/          # 地表构建器
    │   │   └── SurfaceBuilders.hpp
    │   ├── carver/           # 洞穴雕刻
    │   │   ├── WorldCarver.hpp
    │   │   ├── CaveCarver.hpp
    │   │   ├── CanyonCarver.hpp
    │   │   └── UnderwaterCarver.hpp
    │   ├── feature/          # 地物
    │   │   ├── Feature.hpp
    │   │   ├── ConfiguredFeature.hpp
    │   │   ├── FeatureSpread.hpp
    │   │   ├── ore/
    │   │   │   └── OreFeature.hpp
    │   │   ├── tree/          # 树木生成
    │   │   │   ├── TreeFeature.hpp
    │   │   │   ├── trunk/     # 树干放置器
    │   │   │   │   ├── TrunkPlacer.hpp
    │   │   │   │   ├── StraightTrunkPlacer.hpp
    │   │   │   │   └── TrunkPlacers.hpp
    │   │   │   └── foliage/   # 树叶放置器
    │   │   │       ├── FoliagePlacer.hpp
    │   │   │       ├── BlobFoliagePlacer.hpp
    │   │   │       └── FoliagePlacers.hpp
    │   │   ├── vegetation/    # 植被
    │   │   │   ├── FlowerFeature.hpp
    │   │   │   ├── GrassFeature.hpp
    │   │   │   ├── BigMushroomFeature.hpp
    │   │   │   ├── CactusFeature.hpp
    │   │   │   ├── SugarCaneFeature.hpp
    │   │   │   └── IceSpikeFeature.hpp
    │   │   ├── lake/
    │   │   │   └── LakeFeature.hpp
    │   │   └── template/      # 结构模板
    │   │       ├── Template.hpp
    │   │       ├── TemplateManager.hpp
    │   │       └── TemplateLoader.hpp
    │   ├── structure/        # 结构生成
    │   │   ├── Structure.hpp
    │   │   ├── JigsawStructure.hpp
    │   │   ├── StructureManager.hpp
    │   │   ├── StructureBoundingBox.hpp
    │   │   └── structures/
   │   │       ├── README.md
    │   │       ├── VillageStructure.hpp
    │   │       ├── MineshaftStructure.hpp
    │   │       ├── StrongholdStructure.hpp
    │   │       ├── DesertPyramidStructure.hpp
    │   │       ├── JungleTempleStructure.hpp
    │   │       ├── OceanMonumentStructure.hpp
   │   │       ├── ShipwreckStructure.hpp
   │   │       ├── OceanRuinStructure.hpp
    │   │       ├── RuinedPortalStructure.hpp
    │   │       └── BuriedTreasureStructure.hpp
    │   ├── jigsaw/           # Jigsaw 拼装系统
    │   │   ├── JigsawManager.hpp
    │   │   ├── JigsawPiece.hpp
    │   │   ├── JigsawPattern.hpp
    │   │   └── JigsawJunction.hpp
    │   ├── placement/        # 放置修饰器
    │   │   ├── Placement.hpp
    │   │   ├── PlacementUtils.hpp
    │   │   ├── Placements.hpp
    │   │   └── PlacementRegistry.hpp
    │   └── spawn/            # 世界生成生物生成
    │       └── WorldGenSpawner.hpp
    ├── spawn/                # 生物生成
    │   └── MobSpawnInfo.hpp
    ├── tick/                 # Tick 系统
    │   ├── base/
    │   │   └── TickPriority.hpp
    │   ├── list/
    │   │   ├── ITickList.hpp
    │   │   ├── EmptyTickList.hpp
    │   │   └── ServerTickList.hpp
    │   └── manager/
    │       └── TickManager.hpp
    ├── time/                 # 游戏时间
    │   └── GameTime.hpp
    └── weather/              # 天气系统
        └── WeatherUtils.hpp
```

## 子目录职责

### core/
核心类型定义和基础工具：
- **Types.hpp**: 基础类型别名 (i8, i16, i32, u8, std::string, Optional 等)
- **Result.hpp**: 错误处理系统 (Result<T>, Error, ErrorCode)
- **EnumSet.hpp**: 类型安全的枚举集合
- **settings/**: 设置系统基类和选项类型

### command/
命令系统，支持游戏内命令的解析、执行和 Tab 补全：
- 命令分发和节点树结构
- 参数类型解析 (实体、游戏模式、物品等)
- 命令异常和建议系统

### entity/
实体系统，包含所有游戏实体的基类和实现：
- **Entity.hpp**: 实体基类，位置、速度、旋转、玩家交互（processInitialInteract、applyPlayerInteraction）
- **ai/**: AI 系统 (控制器、目标、寻路)
- **animal/**: 动物实体 (猪、牛、羊、鸡)
- **attribute/**: 属性系统 (生命值、移动速度等)
- **combat/**: 战斗系统
- **damage/**: 伤害追踪
- **inventory/**: 背包和容器
- **living/**: 生物实体基类
- **loot/**: 战利品表系统
- **mob/**: 生物实体
- **movement/**: 移动系统 (自动跳跃)

### item/
物品系统：
- 物品基类、物品堆、物品注册表
- **crafting/**: 合成配方 (有序、无序)
- **enchantment/**: 附魔系统
- **tier/**: 工具等级 (木质、石质、铁质等)
- **tool/**: 工具物品 (镐、斧、锹、锄、剑)

### network/
网络通信系统：
- **connection/**: 连接管理 (本地连接用于集成服务器)
- **packet/**: 数据包序列化和处理
- **sync/**: 区块同步

### perfetto/
性能追踪系统 (Perfetto SDK 集成)：
- 追踪管理器、配置、事件宏、追踪类别

### physics/
物理引擎：
- 碰撞检测和碰撞缓存
- AABB 碰撞箱

### resource/
资源包系统，支持 MC 1.12-1.19+ 资源包格式：
- 资源定位符、资源包接口
- 文件夹/ZIP/内存资源包实现

### screen/
屏幕类型定义 (主菜单、背包、暂停等)

### input/
输入系统，按键绑定

### util/
工具库：
- **assert/**: 断言库 (支持堆栈跟踪)
- **cache/**: LRU 缓存实现
- **math/**: 数学工具和随机数生成器
- **nbt/**: NBT 序列化
- **property/**: 方块状态属性系统

### world/
世界系统，最大的子系统：

- **biome/**: 生物群系 (170 种群系，层式生成)
- **block/**: 方块系统 (方块状态、材质、注册表)
- **blockentity/**: 方块实体 (工作台、箱子等)
- **chunk/**: 区块系统 (数据、生命周期、加载票、取消调度)
- **fluid/**: 流体系统 (水、岩浆)
- **gen/**: 世界生成 (噪声、雕刻器、地物、结构)
- **lighting/**: 光照系统 (方块光、天空光)
- **redstone/**: 红石系统 (信号传输、逻辑运算)
- **spawn/**: 生物生成信息
- **tick/**: Tick 调度系统
- **time/**: 游戏时间 (昼夜循环)
- **weather/**: 天气系统

区块系统已经把“票据 → 生命周期 → 调度 → 取消”这条链路拆开：`ChunkLoadTicketManager` 负责汇聚不同来源的加载票据，`SingleChunkLifecycleManager` 负责每个区块的生命周期和请求代际，服务端再据此决定是否继续排队、生成或丢弃过期结果。

## 子目录之间的关系

```
                    ┌─────────┐
                    │  core   │ (基础类型、错误处理)
                    └────┬────┘
                         │
         ┌───────────────┼───────────────┐
         │               │               │
    ┌────▼────┐    ┌────▼────┐    ┌────▼────┐
    │   util  │    │ command │    │ screen  │
    └────┬────┘    └────┬────┘    └─────────┘
         │              │
         │    ┌─────────┴─────────┐
         │    │                   │
    ┌────▼────▼────┐    ┌────────▼────────┐
    │    entity    │◄───┤     network     │
    │  (实体系统)   │    │   (数据包同步)   │
    └────┬────┬────┘    └─────────────────┘
         │    │
    ┌────▼────▼────┐    ┌─────────────────┐
    │     item     │    │    resource     │
    │  (物品系统)   │    │  (资源包加载)    │
    └────┬─────────┘    └─────────────────┘
         │
    ┌────▼────────────────────────────────────┐
    │                 world                    │
    │  (世界系统 - 方块、群系、生成、光照等)      │
    └────────────────────┬────────────────────┘
                         │
    ┌────────────────────▼────────────────────┐
    │                physics                   │
    │  (物理引擎 - 碰撞检测)                     │
    └─────────────────────────────────────────┘
```

**依赖关系说明**:
- `core` 是所有模块的基础
- `entity` 依赖 `item` (手持物品)、`world` (位置)
- `network` 与 `entity` 相互依赖 (实体同步)
- `world` 依赖 `resource` (资源加载)
- `physics` 依赖 `world` (碰撞检测)
- `command` 依赖 `entity` 和 `world` (命令执行)

## 模块职责

### 整体职责

`mc_common` 库是项目的共享核心，提供：

1. **类型系统**: 统一的基础类型、错误处理、常量定义
2. **游戏逻辑**: 实体、物品、方块、群系等核心游戏机制
3. **世界生成**: 完整的 MC 1.16.5 兼容地形生成系统
4. **网络通信**: 数据包序列化和本地连接
5. **资源加载**: 资源包解析和版本兼容
6. **性能追踪**: Perfetto 集成

### 输入和输出

**输入**:
- 资源包文件 (ZIP/文件夹)
- NBT 数据文件
- 配置文件 (设置)
- 网络数据包

**输出**:
- 游戏状态更新
- 网络数据包
- 性能追踪数据
- 序列化的世界数据

### 依赖项

**外部依赖** (通过 vcpkg):
- `glm` - 数学库
- `spdlog` - 日志
- `nlohmann_json` - JSON 解析
- `LibArchive` - ZIP 解压
- `ZLIB` - 压缩
- `perfetto` - 性能追踪

**内部依赖**:
- 无 (这是最底层模块)

### 使用方法

在 CMake 中链接:
```cmake
target_link_libraries(your_target mc_common)
```

包含头文件:
```cpp
#include "common/core/Types.hpp"
#include "common/world/block/Block.hpp"
#include "common/entity/Entity.hpp"
// ... 其他头文件
```

### 容易踩的坑

1. **命名空间**: 所有类型在 `mc` 命名空间下
   ```cpp
   mc::BlockPos pos;      // 正确
   BlockPos pos;          // 错误
   ```

2. **智能指针**: 实体和方块使用原始指针或引用，所有权由管理器控制
   ```cpp
   Block* block = VanillaBlocks::STONE;  // 静态指针，无需释放
   ```

3. **错误处理**: 使用 `Result<T>` 而非异常
   ```cpp
   Result<int> divide(int a, int b) {
       if (b == 0) return Error(ErrorCode::InvalidArgument, "Division by zero");
       return a / b;
   }
   ```

4. **区块坐标 vs 方块坐标**:
   ```cpp
   ChunkCoord cx = pos.x >> 4;  // 方块坐标转区块坐标
   BlockCoord bx = pos.x & 15;  // 区块内方块坐标
   ```

5. **日志级别**: 项目使用 `spdlog`，默认只输出 `info` 及以上级别，`debug` 日志不可见

6. **资源定位符**: 使用 `minecraft:` 命名空间
   ```cpp
   ResourceLocation loc("minecraft:stone");  // 正确
   ResourceLocation loc("stone");             // 也可以，自动补全
   ```

7. **流体系统未完成**: 参考 `world/fluid/FLUID_TODO.md`

8. **性能追踪**: 需要编译时启用 `-DMC_ENABLE_TRACING=ON`

## 测试用例

测试文件位于 `tests/common/`，共 56 个测试文件:

| 测试文件 | 测试内容 |
|---------|---------|
| `test_core.cpp` | 核心类型、Result、EnumSet |
| `test_math.cpp` | 数学工具、向量运算 |
| `test_block.cpp` | 方块系统、方块状态、属性 |
| `test_item.cpp` | 物品系统、物品堆 |
| `test_block_item.cpp` | 方块物品 |
| `test_inventory.cpp` | 背包系统 |
| `test_container.cpp` | 容器系统 |
| `test_entity.cpp` | 实体系统 |
| `test_world.cpp` | 世界系统 |
| `test_biome.cpp` | 生物群系 |
| `test_chunk_generation.cpp` | 区块生成 |
| `test_chunkloadticket.cpp` | 区块加载票 |
| `test_chunksync.cpp` | 区块同步 |
| `test_carver.cpp` | 洞穴雕刻 |
| `test_surface.cpp` | 地表构建 |
| `test_tree_feature.cpp` | 树木生成 |
| `test_ore_feature.cpp` | 矿石生成 |
| `test_decoration_stage.cpp` | 装饰阶段 |
| `test_time.cpp` | 游戏时间 |
| `test_tick_manager.cpp` | Tick 管理器 |
| `test_localconnection.cpp` | 本地连接 |
| `test_network.cpp` | 网络数据包 |
| `test_property.cpp` | 属性系统 |
| `test_fluid.cpp` | 流体系统 |
| `entity/PlayerMovementTest.cpp` | 玩家移动 |
| `entity/inventory/CraftingInventoryTest.cpp` | 合成背包 |
| `entity/loot/LootTest.cpp` | 战利品表 |
| `entity/movement/AutoJumpTest.cpp` | 自动跳跃 |
| `item/crafting/*.cpp` | 合成配方测试 |
| `item/enchantment/EnchantmentTest.cpp` | 附魔系统 |
| `item/tool/ToolTests.cpp` | 工具测试 |
| `util/assert/AssertTest.cpp` | 断言库 |
| `util/cache/CacheBenchmark.cpp` | 缓存性能测试 |
| `world/biome/layer/*.cpp` | 群系层生成 |
| `world/fluid/FluidTest.cpp` | 流体系统 |
| `world/lighting/*.cpp` | 光照系统 |
| `world/tick/ServerTickListTest.cpp` | 服务器 Tick 列表 |
| `world/gen/*.cpp` | 世界生成测试 |

运行测试:
```powershell
./build/bin/Release/mc_tests.exe
```

## 构建配置

```cmake
# 启用性能追踪
cmake -B build -DMC_ENABLE_TRACING=ON ...

# 启用 Vulkan 验证层 (仅客户端)
cmake -B build -DMC_ENABLE_VULKAN_VALIDATION=ON ...

# 启用 Sanitizer
cmake -B build -DMC_ENABLE_SANITIZERS=ON ...
```

## 相关文档

- [CLAUDE.md](../../CLAUDE.md) - 项目整体文档
- [world/fluid/FLUID_TODO.md](world/fluid/FLUID_TODO.md) - 流体系统待完成计划
- [util/assert/README.md](util/assert/README.md) - 断言库使用说明
- [util/nbt/README.md](util/nbt/README.md) - NBT 库说明
