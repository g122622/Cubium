# 洞穴方块 (Cave Blocks)

洞穴与溶洞生物群系特有的方块实现，涵盖紫水晶、滴水石、发光地衣、洞穴藤蔓、苔藓、杜鹃花、孢子花等。

## 目录结构

```
cave/
├── README.md                          # 本文档
├── AmethystBlock.hpp/.cpp             # 紫水晶方块（装饰性）
├── AmethystClusterBlock.hpp/.cpp      # 紫水晶簇（可生长的晶体）
├── AzaleaBlock.hpp/.cpp              # 杜鹃花方块（地表装饰，下方需要有根土）
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
├── SmallDripleafBlock.hpp/.cpp       # 小滴水叶
└── SporeBlossomBlock.hpp/.cpp        # 孢子花方块（顶面悬挂，粒子效果）
```

## 内部模块关系

```
Block
├── GlowLichenBlock → Block, IWaterLoggable
├── MossBlock → Block, IGrowable
├── SporeBlossomBlock → Block
├── HangingRootsBlock → Block
├── PowderSnowBlock → Block
├── RootedDirtBlock → Block
├── AzaleaBlock → BushBlock, IGrowable
├── AmethystBlock → Block
├── BuddingAmethystBlock → Block
├── AmethystClusterBlock → Block, IBucketPickupHandler
├── PointedDripstoneBlock → Block, IWaterLoggable
├── CaveVinesBlock → GrowingPlantHeadBlock, IGrowable
├── CaveVinesPlantBlock → GrowingPlantBodyBlock, IGrowable
├── FrogspawnBlock → Block
├── BigDripleafBlock → Block
├── BigDripleafStemBlock → Block
└── SmallDripleafBlock → BushBlock, IGrowable
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
| `world/block/growing_plant/` | 生长植物基类（GrowingPlantHeadBlock、GrowingPlantBodyBlock） |
| `physics/collision/CollisionShape` | 碰撞形状 |
| `util/Direction` | 方向枚举 |
| `entity/utils/ItemDropHelper` | 物品掉落工具 |
| `item/Items` | 物品注册表（GLOW_BERRIES等） |
| `sound/SoundEvents` | 音效事件 |

### 下游依赖（谁依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/block/registry/CaveBlocks` | 注册所有洞穴方块 |
| `world/block/BlockTags` | `CRYSTAL_SOUND_BLOCKS`等标签 |

## 容易踩的坑

### #1. GlowLichenBlock 形状需要根据面方向组合

`GlowLichenBlock` 预计算了64种形状组合（2^6 = NORTH|SOUTH|EAST|WEST|UP|DOWN），每个面方向是一个1像素厚的薄板。通过 `_getShapeIndex` 位编码索引从 `m_shapes` 数组查找。当没有任何面激活时返回 `fullBlock` 形状。

### #2. GlowLichenBlock 继承 IWaterLoggable

`GlowLichenBlock` 同时继承 `Block` 和 `IWaterLoggable`，需要正确实现 `isWaterlogged()`、`getFluidState()` 和含水刻调度。

### #3. SporeBlossomBlock 需要上方实心面支撑

`SporeBlossomBlock::isValidPosition()` 检查上方方块的向下实心面（`isSolidSide`），且当前位置不能在水中。支撑失效时直接在 `updatePostPlacement` 中返回空气状态。

### #4. BuddingAmethystBlock 使用 ResourceLocation 查找方块

由于注册顺序依赖，`BuddingAmethystBlock` 通过 `BlockRegistry::instance().getBlock(ResourceLocation(...))` 延迟查找紫水晶簇方块，而非直接引用指针。

### #5. PointedDripstoneBlock 滴石生长逻辑

滴水石锥实现了完整的 MC 1.21.11 生长逻辑，包括：

- **随机刻生长**：以 0.011377778/tick 的概率触发，条件为上方1格是滴水石块且上方2格是水源
- **厚度计算**：根据邻居滴石方向和厚度推断当前位置的厚度（TipMerge/Tip/Frustum/Middle/Base）
- **放置方向**：根据点击面确定方向（顶面→朝下，底面→朝上），潜行时不合并尖端
- **支撑失效**：钟乳石失去支撑时延迟2tick掉落，石笋失去支撑时立即破坏
- **流体传输**：钟乳石可传输水/岩浆到下方炼药锅（水0.17578125/tick，岩浆0.05859375/tick）
- **泥巴变粘土**：当泥巴在滴水石块上方时，钟乳石可将水滴穿泥巴变为粘土（TODO：需要 Mud/Clay 方块注册后启用）
- **碰撞箱**：Tip朝上/朝下有不同形状，其他厚度均为全高柱状

关键静态方法：`canGrow`、`findTip`、`findRootBlock`、`canDrip`、`canTipGrow`、`calculateDripstoneThickness`、`maybeTransferFluid`

### #6. CaveVinesBlock/CaveVinesPlantBlock 的中键选取和收获

洞穴藤蔓的中键选取（`getCloneItemStack`）返回的是 `GLOW_BERRIES` 物品而非方块物品，因为原版MC中玩家中键点击洞穴藤蔓获得的是发光浆果。右键收获时掉落1个发光浆果并播放 `BLOCK_CAVE_VINES_PICK_BERRIES` 音效。骨粉效果是设置 `BERRIES=true`（不是生长），`setBlockState` 标志位为2。
