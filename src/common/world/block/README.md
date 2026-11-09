# block 模块

## 目录结构树

```text
block/
├── base/  # 本领域共享的基础类型与定义
├── blocks/  # 按主要行为组织的具体方块
├── capability/  # 方块可选能力和跨行为族复用的实现
├── core/  # 方块身份、状态容器与行为扩展入口
├── dispense/  # 该领域的子模块
├── fire/  # 可燃参数查询
├── pattern/  # 3D 方块模式匹配（末地传送门、出口讲台、傀儡、凋灵）
├── properties/  # 方块注册属性构建与状态谓词
├── registry/  # 方块注册与状态索引
├── state/  # 不可变方块状态
├── support/  # 附着方块的支撑形状判定
└── tags/  # 数据包方块标签
```

## 内部模块关系

Block 定义共享行为入口，BlockState 维护不可变状态；BlockProperties 与方块族描述在注册时绑定。注册表持有方块和状态，原版装配位于 registry/vanilla。`pattern/` 提供基于方块状态谓词的三维结构匹配。

## 上下游外部依赖关系

实体、物品、区块和服务端世界消费方块接口；几何运算使用 common/physics，状态属性使用 common/util/property。

## 容易踩的坑

- 方块和状态由注册表持有，发放的引用必须保持稳定。
- 形状工厂显式区分 box 的归一化坐标和 fromPixelBox 的像素坐标。
- 声音事件标识、方块族规则、LevelEventId 与模拟 GameEvent 分别管理。
- 原版地图颜色策略在注册时绑定，公共模型不直接引用原版快照。

### 1. 状态不可变性

`BlockState::with()` 返回新状态，不修改原状态：

```cpp
// 错误：没有使用返回值
state.with(property, value); // 状态未改变！

// 正确：使用返回的新状态
const BlockState& newState = state.with(property, value);
```

### 2. 材质比较必须用地址比较

```cpp
// 正确：使用预定义材质引用
if (block.material() == Material::ROCK) {}

// 错误：创建新实例比较永远为 false
Material myRock = MaterialBuilder().solid().opaque().build();
if (block.material() == myRock) {} // 始终 false！
```

### 3. 空气方块 ID 固定为 0

`minecraft:air` 始终获得 ID 0，其他方块从 ID 1 开始。协议编码和存储时需注意。

### 4. BlockState 缓存不可变

`BlockState` 构造时缓存属性值，方块属性在构造后不可修改。

### 5. 状态ID vs 方块ID

- 方块ID：标识方块类型
- 状态ID：标识方块的特定状态（包含属性值）

```cpp
u32 blockId = block.blockId();  // 方块ID
u32 stateId = state.stateId();  // 状态ID
```

### 6. 光照透明度 vs 天空光传播

`opacity` 和 `propagatesSkylightDown` 是两个独立属性：

- 玻璃：`opacity = 0` 但 `propagatesSkylightDown = false`
- 树叶：`opacity = 0` 且 `propagatesSkylightDown = true`

### 7. LiquidBlock 等级映射

方块 level 与流体 level 映射：

- 方块 level = 0 → 流体 level = 8（源头）
- 方块 level = 1-7 → 流体 level = 1-7
- 方块 level = 8-15 → 流体 level = 8, falling = true

### 8. 挖掘工具类型同步

`HarvestTool` 命名空间中的常量必须与 `item::tool::ToolType` 枚举值保持同步。

### 9. 树苗和树木生成支撑方块一致性

`SaplingBlock` 和 `TreeFeature` 必须就根支撑方块达成一致，否则会出现"可以放置但不能生长"的不匹配。

### 10. getBlock() vs getBlockMutable()

`BlockState::getBlock()` 返回 `const Block&`，适用于只读访问。当需要调用非 const 方法（如 `tick`、`neighborChanged`、`onBlockRemoved`、`scheduleBlockTick` 等）时，使用 `getBlockMutable()` 获取 `Block&`：

```cpp
// 只读访问
const Block& block = state->getBlock();

// 需要非const引用时（如调用scheduleBlockTick、onBlockRemoved等）
Block& block = state->getBlockMutable();
```

### 11. 冰块融化与破坏路径分离

`IceBlock::randomTick()` 只负责融化，`onBlockRemoved()` 只负责破坏后的替换。不要让随机刻回调 `onBlockRemoved()`。

### 12. 作物骨粉增长随机数

骨粉增长必须从世界种子和方块位置派生随机数，不能使用全局 `rand()`。

### 13. 农田降雨补湿条件

`FarmlandBlock` 的降雨补湿要同时检查 `isRaining()` 和 `canRainAt(pos.up())`，否则测试会出现伪阳性。

### 14. 天气降水判定

`WeatherUtils::canRainAt()` / `canSnowAt()` 需要结合生物群系的 `hasPrecipitation()` 布尔值以及温度阈值一起判断。沙漠、蘑菇岛、恶地等无降水生物群系必须在注册数据里显式设置 `hasPrecipitation` 为 `false`（通过 `setHasPrecipitation(false)`）。

### 15. PaneBlock 连接形状

`PaneBlock` 连接形状按 4 位掩码缓存并使用规范化坐标，不要回退到单个中心形状占位符。

### 16. 重复注册静默返回已存在方块

重复注册同一资源位置的方块会返回已存在的方块，新属性被忽略。

### 17. AirBlock 碰撞特殊性

`AirBlock` 的 `isSolid()` 返回 `false`，碰撞检测时需同时检查 `isAir()`：

```cpp
if (!state.isAir() && state.isSolid()) {
    // 执行碰撞检测
}
```

### 18. RotatedPillarBlock 默认轴向

默认轴向是 `Axis::X`（枚举第一个值），但大多数原木默认应该是 `Axis::Y`。注册时需要设置默认状态。

### 19. 门方块双方块结构

`DoorBlock` 使用 `HALF` 属性区分上下半部分，操作时需同时处理两个方块位置。

### 20. 炼药锅无方块实体

`CauldronBlock`（空炼药锅）无水位属性；分层水位由 `LayeredCauldronBlock` 的 `LEVEL_1_3`（1-3）承载，交互操作直接修改方块状态，不需要方块实体。

