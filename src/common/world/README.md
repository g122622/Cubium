# World Module

世界模块是 Cubium 世界模拟的核心，提供地形生成、方块/流体管理、光照系统、区块处理和世界状态管理。实现兼容 MC 1.16.5 的世界机制。

## 目录结构

```
world/
├── IWorld.hpp/cpp              # 世界访问接口（含 isBlockInLine 方块射线遍历、containsAnyLiquid 碰撞箱流体检测）
├── IWorldWriter.hpp            # 世界写入接口（生成用）
├── WorldConstants.hpp          # 世界常量（高度限制、区块尺寸等）和坐标转换工具
├── WorldConfig.hpp             # 世界配置
├── WorldEvents.hpp             # 世界事件ID常量
├── gameevent/                  # 游戏事件系统
│   ├── GameEvent.hpp           # 游戏事件类（含 Context）
│   ├── GameEvents.hpp          # 原版游戏事件常量定义
│   └── README.md               # 游戏事件系统文档
├── GlobalPos.hpp               # 全局位置类型
├── biome/                      # 生物群系系统
│   ├── Biome.hpp/cpp           # 生物群系定义
│   ├── BiomeClimate.hpp/cpp    # 气候参数
│   ├── BiomeIds.hpp/cpp        # 生物群系ID常量
│   ├── BiomeFactory.hpp        # 工厂函数声明
│   ├── BiomeFactoryOverworld.cpp  # 主世界工厂
│   ├── BiomeFactoryNether.cpp     # 下界工厂
│   ├── BiomeFactoryEnd.cpp        # 末地工厂
│   ├── BiomeTag.hpp/cpp           # 生物群系标签类型
│   ├── BiomeTags.hpp/cpp          # 原版生物群系标签常量（IS_OCEAN 等）
│   ├── BiomeTagLoader.hpp/cpp     # 生物群系标签加载器
│   ├── BiomeSource.hpp/cpp     # 生物群系源基类 (IBiomeSource)
│   ├── BiomeGenerationSettings.hpp/cpp  # 群系生成配置
│   ├── BiomeRegistry.hpp/cpp   # 群系注册表
│   ├── Biomes.hpp              # 聚合头文件
│   ├── BiomeEffects.hpp        # 群系视觉效果（仅头文件，无 .cpp）
│   ├── BiomeAmbientSounds.hpp  # 群系环境音效
│   ├── climate/                # 气候系统
│   └── source/                 # 群系源实现
├── block/                      # 方块系统
│   ├── Block.hpp/cpp           # 方块基类
│   ├── BlockPos.hpp            # 方块位置类型
│   ├── BlockRegistry.hpp/cpp   # 方块注册表
│   ├── Material.hpp/cpp        # 方块材质
│   ├── HarvestTool.hpp         # 工具类型定义
│   ├── ILiquidContainer.hpp    # 液体容器接口
│   ├── VanillaBlocks.hpp/cpp   # 原版方块定义
│   ├── blocks/                 # 具体方块类型（按功能分类）
│   │   ├── agricultural/       # 农作物类
│   │   ├── building/           # 建筑方块
│   │   ├── cave/               # 洞穴相关
│   │   ├── copper/             # 铜相关
│   │   ├── coral/              # 珊瑚类
│   │   ├── decorative/         # 装饰方块
│   │   ├── dirt/               # 泥土类
│   │   ├── end/                # 末地相关
│   │   ├── functional/         # 功能方块
│   │   ├── ice/                # 冰类
│   │   ├── mangrove/           # 红树林
│   │   ├── mob/                # 生物相关
│   │   ├── nether/             # 下界相关
│   │   ├── ocean/              # 海洋相关
│   │   ├── pale_garden/        # 苍白花园
│   │   ├── redstone/           # 红石方块
│   │   ├── sculk/              # 幽匿块
│   │   ├── special/            # 特殊方块
│   │   ├── trial/              # 试炼相关
│   │   └── vegetation/         # 植被类
│   ├── dispense/               # 发射器行为
│   └── registry/               # 方块注册辅助
├── blockevent/                  # 方块事件系统（服务端→客户端方块动画同步）
│   ├── BlockEventData.hpp       # 方块事件数据结构（位置、方块类型、事件参数）
│   └── README.md                # 方块事件系统文档
├── blockentity/                # 方块实体
│   ├── core/                   # 核心方块实体
│   ├── interactive/            # 交互类（告示牌、床等）
│   ├── processing/             # 处理类（高炉、烟熏炉等）
│   ├── redstone/               # 红石类
│   ├── storage/                # 存储类（箱子、漏斗等）
│   ├── transport/              # 运输类
│   └── trial/                  # 试炼类
├── border/                     # 世界边界系统
│   └── WorldBorder.hpp/cpp     # 边界管理（伤害、渐变动画）
├── chunk/                      # 区块管理
│   ├── ChunkData.hpp/cpp       # 区块数据存储
│   ├── ChunkPos.hpp            # 区块位置类型
│   ├── ChunkPrimer.hpp/cpp     # 生成中间态区块
│   ├── ChunkStatus.hpp/cpp     # 区块生成阶段
│   ├── IChunk.hpp/cpp          # 区块接口
│   ├── ChunkDistanceGraph.hpp/cpp    # BFS距离计算
│   ├── ChunkLoadTicket.hpp     # 加载票据类型
│   ├── ChunkLoadTicketManager.hpp/cpp # 多源票据聚合
│   └── SingleChunkLifecycleManager.hpp/cpp # 单区块生命周期
├── dimension/                  # 维度系统
│   ├── DimensionRenderSettings.hpp
│   └── teleport/               # 维度传送
├── entity/                     # 世界实体管理
│   └── EntityManager.hpp/cpp   # 实体生命周期和查询
├── explosion/                  # 爆炸系统
│   ├── Explosion.hpp/cpp       # 爆炸核心逻辑
│   ├── ExplosionContext.hpp/cpp # 爆炸上下文
│   └── ExplosionMode.hpp       # 爆炸模式枚举
├── fluid/                      # 流体系统
│   ├── Fluid.hpp/cpp           # 流体基类
│   ├── FlowingFluid.hpp/cpp    # 流动流体力学
│   ├── FluidRegistry.hpp/cpp   # 流体注册表
│   ├── FluidTags.hpp/cpp       # 流体标签
│   ├── FLUID_TODO.md           # 流体系统TODO
│   └── fluids/                 # 具体流体（水、岩浆等）
├── gamerule/                   # 游戏规则
│   ├── GameRule.hpp/cpp        # 单个规则定义
│   └── GameRules.hpp/cpp       # 规则集合管理
├── gen/                        # 世界生成
│   ├── aquifer/                # 含水层生成
│   ├── carver/                 # 洞穴/峡谷雕刻
│   ├── chunk/                  # 区块生成器
│   ├── density/                # 密度函数
│   ├── feature/                # 世界特性（树、矿等）
│   │   ├── cave/               # 洞穴特性
│   │   ├── fungus/             # 菌类
│   │   ├── gateway/            # 末地折跃门
│   │   ├── lake/               # 湖
│   │   ├── nether/             # 下界特性
│   │   ├── ocean/              # 海洋特性
│   │   ├── ore/                # 矿石
│   │   ├── predicate/          # 放置条件
│   │   ├── spike/              # 尖刺（冰刺等）
│   │   ├── state/              # 状态特性
│   │   ├── template/           # 模板特性
│   │   ├── tree/               # 树生成
│   │   └── vegetation/         # 植被
│   ├── jigsaw/                 # 拼图结构组装
│   ├── noise/                  # MC 1.18+ 噪声生成器
│   ├── placement/              # 特性放置
│   ├── settings/               # 生成设置
│   ├── spawn/                  # 世界出生点
│   ├── structure/              # 结构生成
│   │   ├── pools/              # 结构池
│   │   └── structures/         # 具体结构
│   ├── surface/                # MC 1.21 地表规则系统
│   └── valueprovider/          # 值提供器
├── lighting/                   # 光照系统
│   ├── IChunkLightProvider.hpp # 区块光照提供接口
│   ├── LightType.hpp           # 光照类型枚举
│   ├── InternalLightUtils.hpp/cpp # 内部光照工具
│   ├── engine/                 # 光照引擎
│   ├── manager/                # 光照管理器
│   └── storage/                # 光照存储
├── map/                        # 地图系统
│   ├── MapData.hpp/cpp         # 地图数据核心 - 128x128像素、装饰物、旗帜、展示框标记
│   ├── MapDecoration.hpp/cpp   # 地图装饰物 - 27种装饰类型定义、NBT/网络序列化
│   ├── MapBanner.hpp/cpp       # 旗帜标记 - 旗帜位置/颜色记录、fromWorld世界交互
│   ├── MapFrame.hpp/cpp        # 展示框标记 - 展示框位置/旋转记录
│   ├── MapIdTracker.hpp/cpp    # 地图ID追踪器 - 自增ID分配
│   ├── MapDataManager.hpp/cpp  # 地图数据管理器 - CRUD、tick更新
│   ├── MaterialColor.hpp/cpp   # 材质颜色 - 59种颜色定义和阴影计算
│   └── README.md               # 地图系统详细文档
├── redstone/                   # 红石系统
│   ├── RedstoneSystem.hpp/cpp  # 红石系统管理
│   ├── RedstonePower.hpp/cpp   # 信号强度计算
│   ├── RedstoneContext.hpp/cpp # 递归保护
│   └── RedstoneHelper.hpp/cpp  # 辅助函数
├── spawn/                      # 生物生成信息
│   └── MobSpawnInfo.hpp/cpp
├── storage/                    # 世界持久化
│   ├── backend/                # 存储后端
│   ├── blockentity/            # 方块实体存储
│   ├── core/                   # 核心存储接口
│   ├── db/                     # 数据库
│   ├── entity/                 # 实体存储
│   ├── list/                   # 世界列表
│   ├── player/                 # 玩家数据存储
│   ├── reader/                 # 存档读取器
│   │   ├── bedrock/            # 基岩版格式
│   │   └── java/               # Java版格式
│   ├── request/                # 世界操作请求
│   ├── save/                   # 存档保存
│   ├── section/                # Section存储
│   ├── snapshot/               # 世界快照
│   └── task/                   # 存储任务
├── tick/                       # Tick调度
│   ├── base/                   # 基础类型
│   ├── list/                   # Tick列表
│   └── manager/                # Tick管理器
├── time/                       # 游戏时间
│   └── GameTime.hpp/cpp        # 昼夜循环
├── village/                    # 村庄系统
│   ├── Village.hpp/cpp         # 村庄核心
│   ├── VillageManager.hpp/cpp  # 村庄管理
│   ├── VillageGossip.hpp/cpp   # 村庄流言
│   ├── VillageGossipType.hpp/cpp
│   ├── poi/                    # 兴趣点
│   ├── raid/                   # 袭击
│   ├── trade/                  # 交易
│   └── README.md
└── weather/                    # 天气系统
    ├── WeatherConstants.hpp    # 天气常量
    ├── WeatherState.hpp        # 天气状态
    └── WeatherUtils.hpp/cpp    # 天气工具
```

