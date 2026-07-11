# 洞穴方块 (Cave Blocks)

洞穴与溶洞生物群系特有的方块实现，涵盖紫水晶、滴水石、发光地衣、洞穴藤蔓、苔藓、杜鹃花、孢子花等。

## 目录结构

```
cave/
├── README.md                          # 本文档
├── AmethystBlock.hpp/.cpp             # 紫水晶方块（装饰性）
├── AmethystClusterBlock.hpp/.cpp      # 紫水晶簇（可生长的晶体）
├── AzaleaBlock.hpp/.cpp              # 杜鹃花方块（地表装饰，IPlantable(Plains)）
├── BigDripleafBlock.hpp/.cpp         # 大滴水叶（可倾斜的平台植物）
├── BigDripleafStemBlock.hpp/.cpp     # 大滴水叶茎
├── BuddingAmethystBlock.hpp/.cpp     # 芽生紫水晶方块（周期性生成紫水晶簇）
├── CaveVinesBlock.hpp/.cpp           # 洞穴藤蔓头部（发光浆果，可收获）
├── CaveVinesPlantBlock.hpp/.cpp      # 洞穴藤蔓身体（发光浆果，可收获）
├── FrogspawnBlock.hpp/.cpp           # 青蛙卵方块
├── GlowLichenBlock.hpp/.cpp          # 发光地衣方块（多面附着，光照等级7）
├── HangingRootsBlock.hpp/.cpp        # 垂根方块
├── MossBlock.hpp/.cpp                # 苔藓方块（可被骨粉传播）
├── PointedDripstoneBlock.hpp/.cpp    # 滴水石锥（钟乳石/石笋）
├── PowderSnowBlock.hpp/.cpp          # 细雪方块（可陷入）
├── RootedDirtBlock.hpp/.cpp          # 根土方块（杜鹃花下方）
├── SmallDripleafBlock.hpp/.cpp       # 小滴水叶（IPlantable(Water)）
└── SporeBlossomBlock.hpp/.cpp        # 孢子花方块（顶面悬挂，粒子效果）
```

## 内部模块关系

```
Block
├── GlowLichenBlock → Block, IWaterLoggable
├── MossBlock → Block, IGrowable
├── SporeBlossomBlock → Block
├── HangingRootsBlock → Block
├── PowderSnowBlock → Block, IBucketPickupHandler
├── RootedDirtBlock → Block
├── AzaleaBlock → Block, IGrowable, IPlantable（骨粉生长为杜鹃树，构造时注入 TreeGenerator）
├── FloweringAzaleaBlock → AzaleaBlock
├── AmethystBlock → Block
├── BuddingAmethystBlock → Block
├── AmethystClusterBlock → Block, IBucketPickupHandler
├── PointedDripstoneBlock → Block, IWaterLoggable
├── CaveVinesBlock → GrowingPlantHeadBlock, IGrowable
├── CaveVinesPlantBlock → GrowingPlantBodyBlock, IGrowable
├── FrogspawnBlock → Block
├── BigDripleafBlock → Block, IWaterLoggable, IGrowable
├── BigDripleafStemBlock → Block, IWaterLoggable
└── SmallDripleafBlock → Block, IWaterLoggable, IGrowable, IPlantable
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/IWaterLoggable` | 含水方块接口 |
| `world/block/IGrowable` | 骨粉可催熟接口 |
| `world/block/Material` | 材质系统 |
| `world/block/WaterLoggableHelpers` | 含水工具函数 |
| `world/block/BlockStateProperties` | 方块状态属性（BERRIES、AGE_0_25等） |
| `world/block/blocks/vegetation/SaplingBlock` | TreeGenerator 类型定义（AzaleaBlock 构造参数） |
| `world/block/blocks/vegetation/TreeGenerators` | azaleaTree() 工厂（注册时注入） |
| `world/block/growing_plant/` | 生长植物基类（GrowingPlantHeadBlock、GrowingPlantBodyBlock） |
| `physics/collision/CollisionShape` | 碰撞形状 |
| `util/Direction` | 方向枚举 |
| `entity/utils/ItemDropHelper` | 物品掉落工具 |
| `item/Items` | 物品注册表（GLOW_BERRIES等） |
| `sound/SoundEvents` | 音效事件 |
| `world/redstone/RedstonePower` | 红石信号检测（BigDripleafBlock） |
| `world/gameevent/GameEvents` | 游戏振动事件（BigDripleafBlock） |

### 下游依赖（谁依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/block/registry/CaveBlocks` | 注册所有洞穴方块 |
| `world/block/BlockTags` | `CRYSTAL_SOUND_BLOCKS`、`SMALL_DRIPLEAF_PLACEABLE`、`BIG_DRIPLEAF_PLACEABLE`等标签 |