### 21. 含水方块实现步骤

实现 `IWaterLoggable` 接口需要：

1. 添加 `WATERLOGGED` 属性到状态容器
2. 在 `getStateForPlacement()` 中检测水
3. 在 `updatePostPlacement()` 中调度流体 tick
4. 实现 `getFluidState()` 返回水流体状态

### 22. FireInfoRegistry 火焰参数系统

`Block::getFlammability()` 和 `Block::getFireSpreadSpeed()` 的默认实现已改为查询 `FireInfoRegistry`，无需子类重写即可获得正确的燃烧参数。

- `FireInfoRegistry::initializeVanillaFireInfos()` 在 `VanillaBlocks::initialize()` 末尾自动调用
- 仅注册 MC 原版 `FireBlock.bootStrap()` 中注册的可燃方块，未注册的方块火焰不会蔓延到其上
- 新增可燃方块时，在 `FireInfoRegistry::initializeVanillaFireInfos()` 中注册即可
- 子类仍可通过重写 `getFlammability()`/`getFireSpreadSpeed()` 提供自定义值，会覆盖注册表值
- 部分方块（如 SHELF）尚待对应方块指针注册后补充

**火焰蔓延 vs 岩浆点燃**：火焰蔓延（FireBlock）仅依赖本注册表，不检查 Material。岩浆点燃（LavaFluid）通过 `Material::isFlammable()` 判断，是独立系统。因此，告示牌、树苗等虽然未在本注册表中注册（与原版一致），但由于使用 WOOD / PLANT 材质，仍可被岩浆点燃。

### 23. canBeReplaced / canBeReplacedByFluid 语义

`BlockState` 提供两个可替换性查询方法：

- **`canBeReplaced()`**：对应 MC 的 `BlockState.canBeReplaced()` 无参版，缓存自 `Block.m_isReplaceable`。空气、水、岩浆、花草、火、雪层等返回 `true`；石头、泥土等实心方块返回 `false`。等价于 `isAir() || getMaterial().isReplaceable()` 但性能更优（缓存值）。
- **`canBeReplacedByFluid()`**：对应 MC 的 `BlockBehaviour.canBeReplaced(BlockState, Fluid)`，实现为 `canBeReplaced() || !isSolid()`。非固体但不可替换的方块（门、告示牌等）也返回 `true`，允许流体流入。

| 场景 | 使用哪个方法 |
|------|--------------|
| 世界生成（ReplaceablePredicate）、掉落方块判断 | `canBeReplaced()` |
| 方块放置替换 | `canBeReplaced()`（上下文感知版应使用 `Block::isReplaceable(state, context)`） |
| 流体流动 / 桶放置流体 | `canBeReplacedByFluid()` |

**注意**：不要再用 `isAir() || getMaterial().isReplaceable()` 手动判断可替换性，统一使用 `canBeReplaced()`。

#### canBeReplacedByFluid 与 FlowingFluid::isBlocked 的关系

`canBeReplacedByFluid()` 仅描述方块的结构属性（是否允许流体占据），不包含 MC Java 中的业务黑名单。`FlowingFluid::isBlocked()` 在 `canBeReplacedByFluid()` 之上叠加了额外的黑名单逻辑：

1. **ILiquidContainer 方块**（含水方块等）→ 委托给 `canContainFluid()` 判断
2. **路径黑名单**：`_door`、`_sign`、`ladder`、`sugar_cane`、`bubble_column` → 返回 `true`（阻挡流体），尽管这些方块的 `canBeReplacedByFluid()` 返回 `true`
3. **材质黑名单**：`Material::PORTAL`、`Material::STRUCTURE_VOID` → 返回 `true`（阻挡流体）
4. **默认**：`!canBeReplacedByFluid()` → 如果方块不可被流体替换，则阻挡

这是 MC Java 中 `canHoldAnyFluid()` 的精确对应：黑名单方块虽然 `canBeReplaced(Fluid)` 返回 `true`，但 MC Java 明确排除了它们。

#### 液体方块的 canBeReplacedByFluid

液体方块（水、岩浆）的 `canBeReplacedByFluid()` 返回 `true`（因为 `canBeReplaced() = true`）。这与 MC Java 一致：`BucketItem.emptyContents()` 允许在已有液体上放置桶装流体。旧代码中 `canBeReplaced() && !isLiquid()` 的 `!isLiquid()` 检查是错误的——MC Java 不检查目标方块是否已是液体。放置在已有液体上的行为是 `setBlock` 替换同类型液体（无操作）或水 / 岩浆交互。

### 24. GameMasterBlock 权限检查

管理员方块（CommandBlock、StructureBlock、JigsawBlock）有三层权限防护：

1. **放置限制**：使用 `GameMasterBlockItem`（继承 BlockItem），重写 `getStateForPlacement()` 检查 `player.canUseGameMasterBlocks()`（创造模式 + OP≥2）
2. **破坏限制**：`BlockInteractionManager::_canBreakBlock()` 检查 `block.isGameMaster()`，无权限阻止破坏
3. **交互限制**：各方块 `useItemOn` / `useWithoutItem` 中检查 `player.canUseGameMasterBlocks()`

判断是否为管理员方块应使用 `block.isGameMaster()` 虚方法，而非 `dynamic_cast<GameMasterBlock>`（性能更好）。新增管理员方块时需：

1. 继承 `GameMasterBlock` 标记接口
2. 重写 `isGameMaster()` 返回 `true`
3. 在 `BlockItemRegistry` 中使用 `GameMasterBlockItem` 注册
4. 在 `useItemOn` / `useWithoutItem` 中添加权限检查

### 25. animateTick 系统与 IBlockAnimateContext

`Block::animateTick()` 是客户端方块动画 tick 方法，每 tick 由 `ClientWorld::animateTick()` 调度，用于生成粒子效果、播放环境音效等视觉效果。