## 内部模块关系

```
                    ┌─────────────────┐
                    │     IWorld      │
                    │   (接口层)       │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│   ChunkData     │ │   Block/Fluid   │ │   Lighting      │
│   ChunkManager  │ │   Registry      │ │   System        │
└────────┬────────┘ └────────┬────────┘ └────────┬────────┘
         │                   │                   │
         └───────────────────┼───────────────────┘
                             │
                    ┌────────┴────────┐
                    │   Generation    │
                    │ (Noise, Biome,  │
                    │  Feature, etc)  │
                    └─────────────────┘
```

**核心数据流**：
- `IWorld` 作为统一访问接口，被 `ServerWorld`/`ClientWorld` 实现
- 区块通过 `ChunkLoadTicketManager` → `SingleChunkLifecycleManager` → `ChunkPrimer` → `ChunkData` 流转
- 生成器链：`NoiseChunkGenerator` 调用 `IBiomeSource` → 噪声 → 地表 → 雕刻器 → 特性
- 光照由 `WorldLightManager` 协调 `SkyLightEngine`/`BlockLightEngine`，使用 `SWMRNibbleArray` 存储

## 上下游外部依赖关系

### 被依赖方（谁依赖了这个模块）
- `src/server/world/` - 服务端世界实现（ServerWorld、区块加载、玩家管理）
- `src/client/world/` - 客户端世界实现（ClientWorld、渲染数据）
- `src/server/network/` - 数据包序列化（区块数据、方块更新）
- `src/client/renderer/` - 渲染数据消费（区块网格、光照）

