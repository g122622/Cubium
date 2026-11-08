# world 模块

## 目录结构树

```text
world/
├── attribute/  # 环境属性分层求值
├── biome/  # 生物群系与气候采样
├── block/  # 方块模型、能力与具体行为
├── blockentity/  # 方块实体状态与行为
├── border/  # 该领域的子模块
├── chunk/  # 区块存储与加载票据
├── dimension/  # 维度类型与传送规则
├── effect/  # 客户端世界效果编号
├── entity/  # 该领域的子模块
├── explosion/  # 该领域的子模块
├── fluid/  # 流体状态与传播
├── gameevent/  # 世界模拟事件与振动
├── gamerule/  # 游戏规则
├── gen/  # 该领域的子模块
├── generation/  # 气候和生成共用的定义
├── lighting/  # 光照计算
├── map/  # 该领域的子模块
├── redstone/  # 该领域的子模块
├── spawn/  # 共享生成和重生规则
├── tick/  # 计划刻与随机刻基础设施
├── time/  # 时间规则与时间线
├── village/  # 该领域的子模块
├── weather/  # 该领域的子模块
├── GlobalPos.hpp  # 全局位置 - 方块位置 + 维度ID
├── IWorld.cpp  # 模块接口与实现
├── IWorld.hpp  # 世界访问接口
├── IWorldWriter.hpp  # 世界写入器接口
├── WorldConfig.hpp  # 世界类型枚举
└── WorldConstants.hpp  # 检查Y坐标是否在有效范围内
```

## 内部模块关系

各文件承接目录树中标明的单一职责，通过同领域接口组合。独占的辅助实现位于使用者目录下。

## 上下游外部依赖关系

公共层提供领域类型与接口；服务端通过应用装配连接玩家、网络和世界实现。源文件的构建归属与领域目录一致。

## 容易踩的坑

- 目录按领域职责组织；跨领域协作通过明确入口和基础类型。
- 新增或迁移源文件时同步更新所有消费目标的 CMake 清单。

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
- `PlayerInteractedWithEntityTrigger` 不通过 IWorld 回调触发，而是在 `ServerPlayHandler::_triggerPlayerInteractedWithEntity`（INTERACT/INTERACT_AT 返回 Success 时调用）中按 `AbstractCriterionTrigger::trigger()` 模板触发。
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

### 16. IWorld::chunkManager() 区块访问

**问题**：common 层代码需要按区块粒度访问数据（生物群系、按类型高度图、`inhabitedTime`、section 级缓存、区块加载判定、同步加载），但 common 不能依赖 server 层的 `ServerChunkManager`。

**解决**：`IWorld` 暴露 `chunkManager()` 访问器，返回 common 层窄接口 `world::chunk::IChunkManager`（声明于 `chunk/IChunkManager.hpp`），只含 4 个操作：

| 操作 | 语义 |
|------|------|
| `tryToGetChunkInMem(x, z)` | 仅查内存缓存，不触发加载/生成 |
| `hasChunkInMem(x, z)` | 区块是否已在内存中 |
| `requestFullChunkSync(x, z)` | 同步触发加载/生成，对应 MC Java `Level.getChunk(x, z, require=true)`；仅服务端主线程安全 |

`ServerChunkManager` 实现该接口；`ServerWorld::chunkManager()` 借 C++ 协变返回类型返回具体类型 `ServerChunkManager*`，因此服务端代码看到的是完整管理器 API，而 common 代码只看到这 4 个方法。`IWorld` 自身不再提供 `getChunk`/`hasChunk`/`getOrLoadChunk` 这类逐操作转发方法。

**默认实现**：`IWorld::chunkManager()` 默认返回 `EmptyChunkManager`（null object，语义为"所有区块均未加载"），供客户端只读适配器、测试桩世界等不持有区块存储的实现使用。测试侧另有 `mc::test::StubChunkManager`（固定返回一份 ChunkData）与 `mc::test::MapChunkManager`（把测试的 ChunkData 表当作缓存）。

**使用场景**：`EndGatewayEntity::_generateExitPortal` 经 `world.chunkManager()->requestFullChunkSync(...)` 同步加载外岛区块并逐段判空，完整复刻 MC Java 的 `TheEndGatewayBlockEntity.findExitPortalXZPosTentative` 行为。

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

### 18. 摧毁方块统一走 IWorld::destroyBlock / removeBlock

`IWorld` 提供两个与原版 `Level` 一一对应的方块移除入口，**任何"让方块消失"的代码都必须使用它们**，禁止再手写 `setBlockState(air)` + `spawnAfterBreak` 组合：

| 方法 | 语义 | 对应原版 |
|------|------|---------|
| `destroyBlock(pos, dropBlock[, entity])` | 2001 破坏粒子（火焰方块不发）→ 掉落表 → 写回流体遗留方块 → `BLOCK_DESTROY` 游戏事件 | `Level.destroyBlock` |
| `removeBlock(pos, isMoving)` | 只写回该位置的流体遗留方块（无水则空气），无粒子/掉落/事件 | `Level.removeBlock` |

要点：

- **掉落只在 `destroyBlock(..., true)` 时产生**。`spawnAfterBreak` 本身不产出物品（只做蠹虫生成等额外效果），旧代码把它当"掉落"会静默丢物品。
- 需要"破坏但不掉落"时用 `destroyBlock(pos, false)`（如投掷药水灭火、劫掠兽踩碎瓶草之外的选择）；连粒子/振动都不要时用 `removeBlock`（末影龙/凋灵穿墙、末影人取方块、爆炸移除）。
- `Block::updateOrDestroy` 在形状更新导致方块变空气时已改走 `destroyBlock`：依附方块（火把、红石线、梯子、悬挂告示牌、藤蔓）脱落会正常掉落并产生 2001 粒子与 `BLOCK_DESTROY` 振动；`UPDATE_SUPPRESS_DROPS` 置位时降级为不掉落。
- 玩家挖掘链路（`BlockInteractionManager`）保持独立：它需要工具附魔、经验、统计、`playerWillDestroy` 等上下文，**不要**改成调用 `destroyBlock`。
- 爆炸（`Explosion`）自行合并掉落，只使用 `removeBlock` 移除方块。