- **仅客户端执行**：animateTick 在服务端永远不会被调用
- **IBlockAnimateContext 接口**：为避免 Block 直接依赖完整的 IWorld，animateTick 接收 `IBlockAnimateContext&` 而非 `IWorld&`，仅提供 `addAnimateParticle()`、`playLocalSound()`、`getBlockState()` 三个轻量方法
- **签名**：`virtual void animateTick(IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const;`
- **基类默认实现**：空操作（无粒子、无音效）
- **调度逻辑**：ClientWorld 每帧执行 667 次迭代 × 2 范围 pass（16 + 32），共 1334 次随机采样
- **已实现的方块**：BubbleColumnBlock（气泡 / 漩涡粒子 + 环境音）、SporeBlossomBlock（孢子花粒子）

### 26. MUSHROOM_GROW_BLOCK 标签与蘑菇放置

`BlockTags::MUSHROOM_GROW_BLOCK()` 标签包含菌丝（mycelium）、灰化土（podzol）、绯红菌岩（crimson_nylium）、诡异菌岩（warped_nylium），用于蘑菇放置判定。

蘑菇（`MushroomBlock`）的放置判定分两层：

1. **`Block::canSustainPlant()`**（`PlantType::Cave` 分支）：检查下方方块是否属于 `MUSHROOM_GROW_BLOCK` 标签，只有标签内的方块才返回 true
2. **`MushroomBlock::isValidPosition()`**：在标签方块上无条件允许放置，在其他固体方块上需光照 < 13

**注意**：不要在 `canSustainPlant` 的 `PlantType::Cave` 分支中添加光照检查——光照检查由 `MushroomBlock::isValidPosition()` 独立完成。`canSustainPlant` 只负责判断土壤类型兼容性。

蘑菇注册使用 `blocks::MushroomBlock` 而非 `SimpleBlock`，巨型蘑菇方块使用 `blocks::HugeMushroomBlock`（具有 6 方向布尔属性）。

### 27. Block::pushEntitiesUp 实体推出

`Block::pushEntitiesUp(oldState, newState, world, pos)` 是一个静态工具方法，当方块碰撞形状增大时将嵌入方块内的实体向上推出。对应 MC Java 的 `Block.pushEntitiesUp()`。

**工作原理**：

1. 计算 `oldState` 与 `newState` 的碰撞形状差集（`BooleanOps::OnlySecond`，即"在 newState 中但不在 oldState 中"的部分）并平移到世界坐标
2. 若差集形状为空，直接返回 `newState`
3. 取差集形状的世界 AABB，查找其中的所有实体
4. 对每个实体：将其碰撞箱上移 1 格，沿 Y 轴向下（`movement = -1.0`）与差集形状做碰撞，得到最大可下落距离 `d0`（非正数）
5. 让实体相对上移 `1 + d0`，即正好停在差集形状顶部之上

**使用场景**：

- **雪层增加**：`tickPrecipitation()` 中雪层层数增加时调用 `pushEntitiesUp` 推出站在雪上的实体
- **耕地变泥土**：`FarmlandBlock::turnToDirt()` 中耕地（15/16格高）变为泥土（1格高）时推出实体
- **其他碰撞形状增大的场景**

**签名**：`static const BlockState& pushEntitiesUp(const BlockState& oldState, const BlockState& newState, IWorld& world, const BlockPos& pos)`

**注意**：

- 方法返回 `newState`，方便链式调用
- 实现与 MC Java 1.21.11 一致，使用 `Shapes::joinUnoptimized` + `BooleanOps::OnlySecond` 计算形状差集，再用 `VoxelShape::collide(Axis::Y, ..., -1.0)` 计算推出距离
- 实体移动通过 `Entity::move(MoverType::Piston, delta)` 完成，该重载不做碰撞检测
- 必须在 `setBlockState` **之前**调用，先推出实体再更新方块状态

### 28. Block::handlePrecipitation 降水方块处理

`Block::handlePrecipitation(IWorld&, const BlockPos&, BiomeClimate::Precipitation)` 是方块的降水处理虚方法，默认实现为空操作。方块可以重写此方法来响应降水：

- **CauldronBlock**：雨天 5% 概率增加水位、雪天 10% 概率增加水位，水位上限为 3
- **LightningRodBlock**：雷暴天气且避雷针朝上时，通过 `onLightningStrike()` 激活避雷针

**调用时机**：`ServerWorld::tickPrecipitation()` 在每个降水 tick 中，对表面方块调用 `biome.getPrecipitationAt()` 确定降水类型后，调用 `block.handlePrecipitation(world, pos, precipitation)`。

**注意**：

- 此方法替代了旧的 `fillWithRain()` 方法，增加了降水类型参数（Rain / Snow / None）
- 降水类型由 `Biome::getPrecipitationAt()` 确定，综合考虑生物群系降水设置和高度调整后的温度
- 只有 `isRaining()` 为 true 时才会调用 `handlePrecipitation`（在 `tickPrecipitation` 中判断）

### 29. Block::onFallenUpon 摔落伤害系统

`Block::onFallenUpon(IWorld&, const BlockPos&, const BlockState&, Entity&, f32 fallDistance)` 是方块响应实体摔落的虚方法。对应 MC Java 的 `Block.fallOn()`。

**默认实现**：调用 `entity.causeFallDamage(fallDistance, 1.0f, DamageSources::fall())` 施加普通摔落伤害。

**调用链**：

```
Entity::move() → updateFallDistance() → _handleLandingOnBlock() → Block::onFallenUpon()
```

**重要**：`Entity::updateFallDistance()` 不再直接调用 `handleFallDamage()`，摔落伤害完全由 `Block::onFallenUpon` 负责。方块子类通过重写此方法自定义摔落行为：

| 方块 | onFallenUpon 行为 | 摔落伤害 |
|------|-------------------|----------|
| Block（基类） | 调用 `causeFallDamage(dist, 1.0, fall())` | 普通摔落伤害 |
| PointedDripstoneBlock（石笋尖端） | 调用 `causeFallDamage(dist + 2.5, 2.0, stalagmite())`，不调用父类 | 增大石笋伤害，替代普通摔落 |
| FarmlandBlock | 先执行踩踏逻辑，再调用 `Block::onFallenUpon` | 保留普通摔落伤害 |
| TurtleEggBlock | 先执行踩破逻辑，再调用 `Block::onFallenUpon` | 保留普通摔落伤害 |

**与 onLanded 的区别**：