### 依赖方（这个模块依赖谁）
- `common/core/` - 基础类型（i8/u8/i32/u64/f32/f64、Result、Constants）
- `common/util/math/` - 数学工具（向量、噪声、坐标转换）
- `common/util/nbt/` - NBT序列化
- `common/util/property/` - 方块状态属性系统
- `common/physics/collision/` - 碰撞形状
- `common/entity/` - 实体基类
- `common/resource/` - 资源定位
- `glm` - 数学库
- `spdlog` - 日志

## 容易踩的坑

### 1. 区块生成顺序
**问题**：在区块达到 FULL 状态前访问方块数据会崩溃。
**解决**：检查区块状态或使用 future 等待。

### 2. 坐标转换负数处理
**问题**：`x / 16` 对负数会向下取整，导致错误。
**解决**：使用 `WorldConstants.hpp` 中的 `toChunkCoord()`、`toLocalCoord()`。

### 3. 生物群系采样分辨率
**问题**：生物群系以 4x4x4 块存储，逐块查询会出错。
**解决**：使用 `BiomeContainer` 进行正确的 4x4x4 采样。

### 3b. 已移除的接口
- `Biome::Category` 枚举已移除，改用 `BiomeTags` 系统判断生物群系分类
- `BiomeEffects.cpp` 已移除，`BiomeEffects.hpp` 为 header-only
- `isOceanOrRiverBiome()` 已移除，改用 `BiomeTags::IS_OCEAN` / `BiomeTags::IS_RIVER`
- `Biome::temperature()` 访问器已移除，改用 `biome.climate().temperature`