## 容易踩的坑

### #1. GlowLichenBlock 形状需要根据面方向组合

`GlowLichenBlock` 预计算了64种形状组合（2^6 = NORTH|SOUTH|EAST|WEST|UP|DOWN），每个面方向是一个1像素厚的薄板。通过 `_getShapeIndex` 位编码索引从 `m_shapes` 数组查找。当没有任何面激活时返回 `fullBlock` 形状。

### #2. GlowLichenBlock 继承 IWaterLoggable

`GlowLichenBlock` 同时继承 `Block` 和 `IWaterLoggable`，需要正确实现 `isWaterlogged()`、`getFluidState()` 和含水刻调度。

### #3. SporeBlossomBlock 需要上方实心面支撑

`SporeBlossomBlock::isValidPosition()` 检查上方方块的向下实心面（`isSolidSide`），且当前位置不能在水中。支撑失效时直接在 `updatePostPlacement` 中返回空气状态。

`SporeBlossomBlock::animateTick()` 在客户端每 tick 被调用，生成两种粒子效果：
- `FallingSporeBlossom`：从花底部掉落的绿色孢子粒子（每 tick 1 个）
- `SporeBlossomAir`：在花周围10格半径内漂浮的环境粒子（14 次随机尝试，仅在非固体位置生成）

### #4. BuddingAmethystBlock 使用 ResourceLocation 查找方块

由于注册顺序依赖，`BuddingAmethystBlock` 通过 `BlockRegistry::instance().getBlock(ResourceLocation(...))` 延迟查找紫水晶簇方块，而非直接引用指针。

### #5. PointedDripstoneBlock 滴石生长逻辑

滴水石锥实现了完整的 MC 1.21.11 生长逻辑，包括：

- **随机刻生长**：以 0.011377778/tick 的概率触发，条件为上方1格是滴水石块且上方2格是水源
- **厚度计算**：根据邻居滴石方向和厚度推断当前位置的厚度（TipMerge/Tip/Frustum/Middle/Base）
- **放置方向**：根据点击面确定方向（顶面→朝下，底面→朝上），潜行时不合并尖端
- **支撑失效**：钟乳石失去支撑时延迟2tick掉落（生成 FallingBlockEntity），石笋失去支撑时立即破坏
- **坠落伤害**：钟乳石掉落砸中实体造成 `FallingStalactite` 类型伤害（每格1点，上限40点）
- **石笋伤害**：实体踩在朝上的TIP尖端时触发 `Stalagmite` 类型摔落伤害（摔落距离+2.5，伤害倍率2.0），替代普通摔落伤害
- **流体传输**：钟乳石可传输水/岩浆到下方炼药锅（水0.17578125/tick，岩浆0.05859375/tick），传输时触发 `WorldEvents::DRIPSTONE_DRIP` 事件
- **泥巴变粘土**：当泥巴在滴水石块上方时，钟乳石可将水滴穿泥巴变为粘土，触发 `GameEvents::BLOCK_CHANGE` 和 `WorldEvents::DRIPSTONE_DRIP`
- **碰撞箱**：Tip朝上/朝下有不同形状，其他厚度均为全高柱状

关键静态方法：`canGrow`、`findTip`、`findRootBlock`、`canDrip`、`canTipGrow`、`calculateDripstoneThickness`、`maybeTransferFluid`、`getDripParticlePosition`

**`getDripParticlePosition`**：计算钟乳石滴水粒子的生成位置。客户端在处理 `WorldEvents::DRIPSTONE_DRIP` 事件时调用此方法获取粒子坐标，Y偏移为 `STALACTITE_DRIP_START_PIXEL - 0.0625 = 0.25`。

**摔落伤害架构**：`Block::onFallenUpon` 默认实现调用 `entity.causeFallDamage()` 施加普通摔落伤害。`PointedDripstoneBlock::onFallenUpon` 重写：石笋尖端调用 `causeFallDamage` 并传入 `DamageSources::stalagmite()` 但不调用父类（替代普通摔落伤害）；非尖端调用父类 `Block::onFallenUpon`（保留普通摔落伤害）。

### #7. SmallDripleafBlock 双格完整性与放置检查

`SmallDripleafBlock` 实现了双格方块完整性检查（`updatePostPlacement` 中另一半消失时当前半部也变为空气），以及 `isValidPosition` 放置检查：
- 下半部分：通过 `mayPlaceOn()` 检查下方支撑，条件为 `SMALL_DRIPLEAF_PLACEABLE` 标签（黏土、苔藓块）或水源+`DIRT` 标签/耕地
- 上半部分：下方必须是同类型方块的下半部分
- 下方支撑失效时（`facing == Down`），下半部分也会断裂变为空气