- `onLanded`：实体着地时修改运动向量（蜂蜜块取消摔落距离、史莱姆块弹跳），在 `updateFallDistance` 之前调用
- `onFallenUpon`：实体着地后施加摔落伤害，由 `updateFallDistance` 内部调用

**乘客摔落伤害传播**：`Entity::causeFallDamage` 会先将摔落伤害传播给所有乘客（`propagateFallToPassengers`），因此当载具（如马、船、矿车）受到摔落伤害时，乘客也会受到相同的摔落伤害。

### 30. Block::playerWillDestroy 玩家即将破坏方块回调

`Block::playerWillDestroy(IWorld&, const BlockPos&, const BlockState&, Player&)` 是玩家即将破坏方块时调用的虚方法。对应 MC Java 的 `Block.playerWillDestroy()`。

**默认实现**：空操作。需要特殊行为的方块应重写此方法。

**与 onBlockRemoved 的区别**：

- `playerWillDestroy`：在方块被移除**之前**调用，接收玩家信息，可区分创造 / 生存模式
- `onBlockRemoved`：在方块状态变更**之后**调用，不包含玩家上下文，由 `ServerWorld::setBlockState` 触发

**调用时机**：`BlockInteractionManager::handleBlockBreak` 和 `StopDestroyBlock` 中，在生成掉落物和设置方块为空气之前调用。

**已实现方块**：

- `PistonHeadBlock`：创造模式下破坏活塞头时，级联销毁匹配的活塞基座且不产生掉落物；生存模式不执行操作（级联销毁和掉落物由 `onBlockRemoved` 处理）

**创造模式掉落物抑制**：`BlockInteractionManager` 在 `playerWillDestroy` 之后检查 `player.isCreative()`，创造模式下跳过 `_generateBlockDrops`。

**注意**：新增方块如需在破坏时区分创造 / 生存模式行为，应重写 `playerWillDestroy` 而非在 `onBlockRemoved` 中判断。

### 31. Block::updateFromNeighbourShapes 邻居形状更新

`Block::updateFromNeighbourShapes(state, world, pos)` 是静态方法，按照 `UPDATE_SHAPE_ORDER`（WEST→EAST→NORTH→SOUTH→DOWN→UP 轴对顺序）遍历 6 个方向，对每个方向调用 `updatePostPlacement` 累积更新方块状态。对应 MC Java 的 `Block.updateFromNeighbourShapes()`。

**调用场景**：

- `ServerChunkManager::_postProcessChunk`：区块后处理生成，非液体方块形状更新（flags = 276）
- `PistonBlockEntity::clearPistonBlockEntity`：活塞完成移动后更新被移动方块形状
- `EndermanPlaceBlockGoal::tick`：末影人放置方块后更新形状
- `Template::placeInWorld`：结构模板放置后批量更新所有方块形状（flags = 276）

**flags = 276 含义**：`SKIP_BLOCK_ENTITY_SIDEEFFECTS | KNOWN_SHAPE | INVISIBLE`（256 | 16 | 4），区块后处理和结构放置不需要通知邻居和客户端。

**方向迭代顺序**：轴对排列（WEST→EAST, NORTH→SOUTH, DOWN→UP），同轴方向连续处理确保方块形状在轴向上一致收敛。这个顺序与 `ServerWorld::setBlockState` 中的邻居通知顺序不同，后者使用 `NEIGHBOR_DELTAS`（WEST→EAST→DOWN→UP→NORTH→SOUTH）。

**注意**：`getBlockState` 返回 `nullptr` 时（区块未加载）安全跳过该方向；`updatePostPlacement` 是非 const 虚方法，内部使用 `const_cast` 调用。

### 32. Block::dropResources 非玩家掉落生成

`Block::dropResources(IWorld&, const BlockPos&, const BlockState&)` 是静态方法，用于方块被非玩家方式破坏时（如海绵吸水、爆炸等）在世界中生成掉落物品。对应 MC Java 的 `Block.dropResources(BlockState, LevelAccessor, BlockPos, BlockEntity)`。

**与 BlockDropHandler 的区别**：

- `BlockDropHandler`：服务端专用，处理玩家破坏方块的掉落，携带工具和玩家上下文，支持时运 / 精准采集加成
- `Block::dropResources`：通用侧（common），不携带工具和玩家上下文，仅使用方块的掉落表生成物品

**调用场景**：

- `SpongeBlock::absorb`：海绵吸收海带、海草等海洋植物时生成掉落物
- `IWorld::destroyBlock` 的内部实现（非玩家破坏的统一入口）

**注意**：常规"摧毁方块"不要直接调用它，统一走 `IWorld::destroyBlock(pos, true, entity)`——后者额外处理 2001 破坏粒子、写回流体遗留方块与 `BLOCK_DESTROY` 游戏事件。

**实现细节**：

1. 检查 `world.lootTableManager()` 是否为空（客户端返回 nullptr，直接退出）
2. 获取方块的掉落表，如果为空则退出
3. 构建 `LootContext`（使用世界共享随机 `world.getRandom()`）
4. 设置必需参数 `BLOCK_STATE` 和 `BLOCK_POS`（通过 `const_cast` 转为非 const 指针，与 `BlockDropHandler::buildLootContext` 一致）
5. 设置掉落表解析器和条件解析器
6. 调用 `LootTable::generate()` 生成物品列表
7. 通过 `ItemDropHelper::spawnItemEntities()` 在世界中生成掉落物实体
8. 调用 `Block::spawnAfterBreak()` 触发额外效果

**客户端安全**：`lootTableManager()` 在客户端返回 `nullptr`，方法直接返回，不生成任何掉落物。

### 33. Block::isExceptionForConnection 连接例外方块判断

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

### 34. BlockTag 数据驱动（BlockTagLoader）

方块标签**内容 100% 来自数据包**：`RegistryBootstrap` 在方块与方块物品注册完成后执行 `BlockTags::clear()` → `BlockTagLoader::loadFromDataPackRepository()`，从 `data/<ns>/tags/block/` 整体重建标签表。`BlockTags::initialize()` 只切换状态，不留任何编译期快照（数据包含 candle、copper_chain 等 Cubium 尚未实现的成员，快照会与数据包无声漂移）。加载由 `common/resource/tag/GenericTagLoader` 完成，成员为方块资源位置，经 `BlockRegistry::getBlock` 校验存在性（未注册的成员跳过并告警）。`#minecraft:xxx` 引用由加载器按依赖顺序展开。