### 4. 流体级别方向
**问题**：流体级别向源头增加而非减少。
**解决**：级别 8 = 源头，级别 1 = 最远离源头。

### 5. 光照线程安全
**问题**：光照引擎更新时读取光照数据会竞争。
**解决**：使用 `SWMRNibbleArray` 的线程安全访问器或同步访问。

### 6. BlockState 指针有效性
**问题**：存储 BlockState 指针超出注册表生命周期。
**解决**：BlockState 指针在程序整个生命周期有效（注册表永不卸载）。

### 7. IWorld BlockPos 重载
**问题**：非 `IWorld` 接口强制添加 `BlockPos` 重载会导致接口膨胀。
**解决**：`IWorld` 已提供 `BlockPos` 重载。`ISpawnWorldReader`、`ClientWorld`、光照/生成辅助类保持原生 xyz 签名，不要强制添加 `BlockPos` 重载。

### 8. ServerWorld setBlockState 测试
**问题**：测试 `ServerWorld::setBlockState()` 时，未初始化的世界会触发光照更新断言。
**解决**：测试前先初始化世界，否则 `m_lightManager` 为 null 会触发断言。

### 9. BlockUpdatePacket 发送
**问题**：直接从服务器代码发送 `BlockUpdatePacket` 会绕过去重和批处理。
**解决**：通过 `ServerWorld::setOnBlockChanged()` 供给 `BlockUpdateSyncManager`，保持去重和 tick 结束刷新的集中化。

### 9a. notifyBlockUpdate vs setBlockState
**问题**：方块实体内部数据变化后调用 `setBlockState(pos, state, 3)` 通知客户端，但 `setBlockState` 在 `oldState == newState` 时直接返回 false，不触发 `m_onBlockChanged` 回调，导致客户端收不到更新。
**解决**：使用 `IWorld::notifyBlockUpdate(pos)` 代替。该方法即使方块状态未改变也会触发客户端同步通知，对应 MC Java 的 `Level.sendBlockUpdated(pos, oldState, newState, flags)`。适用于营火烹饪物品变化、箱子开合状态同步等方块实体数据变化场景。

### 10. Tick 优先级溢出
**问题**：过多调度的 tick 导致性能问题。
**解决**：`TickManager` 每个 tick 有处理上限（65536），使用优先级处理关键更新。

### 11. 高度常量使用
**问题**：硬编码 0、256、16 等高度/尺寸数字。
**解决**：使用 `mc::world` 命名空间下的常量（`MIN_BUILD_HEIGHT`、`MAX_BUILD_HEIGHT`、`CHUNK_WIDTH` 等）。`CHUNK_HEIGHT`（384）与 `MAX_BUILD_HEIGHT`（320）值不同，语义也完全不同，切勿混淆。这些常量定义在 `WorldConstants.hpp` 中。

**维度感知高度**：`IWorld` 提供虚方法 `getMinBuildHeight()` 和 `getMaxBuildHeight()`，默认返回 `world::MIN_BUILD_HEIGHT` 和 `world::MAX_BUILD_HEIGHT`。`ServerWorld` 覆写了这两个方法，基于 `DimensionType` 返回维度特定的建筑高度范围（如下界 -64~128、末地 0~256 等）。需要维度感知高度时应优先使用这两个虚方法而非全局常量。`SpreadAlgorithm` 等算法已适配此接口。

