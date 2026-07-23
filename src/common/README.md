# Common Module

公共模块 (`mc_common`) 是 Cubium 项目的核心共享库，包含客户端和服务端共用的所有代码。

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
│   ├── Entity.hpp            # 实体基类（含持久化随机数生成器 getRandom()）
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
├── profiler/                 # 性能追踪（Perfetto + Tracy 双轨）
│   ├── ProfilerManager.hpp   # 门面单例（管理两套后端）
│   ├── PerfettoBackend.hpp   # Perfetto 后端
│   ├── ProfilerConfig.hpp    # 编译时开关（MC_ENABLE_TRACING / MC_ENABLE_TRACY）
│   ├── TraceEvents.hpp       # 追踪事件宏（双轨四种组合分支）
│   ├── TraceCategories.hpp   # 追踪类别枚举树
│   └── CMakeLists.txt        # perfetto_sdk + TracyClient + mc_profiler
│
├── particle/                 # 粒子类型定义
│   ├── ParticleTypes.hpp     # 粒子类型枚举 (ParticleTypeId) 及辅助函数
│   └── README.md             # 粒子系统说明
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
│   └── ScreenType.hpp
│
├── stats/                    # 统计常量
│   └── Stats.hpp             # 自定义统计资源位置常量（与 MC Java Stats.java 对应）
│
├── input/                    # 输入系统
│   ├── KeyBinding.hpp         # Keys命名空间（平台无关键码常量）+ KeyBinding类（可重映射按键绑定）
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
    │   ├── noise/            # MC 1.18+ 噪声生成器
    │   │   ├── PerlinNoise.hpp
    │   │   ├── NormalNoise.hpp
    │   │   └── SimplexNoise.hpp
    │   ├── chunk/            # 区块生成器
    │   │   ├── IChunkGenerator.hpp
    │   │   └── NoiseChunkGenerator.hpp
    │   ├── settings/         # 生成设置
    │   │   ├── DimensionSettings.hpp
    │   │   ├── NoiseSettings.hpp
    │   │   ├── ScalingSettings.hpp
    │   │   └── SlideSettings.hpp
    │   ├── surface/          # MC 1.21 地表规则系统
    │   │   └── SurfaceRules.hpp
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
    │   │   │   ├── BigMushroomFeature.hpp
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
    │   │   ├── JigsawAssembler.hpp  # 组装算法
    │   │   ├── JigsawPlacer.hpp     # 放置器
    │   │   ├── JigsawTransform.hpp  # 坐标变换
    │   │   ├── JigsawPiece.hpp      # 拼图块基类
    │   │   ├── TemplatePool.hpp     # 模板池
    │   │   └── JigsawJunction.hpp   # 连接点信息
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

区块系统已经把”票据 → 生命周期 → 调度 → 取消”这条链路拆开：`ChunkLoadTicketManager` 负责汇聚不同来源的加载票据，`SingleChunkLifecycleManager` 负责每个区块的生命周期和请求代际，服务端再据此决定是否继续排队、生成或丢弃过期结果。

## 模块间依赖关系

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

## 外部依赖
- `glm` - 数学库
- `spdlog` - 日志
- `nlohmann_json` - JSON 解析
- `LibArchive` - ZIP 解压
- `ZLIB` - 压缩
- `profiler` - 性能追踪（Perfetto + Tracy 双轨，对应 CMake target `mc_profiler`）

**下游模块**:
- `mc_client` - 客户端
- `mc_server` - 服务端

## 容易踩的坑

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

7. **流体系统**: 参考 `world/fluid/FLUID_TODO.md`

8. **性能追踪**: Perfetto + Tracy 双轨，默认均启用。可用 `-DMC_ENABLE_TRACING=OFF` / `-DMC_ENABLE_TRACY=OFF` 独立关闭。详见 `profiler/README.md`。