**依赖顺序**：`BlockTagLoader` 必须在 `VanillaBlocks::initialize()`（即所有方块注册）之后运行。

**便捷访问器**（`BlockTags::LOGS()` 等）缓存 `BlockTag*`，`clear()` 后旧指针失效；每个访问器都用 `_isLive(tag)` 复核，新增访问器必须照抄。

**陷阱**：数据包中大量方块（coral、quartz_wall、light、test_block 等）在 Cubium 尚未实现，加载时会打印 `GenericTagLoader: unknown member ... (required), skipped` 告警并被跳过——这是预期行为（该方块本就无法放置），不是缺陷。判断"某标签是否被正确加载"应核对已实现方块，而非以告警数量为准。

### 35. BlockTags 各标签说明

#### BARS 标签

`BlockTags::BARS()` 包含铁栏杆（`iron_bars`）和铜栏杆变体，用于：

- WallBlock 的 `_getWallHeight()` 判断——铁栏杆与墙连接时返回 `WallHeight::Low`（低连接）
- PaneBlock / WeatheringCopperBarsBlock / WaxedCopperBarsBlock 的 `shouldConnectTo()` 连接判定
- 上述三个类的 `skipRendering()` 面剔除——BARS 标签方块之间水平双向连接时跳过内侧面渲染

#### SHULKER_BOXES 标签

`BlockTags::SHULKER_BOXES()` 包含所有潜影盒变体（无色 + 16色），用于 `Block::isExceptionForConnection()` 判断——潜影盒虽然是固体，但不应与栅栏、墙、玻璃板建立连接。

#### WALL_POST_OVERRIDE 标签

`BlockTags::WALL_POST_OVERRIDE()` 包含放置在墙上时强制显示墙柱的方块：火把、灵魂火把、红石火把、绊线、告示牌（站立/墙面所有变体）、旗帜（站立/墙面所有颜色）、压力板（木质/石质/铜质/金质/铁质所有变体）。用于 WallBlock 的 `_shouldRaisePost()` 判断——当直线 Tall 墙上方有 WALL_POST_OVERRIDE 标签方块时，强制升起墙柱（UP=true）。

#### DOES_NOT_BLOCK_HOPPERS 标签

`BlockTags::DOES_NOT_BLOCK_HOPPERS()` 包含蜂巢(bee_nest)和蜂箱(beehive)，即与 BEEHIVES 标签相同的方块。用于 `HopperEntity::pullItems()` 中的漏斗吸取判断——即使上方方块碰撞形状为完整方块（`isFaceFull(Direction::Down)` 为 true），若该方块在此标签中，漏斗仍可吸取上方物品实体。这允许漏斗与蜂巢/蜂箱交互（吸取蜂蜜瓶/空瓶）。

#### CHAINS 标签

`BlockTags::CHAINS()` 包含铁锁链（iron_chain）和所有铜锁链变体（copper_chain、exposed_copper_chain、weathered_copper_chain、oxidized_copper_chain 及其涂蜡变种），共 9 种方块。对应 MC 原版标签 `minecraft:chains`。用于锁链方块的分组判断（如攀爬检测、连接形状等）。

**注意**：MC 1.21+ 将原 `minecraft:chain` 方块重命名为 `minecraft:iron_chain`，与铜锁链命名风格统一。

#### WOODEN_DOORS 标签

`BlockTags::WOODEN_DOORS()` 包含所有 12 种木门方块（橡木、云杉、白桦、丛林、金合欢、深色橡木、红树木、樱花木、竹木、苍白橡木、绯红木、诡异木）。对应 MC 原版标签 `minecraft:wooden_doors`。用于区分木门和其他材质门（铁门、铜门），影响红石信号判定、僵尸破门等游戏逻辑。

#### DOORS 标签

`BlockTags::DOORS()` 包含所有门方块：12 种木门 + 铁门 + 8 种铜门（含氧化和涂蜡变种），共 21 种方块。对应 MC 原版标签 `minecraft:doors`。用于通用的门方块判断逻辑。

#### WOODEN_TRAPDOORS 标签

`BlockTags::WOODEN_TRAPDOORS()` 包含所有 12 种木活板门方块（橡木、云杉、白桦、丛林、金合欢、深色橡木、红树木、樱花木、竹木、苍白橡木、绯红木、诡异木）。对应 MC 原版标签 `minecraft:wooden_trapdoors`。用于区分木活板门和其他材质活板门（铁活板门、铜活板门），影响红石信号判定等游戏逻辑。

#### TRAPDOORS 标签

`BlockTags::TRAPDOORS()` 包含所有活板门方块：12 种木活板门 + 铁活板门 + 8 种铜活板门（含氧化和涂蜡变种），共 21 种方块。对应 MC 原版标签 `minecraft:trapdoors`。用于通用的活板门方块判断逻辑。

#### NON_FLAMMABLE_WOOD 标签

`BlockTags::NON_FLAMMABLE_WOOD()` 包含所有不可燃烧的木材方块（绯红木和诡异木系列），包括原木/菌柄、去皮原木/去皮菌柄、菌丝体/去皮菌丝体、木板、台阶、楼梯、栅栏、栅栏门、门、活板门、按钮、压力板、告示牌、墙面告示牌、悬挂告示牌、墙面悬挂告示牌、书架等，共 34 种方块。对应 MC 原版标签 `minecraft:non_flammable_wood`。用于火灾蔓延判定——标签内的方块不会被火焰点燃或烧毁。

#### BEE_ATTRACTIVE 标签

`BlockTags::BEE_ATTRACTIVE()` 包含 29 种吸引蜜蜂的花朵方块：蒲公英、开放眼眸花、虞美人、蓝花美耳草、绒球葱、蓝色滨菊、郁金香（红/橙/白/粉）、滨菊、矢车菊、铃兰、凋零玫瑰、火把花、向日葵、丁香、牡丹、玫瑰丛、瓶子草、开花杜鹃树叶、开花杜鹃、红树胎生苗、樱花树叶、粉瓣花、野花、紫颂花、孢子花、仙人掌花。对应 MC 原版标签 `minecraft:bee_attractive`。