### 12. canSeeSky 实现
**问题**：错误假设 `canSeeSky()` 只检查顶部是否有方块。
**解决**：实际基于天空光照 >= 15 判断，正确处理透明方块（玻璃、水）和部分方块（台阶、楼梯）。

### 13. IWorld::raidManager() 接口
- `IWorld` 新增虚方法 `raidManager()`，返回袭击管理器指针（`RaidManager*`）
- `ServerWorld` 覆写实现，返回实际的 `m_raidManager`
- 客户端世界（`ClientWorld`）返回 `nullptr`，不触发袭击相关逻辑
- 典型使用场景：`VillagerEntity::tick()` 通过 `world->raidManager()` 检查当前区域是否有活跃袭击，若有则广播恐慌粒子

### 13a. IWorld::containsAnyLiquid() 碰撞箱流体检测

`IWorld::containsAnyLiquid(const AxisAlignedBB& box)` 遍历碰撞箱覆盖的所有方块位置，检查是否存在流体。用于实体生成位置验证（如僵尸增援需确保生成位置无液体），以及实体物理检测等场景。与逐坐标的 `isWaterAt()`/`isLavaAt()` 不同，此方法接受碰撞箱参数，一次性检测整个体积。

### 14. IWorld 进度触发回调

`IWorld` 定义了一组虚方法用于通知世界进度相关事件，`ServerWorld` 重写这些方法来发布 `ServerEventBus` 事件，由 `AdvancementEventHandler` 订阅并触发对应的成就检测。完整回调列表：

| 回调方法 | 发布事件 | 触发的 CriterionTrigger |
|---------|---------|----------------------|
| `onBlockPlaced()` | `BlockPlaceEvent` | `PlacedBlockTrigger` |
| `onBredAnimals()` | `BredAnimalsEvent` | `BredAnimalsTrigger` |
| `onVillagerTrade()` | `VillagerTradeEvent` | `VillagerTradeTrigger` |
| `onZombieVillagerCured()` | `CuredZombieVillagerEvent` | `CuredZombieVillagerTrigger` |
| `onChanneledLightning()` | `ChanneledLightningEvent` | `ChanneledLightningTrigger` |
| `onTameAnimal()` | `TameAnimalEvent` | `TameAnimalTrigger` |
| `onSummonedEntity()` | `SummonedEntityEvent` | `SummonedEntityTrigger` |
| `onConsumeItem()` | `ConsumeItemEvent` | `ConsumeItemTrigger` |
| `onItemDurabilityChange()` | `ItemDurabilityEvent` | `ItemDurabilityTrigger` |
| `onEnchantItem()` | `EnchantItemEvent` | `EnchantedItemTrigger` |
| `onFilledBucket()` | `FilledBucketEvent` | `FilledBucketTrigger` |
| `onEnterBlock()` | `EnterBlockEvent` | `EnterBlockTrigger` |
| `onSlideDownBlock()` | `SlideDownBlockEvent` | `SlideDownBlockTrigger` |
| `onBeeNestDestroyed()` | `BeeNestDestroyedEvent` | `BeeNestDestroyedTrigger` |
| `onPlayerDestroyItem()` | `PlayerDestroyItemEvent` | （暂无对应触发器） |

**调用模式**：游戏逻辑（common 模块）调用 `m_world->onXxx(...)` → `ServerWorld::onXxx()` 重写 → `ServerEventBus::publish(event)` → `AdvancementEventHandler` 事件处理器 → `CriterionTriggers::getTrigger<T>()` → `AbstractCriterionTrigger<T>::trigger()`。

**注意事项**：
- `ClientWorld` 不重写这些回调，默认空实现（void 参数），不触发任何事件
- `PlayerInteractedWithEntityTrigger` 不通过 IWorld 回调触发，而是在 `ServerPlayRouter` 的 UseEntity/Interact 分支中直接调用 `AbstractCriterionTrigger::trigger()`（TODO(Phase6) 完成接线）
- `SummonedEntityTrigger` 当前仅在 `/summon` 命令中触发。MC Java 中还会在建造铁傀儡/雪傀儡/凋灵时触发，但当前傀儡建造代码缺少玩家上下文信息，需要重构后才能添加

### 15. IWorld::blockEvent() 方块事件系统

方块事件用于服务端向客户端同步方块动画和状态变化（如箱子开合、陶罐摇晃、末地折跃门冷却等）。