### #8. BigDripleafBlock/BigDripleafStemBlock 支撑检查

`BigDripleafBlock::isValidPosition()` 检查下方是否为大滴叶、大滴叶茎或 `BIG_DRIPLEAF_PLACEABLE` 标签方块。`BigDripleafStemBlock::isValidPosition()` 检查下方是否为茎/标签方块**且**上方是否为茎/大滴叶。`BigDripleafBlock` 在下方支撑失效时直接返回空气；`BigDripleafStemBlock` 在上方或下方支撑失效时通过 `scheduleBlockTick(pos, this, 1)` 延迟1tick后再检查，若仍无法存活则在 `tick()` 中销毁方块并掉落物品——延迟机制可避免邻居更新期间的级联问题。`BigDripleafBlock` 还会在上方也是大滴叶时将自身转换为大滴叶茎。

BigDripleafBlock 完整实现了红石信号交互：`neighborChanged` 和 `tick` 中检测 `RedstonePower::isPowered()`，红石信号激活时立即重置倾斜状态为 NONE；`onEntityCollision` 在红石信号激活时不允许实体触发倾斜；`onProjectileHit` 直接设为 FULL 倾斜（不受红石影响）。倾斜状态变化时播放音效（`BLOCK_BIG_DRIPLEAF_TILT_DOWN` / `BLOCK_BIG_DRIPLEAF_TILT_UP`），FULL 倾斜触发 `GameEvents::BLOCK_CHANGE` 振动事件。

### #9. CaveVinesBlock/CaveVinesPlantBlock 的中键选取和收获

洞穴藤蔓的中键选取（`getCloneItemStack`）返回的是 `GLOW_BERRIES` 物品而非方块物品，因为原版MC中玩家中键点击洞穴藤蔓获得的是发光浆果。右键收获时掉落1个发光浆果并播放 `BLOCK_CAVE_VINES_PICK_BERRIES` 音效。骨粉效果是设置 `BERRIES=true`（不是生长），`setBlockState` 标志位为2。

### #10. PowderSnowBlock 冰冻交互

`PowderSnowBlock::onEntityCollision()` 是冰冻系统的入口点，对应 MC Java 的 `PowderSnowBlock.entityInside()`：

1. **设置细雪状态**：`entity.setIsInPowderSnow(true)` — 此状态在 `Entity::baseTick()` 中每帧重置为 false，由碰撞检测期间设置
2. **递增冰冻计时器**：如果 `entity.canFreeze()` 为 true，`ticksFrozen` 递增 1（上限 `getTicksRequiredToFreeze()` = 140 ticks）
3. **设置运动减速乘数**：`entity.setMotionMultiplier(Vector3(0.9, 0.9, 0.9))` — 实体在细雪中移动减速

**碰撞检测触发路径**：`LivingEntity::aiStep()` → `doBlockCollisions()` → 对每个碰撞方块调用 `getEntityInsideCollisionShape()` → AABB 相交检测 → `onEntityCollision()`

**碰撞形状**：`PowderSnowBlock::getCollisionShape()` 返回 `VoxelShapes::empty()`（无碰撞箱，实体可陷入），`getEntityInsideCollisionShape()` 使用默认的 `fullCube()`，确保 `onEntityCollision()` 被正确调用。

**冰冻计时器递减和伤害**：由 `LivingEntity::tickFreeze()` 处理，不在 `PowderSnowBlock` 中。详见 `entity/core/README.md` 中的冰冻系统文档。

### #11. AzaleaBlock / FloweringAzaleaBlock 骨粉生长为杜鹃树

`AzaleaBlock` 构造函数需要注入 `SaplingBlock::TreeGenerator` 回调（由 `TreeGenerators::azaleaTree()` 提供），注册时在 `CaveBlocks.cpp` 传入。骨粉流程：
- `canGrow`：检查上方无流体
- `canUseBonemeal`：45% 概率成功
- `grow`：构建 `WorldGenRegion` → 位置派生种子 → 清空方块 → 调用树生成器

注意 `grow()` 需要完整类型的 `WorldGenRegion`，必须 include `world/gen/chunk/IChunkGenerator.hpp`（仅 `IWorld.hpp` 的前向声明不够 `unique_ptr` 析构）。`FloweringAzaleaBlock` 继承 `AzaleaBlock`，构造函数同样需要 `TreeGenerator` 参数。