**关键点**：

- 闭合眼眸花**不在**此标签中（与 MC 1.21.11 数据包一致），因此蜜蜂不被闭合眼眸花吸引
- 含水（`waterlogged=true`）的可水合花朵由 `BeeEntity::attractsBees()` 工具函数排除
- 向日葵仅上半部分（`DoubleBlockHalf::Upper`）吸引蜜蜂，由 `BeeEntity::attractsBees()` 处理
- 该标签被 `EyeblossomBlock::onEntityCollision` 通过 `BeeEntity::attractsBees(state)` 间接使用，决定蜜蜂接触眼眸花时是否中毒

### 36. getEntityInsideCollisionShape 实体内部碰撞形状

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

### 37. IBucketPickupHandler 双路径拾取（pickupFluid / pickupItem）

`IBucketPickupHandler` 接口支持两种拾取路径：

1. **流体拾取（pickupFluid）**：流体方块（水、岩浆）重写此方法返回对应的 `Fluid*`，空桶拾取后获得满桶物品（水桶/岩浆桶）。`BucketItem::onItemUse` 的空桶路径优先尝试此方法。
2. **非流体拾取（pickupItem）**：非流体方块（如细雪 PowderSnowBlock）重写此方法返回对应的 `const Item*`（如 `Items::POWDER_SNOW_BUCKET`），空桶拾取后获得细雪桶。当 `pickupFluid()` 返回 nullptr 时，`BucketItem::onItemUse` 会继续尝试 `pickupItem()`。默认实现返回 nullptr。
3. **拾取音效（getPickupSound）**：非流体方块可重写此方法返回拾取时播放的音效（如细雪返回 `SoundEvents::ITEM_BUCKET_FILL_POWDER_SNOW`）。流体方块使用 `BucketItem` 中硬编码的 `ITEM_BUCKET_FILL` 音效，无需重写此方法。默认实现返回 nullptr。

**实现规则**：

- 流体方块只需实现 `pickupFluid()`，`pickupItem()` 和 `getPickupSound()` 保持默认即可
- 非流体方块只需实现 `pickupItem()` 和 `getPickupSound()`，`pickupFluid()` 返回 nullptr
- `BucketItem::onItemUse` 的空桶路径先尝试 `pickupFluid()`，失败后再尝试 `pickupItem()`
- `EmptyBucketDispenseBehavior` 同样遵循此双路径逻辑

### 38. getShadeBrightness 遮光亮度与 AO 阴影

`Block::getShadeBrightness(const BlockState&, IWorld*, const BlockPos*)` 是虚方法，返回方块的遮光亮度值（0.0~1.0），决定方块在环境光遮蔽（Ambient Occlusion）渲染中的阴影程度。对应 MC Java 的 `BlockBehaviour.getShadeBrightness()`。

**默认行为**：

```cpp
// Block::getShadeBrightness 默认实现
return state.hasOpaqueCollisionShape() ? 0.2f : 1.0f;
```

- `0.2f`（有AO阴影）：碰撞形状完整且不透明的方块（如石头、泥土）
- `1.0f`（无AO阴影）：碰撞形状不完整或透明的方块（如玻璃、楼梯、台阶）

**便捷方法**：`BlockState::getShadeBrightness()` 委托到 `Block::getShadeBrightness()`，`BlockState::getAmbientOcclusionLightValue()` 进一步委托到 `getShadeBrightness()`。

**与 AmbientOcclusionCalculator 的集成**：渲染管线中 `AmbientOcclusionCalculator` 调用 `state->getAmbientOcclusionLightValue()` 获取每个方块的遮光亮度，用于计算 AO 顶点颜色。遮光亮度越低（接近0.2），方块边缘阴影越深；遮光亮度为1.0时，方块不产生AO阴影。

**已重写的方块**：

| 方块 | 返回值 | 原因 |
|------|--------|------|
| MudBlock | 0.2f | 碰撞形状不完整（14/16格高），但视觉上应产生AO阴影 |
| SnowBlock | 8层→0.2f, 其他→1.0f | 满层(8层)视觉等价于完整方块产生阴影，低层不产生阴影 |
| BarrierBlock | 1.0f | 不可见方块，不应影响AO计算 |

**添加新重写的注意事项**：

- 如果方块碰撞形状不完整但视觉上应产生阴影（如 MudBlock），需要显式重写返回 0.2f
- 如果方块有完整碰撞形状但不应产生阴影（如 BarrierBlock），需要显式重写返回 1.0f
- 默认行为基于 `hasOpaqueCollisionShape()`（即 `isOpaque && material().blocksMovement()`），大部分方块无需重写

### 39. ColoredBlocks 床方块注册

16 色床方块在 `ColoredBlocks.hpp/.cpp` 中注册，使用 `blocks::BedBlock` 类并传入 `DyeColor` 参数。每种颜色对应一个独立的 `BedBlock` 实例（white_bed、orange_bed、...、black_bed）。

**注册方式**：`registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:xxx_bed"), DyeColor::Xxx, bedProps)`

**方块属性**：`Material::WOOL`，硬度 0.2，`notSolid()`（不阻挡光线），`ignitedByLava()`（可被岩浆点燃）

**状态属性**：`HORIZONTAL_FACING`（朝向）、`BED_PART`（HEAD/FOOT 部分）、`OCCUPIED`（占用状态）

**物品注册**：床物品使用自定义 `BedItem` 子类（非 `registerSimpleBlock`），在 `Items::_registerBeds()` 中通过 `registerItem<BedItem>()` 注册，最大堆叠数为 1。`BedItem` 重写 `getStateForPlacement()` 检查头部位置可替换性，`BedBlock::onBlockPlacedBy()` 在脚部放置后自动放置头部方块。

**注意**：床的双方块结构（HEAD/FOOT）要求放置和破坏时同时处理两个方块位置，详见 `functional/README.md` 中的 BedBlock 文档。

### 40. SupportType 支撑类型与 canSupportCenter / canSupportRigidBlock