**调用流程**：
1. 方块实体调用 `world.blockEvent(pos, block, paramA, paramB)` 将事件入队
2. `ServerWorld::tick()` 中 `runBlockEvents()` 处理队列，验证方块仍匹配后执行事件
3. 执行成功后通过回调广播 `BlockEventPacket` 给附近客户端
4. 客户端收到后调用 `Block::triggerEvent()` → `BlockEntity::triggerEvent()` 处理视觉效果

**注意事项**：
- `BlockEventData` 使用 `operator==` 和 `std::hash` 去重，必须保持一致
- 事件参数 `paramA`/`paramB` 含义因方块类型而异
- 区块未加载的事件会重新入队等待下次处理
- `Block::triggerEvent()` 默认委托给 `BlockEntity::triggerEvent()`，子类可覆写

### 16. IWorld::getOrLoadChunk() 同步区块加载

**问题**：common 层代码需要在判断区块是否为空时触发区块加载（如末地折跃门出口生成扫描外岛区块），但 common 不能依赖 server 层的 `ServerChunkManager::requestFullChunkSync`。

**解决**：`IWorld` 提供虚方法 `getOrLoadChunk(ChunkCoord x, ChunkCoord z)`，对应 MC Java 的 `Level.getChunk(x, z, require=true)`：
- 默认实现委托给 `getChunk(x, z)`（仅查内存缓存，不触发加载）
- `ServerWorld` 覆写为调用 `m_chunkManager->requestFullChunkSync(x, z)`，同步触发区块加载/生成
- `WorldGenRegion` 等只读快照实现继承默认行为（仅查内存）
- `BaseTestWorld`/`BaseChunkBackedTestWorld` 测试桩继承默认行为

**线程安全**：与 `requestFullChunkSync` 相同，仅在服务端主线程调用安全。

**使用场景**：`EndGatewayEntity::_generateExitPortal` 通过 `world.getOrLoadChunk()` 扫描外岛区块判空，完整复刻 MC Java 的 `findExitPortalXZPosTentative` 行为。需要仅查询内存时（不触发加载）仍使用 `getChunk()`。

### 17. IWorld 粒子生成虚接口

`IWorld` 提供一组粒子生成虚方法，供 common 层游戏逻辑（方块动画、方块实体、实体行为等）编程式调用，由 `ServerWorld`（服务端广播）和 `ClientWorld`（客户端直接生成）分别实现：

| 虚方法 | 用途 | 服务端行为 | 客户端行为 |
|--------|------|-----------|-----------|
| `addParticle(type, pos, velocity)` | 普通粒子 | 广播 `ir::play::LevelParticles` | 直接生成 |
| `addParticle(type, pos, velocity, offset, count)` | 批量普通粒子 | 广播 | 直接生成 |
| `addBlockParticle(type, pos, velocity, blockState)` | 方块粒子（Block/Breaking/FallingDust） | 广播 `createBlock()`（携带 BlockState ID） | `ClientWorld` 直接调用 `DiggingParticle::createWithBlock()` |
| `addItemParticle(type, pos, velocity, itemStack)` | 物品粒子（Item/ItemSlime/ItemCobweb/ItemSnowball） | 广播 `createItem()`（携带 ItemStack 序列化字节流） | `ClientWorld` 直接调用 `ItemParticle::createWithItemStack()` |
| `addEntityEffectParticle(pos, velocity, offset, count, color)` | 实体效果粒子（EntityEffect） | 广播 `createEntityEffect()`（携带 ARGB 颜色） | `ClientWorld` 通过 `EntityEffectParticleData` 走数据管线 |
| `addTrailParticle(pos, target, color, duration)` | 轨迹粒子（Trail） | 广播 `createTrail()` | 客户端默认无操作 |

**默认实现**：所有粒子虚方法在 `IWorld` 中提供默认空实现（no-op），`ClientWorld` 和 `ServerWorld` 按需覆写。`WorldGenRegion` 等只读快照实现继承默认空行为。

**服务端广播链路**：`ServerWorld::addXxxParticle()` → `m_onBroadcastXxxParticle` 回调 → `MinecraftServer::broadcastXxxParticleInRange()` → 构造 `ir::play::LevelParticles` 广播给范围内玩家。回调在 `MinecraftServer::attachWorldBindings()` 中注册。详见 `src/server/application/README.md` 的「粒子广播链路」章节。