`SupportType` 对应 MC 1.21.11 `net.minecraft.world.level.block.SupportType`，用于判断方块面是否足够坚固以支撑其他方块（火把、灯笼、钟、压力板、铁轨等）。三种支撑类型：

| 类型 | 判定 | 典型用途 |
|------|------|----------|
| `Full` | 方块面投影必须覆盖整个 1×1 面 | 墙面附着（WallTorchBlock、BellBlock 墙面路径） |
| `Center` | 方块面投影必须包含中心 2×2 像素到 10×10 像素的"中心柱" | 悬挂方块（BellBlock 天花板、LanternBlock、TorchBlock、CandleBlock、SporeBlossomBlock） |
| `Rigid` | 方块面投影必须覆盖 1×1 面除中心 12×12 像素柱以外的外环 | 铁轨、压力板 |

**判定基于方块的 BlockSupportShape（支撑形状）**，默认等于碰撞形状。某些方块（如泥巴、灵魂沙）的碰撞形状比完整方块矮，但支撑形状是完整方块。

**关键 API**：

- `BlockState::isFaceSturdy(world, pos, direction, supportType)`：委托到 `supportType.isSupporting(*this, world, pos, direction)`
- `Block::canSupportCenter(world, pos, direction)`：CEILING 路径专用，对 `UNSTABLE_BOTTOM_CENTER` 标签方块（栅栏门）在 `Direction::Down` 方向返回 false
- `Block::canSupportRigidBlock(world, pos)`：等价于 `state.isFaceSturdy(world, pos, Direction::Up, SupportType::Rigid)`，用于铁轨、压力板

**与 hasEnoughSolidSide 的区别**：

- `hasEnoughSolidSide`：Cubium 遗留方法，检查 `isSolidSide && doesSideFillSquare(collisionShape)`，无 MC 1.21.11 对应（MC 中 `hasEnoughSolidSide` 已移除）
- `isFaceSturdy(Full)`：基于 `blockSupportShape` 判定，与 MC 1.21.11 一致

**迁移指南**：悬挂类方块（BellBlock CEILING、LanternBlock、TorchBlock、CandleBlock、SporeBlossomBlock）应使用 `canSupportCenter`；墙面附着类（WallTorchBlock、BellBlock 墙面路径）应使用 `isFaceSturdy(Full)`；铁轨、压力板使用 `canSupportRigidBlock`（压力板还需 `|| canSupportCenter`）。

**注意**：`UNSTABLE_BOTTOM_CENTER` 标签数据包内容为 `#minecraft:fence_gates`（12 种栅栏门）。`BlockTags::initialize()` 中直接内联栅栏门列表；数据包加载器（BlockTagLoader）具备标签引用解析能力，可正确展开 `#fence_gates`。

### 41. Block 右键交互双入口与 BlockActionResult 语义

`Block` 的右键交互拆成两个虚方法，对齐 vanilla `BlockBehaviour.useItemOn` / `useWithoutItem`：

```cpp
virtual BlockActionResult useItemOn(ItemStack& stack, const BlockState& state, IWorld& world,
    const BlockPos& pos, Player& player, Hand hand, const BlockRaycastResult& hit);
virtual BlockActionResult useWithoutItem(const BlockState& state, IWorld& world,
    const BlockPos& pos, Player& player, const BlockRaycastResult& hit);
```

- `useItemOn`：手持物参与交互时派发（**空手也会派发**，对齐 vanilla——如空手熄灭蜡烛）。默认实现返回 `TryEmptyHandInteraction`。
- `useWithoutItem`：空手语义，或 `useItemOn` 返回 `TryEmptyHandInteraction` 后的回退。默认实现返回 `Pass`。

`stack` 是**玩家权威手持槽引用**（主手=选中槽、副手=OFFHAND 槽），方块对其 `shrink`/赋值的修改直接落盘，服务端随后统一同步；创造模式的"无消耗"须由方块自行用 `player.hasInfiniteMaterials()` 表达。

**返回值语义**（`BlockActionResult`，封装 `ActionResultType + std::optional<ItemStack>`）：

| `ActionResultType` | vanilla 等价 | consumesAction | wasItemInteraction | 服务端挥臂 |
|---|---|---|---|---|
| `Success` | `SUCCESS` | 是 | 是 | 否（客户端挥） |
| `SuccessServer` | `SUCCESS_SERVER` | 是 | 是 | 是 |
| `SuccessServerWithoutItem` | `SUCCESS_SERVER.withoutItem()` | 是 | 否 | 是 |
| `Consume` | `CONSUME` | 是 | 是 | 否 |
| `TryEmptyHandInteraction` | `TRY_WITH_EMPTY_HAND` | 否 | 否 | 否 |
| `Fail` / `Pass` | `FAIL` / `PASS` | 否 | 否 | 否 |

- 返回 `BlockActionResult::success(stack)` / `success().heldItemTransformedTo(stack)` 表示方块改动了手持物，携带转换后的物品堆（如 `ShelfBlock` 交换结果、花盆取物）；无该字段时服务端按原地修改后的权威槽同步。
- `getMenuProvider(state, world, pos)` 是新增的虚方法（等价 vanilla `MenuProvider`）：有界面的方块返回自己的 `ContainerType`，服务端在"旁观者右键"路径据此统一打开界面。

**TODO（本条目遗留，后续补全）**：本次只完成了派发骨架与大部分方块的入口拆分，以下偏差尚未修完：

1. **部分方块的 `useItemOn` / `useWithoutItem` 语义混用未拆分**：多个方块把"手持物分支 + 空手分支"写在同一函数内（如 `FlowerPotBlock::useItemOn` 内含空手取物分支、`CakeBlock::useWithoutItem` 内含进食判断），未按 vanilla 严格拆到两个入口。行为大体等价，但与 vanilla 的 `useItemOn`→`TRY_WITH_EMPTY_HAND`→`useWithoutItem` 链路不完全一致，需逐块核对 vanilla 源后拆分。
2. **`SpawnerBlock` 的刷怪蛋交互归属错误**：vanilla 1.21.11 中刷怪笼的刷怪蛋交互由 `SpawnEggItem::useOn`（物品侧）负责，`SpawnerBlock` 本身无 `useItemOn`。项目把它实现在 `SpawnerBlock::useItemOn`（方块侧），须迁到 `SpawnEggItem`（`SpawnEggItem.cpp` 已有刷怪笼分支，方块侧属重复实现，应删除）。
3. **`AbstractFurnaceBlock::getMenuProvider` 返回 `ContainerType::Furnace`**：高炉 / 烟熏炉应分别返回 `BlastFurnace` / `Smoker`，当前共用基类的 `Furnace`，旁观者右键会开出错误界面。
4. **`TNTBlock` 缺少 `TNT_EXPLODES` gamerule 门控**：vanilla 在 gamerule 关闭时返 `Pass` 并提示 `block.minecraft.tnt.disabled`，项目未实现该分支。
5. **`ChiseledBookShelfBlock` / `VaultBlock` / `CeilingHangingSignBlock` 等 vanilla 有 `useItemOn` 的方块项目尚未实现**，待实现时须按 vanilla 补双入口。

**C++ 引用语义注意**：MC Java 中 `useItemOn(ItemStack p_433583_, ...)` 的 `p_433583_` 是引用副本，`inventory.setItem()` 替换数组槽位不会修改 `p_433583_`。C++ 中权威槽引用（`PlayerInventory::getSelectedStackRef()`）在 `setItem` 后即反映新值。因此若需在替换后判断"原手持物品是否为空"，必须在调用前捕获 `const bool heldItemWasEmpty = heldItem.isEmpty();`（参考 `ShelfBlock::useItemOn` 实现）。

### 42. BlockState 粒子与渲染类型查询方法

`BlockState` 提供两个用于刷子粒子生成等场景的查询方法：

- **`shouldSpawnTerrainParticles()`**：对应 MC 1.21.11 `BlockBehaviour.BlockStateBase#shouldSpawnTerrainParticles`，默认实现 `!isAir()`。刷子（`BrushItem::onUseTick`）等需要根据视线方块生成方块碎屑粒子的场景使用此方法判断是否生成粒子。
- **`isInvisibleRenderType()`**：委托到 `Block::getRenderType(*this) == Block::RenderType::INVISIBLE`。用于刷子粒子生成时排除不可见方块（如空气、屏障），避免在不可见方块表面生成无意义的碎屑粒子。

**设计说明**：由于 `Block.hpp` 包含 `BlockState.hpp`（反向依赖），`BlockState.hpp` 中无法直接内联 `Block::RenderType` 枚举。因此 `isInvisibleRenderType()` 在 `BlockState.hpp` 中仅声明，实现位于 `BlockState.cpp`（可包含 `Block.hpp`）。`shouldSpawnTerrainParticles()` 为纯内联方法（仅依赖 `isAir()`），直接在头文件中实现。

**调用方**：`BrushItem::onUseTick` 中判断 `blockState->shouldSpawnTerrainParticles() && !blockState->isInvisibleRenderType()` 才调用 `spawnDustParticles`，与 MC 1.21.11 `BrushItem.onUseTick` 中的 `blockstate.shouldSpawnTerrainParticles() && blockstate.getRenderShape() != INVISIBLE` 对齐。

### 43. 水下植物必须实现 ILiquidContainer，否则会被水流冲毁

`FlowingFluid::isBlocked()` 对**未实现 `ILiquidContainer`** 的方块只按 `canBeReplacedByFluid()`（= `canBeReplaced || !isSolid`）判定是否可被流体替换。而 `kelp` / `kelp_plant` / `seagrass` / `tall_seagrass` 都注册为 `.noCollision().notSolid()`，`m_isSolid` 为 false，于是被判为"可被流体替换"：水流每次 tick 都把它们当作被冲毁的方块，经 `WaterFluid::beforeReplacingBlock` 当作"方块被水破坏"生成一次掉落物。实测单次会话堆积 19326 个物品实体（该批区块常驻内存，而实体在模拟距离外被冻结、`age` 不增长，故永不超龄消失）。

原版语义：这四类方块都实现 `LiquidBlockContainer` 且 `canPlaceLiquid()`/`placeLiquid()` 恒返 false——流体既不能灌入该格，也不能替换该格的植物。项目侧对应 `ILiquidSealed`（`ILiquidContainer` 的"永不接收"实现，与 `IWaterLoggable` 的"总是接收"互为对立）。

配套注意两点：

1. `FlowingFluid::flowInto` 对 `ILiquidContainer` 分支必须**独占接管**（委派 `receiveFluid` 后直接返回），不可在 `receiveFluid` 返回 false 时继续落到"替换方块 + 生成掉落物"路径。原版 `FlowingFluid::spreadTo` 是 if/else 结构，容器分支不会落到 `beforeDestroyingBlock` + `setBlock`。
2. 新增任何"占位于水中、但不应被水流替换"的方块（含水植物、水下装饰等）都必须实现 `ILiquidSealed`，只靠 `.notSolid()` 是不够的。回归测试见 `tests/unit/common/world/fluid/SealedPlantLiquidTest.cpp`。

### 44. 窒息 / 视线判定挂在属性谓词上，不要写成方块子类方法

`Block::isSuffocatingAt` / `isViewBlockingAt` 的**默认**判据是
`blocksMotion() && isCollisionShapeFullBlock()`（与 `BlockState#isSuffocating` 一致），
个别方块通过 `BlockProperties::suffocating()` / `viewBlocking()` 换成共享谓词实例
（`registry/BlockStatePredicates.hpp`）。

两个坑：

1. **只覆写窒息时视线会跟着变**：原版 `isViewBlocking` 默认就是 `isSuffocating` 那个实例，
   所以「恒不窒息」的树叶、玻璃、铜格栅必须**两个都覆写**；只写 `suffocating(never)` 的话，
   视线判定也会一并变成 false（反之亦然）。
2. **默认判据的输入是碰撞形状，不是遮挡形状**：耕地、泥土路径、灵魂沙、泥巴的碰撞比满格
   矮 1~2 像素，默认判据会给 false，但原版把它们注册成恒 true 的窒息/挡视线方块；
   冰、信标、刷怪笼、潜影盒则是满格碰撞 + `noOcclusion`，默认判据给 true——不要因为
   「它透光」就顺手加 `never()`。改完用差分能力 `block_view_blocking` 逐状态核对。
