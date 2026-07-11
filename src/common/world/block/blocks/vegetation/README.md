# 植被方块模块 (Vegetation Blocks)

植被方块模块提供所有植物类方块的实现，包括树叶、花朵、树苗、仙人掌、甘蔗、藤蔓等。

## 目录结构

```
vegetation/
├── LeavesBlock.hpp/cpp          # 树叶方块（距离腐烂机制）
├── DoublePlantBlock.hpp/cpp     # 双格植物基类（向日葵、丁香、大型蕨等）
├── TallGrassBlock.hpp/cpp       # 高草/蕨类
├── FlowerBlock.hpp/cpp          # 花朵（蒲公英、玫瑰等）及双格花朵（丁香、牡丹等）
├── SaplingBlock.hpp/cpp         # 树苗（可生长成树木，通过 FeaturePlacer 构建 WorldGenRegion 调用 TreeFeature）
├── TreeGenerators.hpp/cpp       # 树苗树木生成器工厂（为每种树苗创建 TreeGenerator 回调，azaleaTree() 也供 AzaleaBlock 复用）
├── MushroomBlock.hpp/cpp        # 蘑菇（IPlantable/Cave，MUSHROOM_GROW_BLOCK 标签判定）及巨型蘑菇方块（6方向属性）
├── CactusBlock.hpp/cpp          # 仙人掌（接触伤害、高度限制3格）
├── SugarCaneBlock.hpp/cpp       # 甘蔗（需靠近水源）
├── VineBlock.hpp/cpp            # 藤蔓（可攀爬、多方向附着）
├── LilyPadBlock.hpp/cpp         # 睡莲（水面放置）
├── SweetBerryBushBlock.hpp/cpp  # 甜浆果丛（采摘、减速伤害）
└── BambooBlock.hpp/cpp          # 竹子及竹子幼苗（最高16格）
```

## 内部模块关系

```
Block (基类)
├── LeavesBlock          # 独立实现，不依赖 BushBlock
├── BushBlock (agricultural)  # 植物基类，来自 agricultural 目录
│   ├── DoublePlantBlock      # 双格植物基类
│   │   ├── LilacBlock        # (在 FlowerBlock.hpp 中定义)
│   │   ├── RoseBushBlock
│   │   ├── PeonyBlock
│   │   └── SunflowerBlock
│   ├── TallGrassBlock
│   │   └── FernBlock
│   ├── FlowerBlock
│   ├── SaplingBlock        # 继承 BushBlock + IGrowable，通过 FeaturePlacer 构建 WorldGenRegion 调用 TreeFeature
│   └── LilyPadBlock
├── MushroomBlock        # 独立实现，IPlantable(Cave)，使用 MUSHROOM_GROW_BLOCK 标签判定放置
├── HugeMushroomBlock    # 巨型蘑菇组成方块（6方向属性：UP/DOWN/NORTH/SOUTH/EAST/WEST）
├── CactusBlock          # 独立实现，IPlantable(Desert)
├── SugarCaneBlock       # 独立实现，IPlantable(Beach)
├── VineBlock            # 独立实现（非 IPlantable，墙面附着）
├── LilyPadBlock         # 继承 BushBlock，IPlantable(Water)
├── SweetBerryBushBlock  # 继承 BushBlock，实现 IGrowable
├── BambooBlock          # 继承 Block，实现 IGrowable + IPlantable(Beach)
└── BambooSaplingBlock   # 继承 Block，实现 IGrowable + IPlantable(Beach)
```

## 上下游依赖关系

### 上游依赖（本模块依赖的外部模块）

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/BlockTags` | 方块标签系统（MUSHROOM_GROW_BLOCK 标签用于蘑菇放置判定） |
| `world/block/IGrowable` | 可生长接口（BambooBlock、SweetBerryBushBlock） |
| `world/block/PlantType` | 植物类型接口（IPlantable，用于土壤兼容性检测） |
| `world/block/blocks/agricultural/BushBlock` | 植物基类（来自 agricultural 目录） |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `physics/collision/CollisionShape` | 碰撞形状 |
| `util/property/Properties` | 方块状态属性 |
| `entity/utils/ItemDropHelper` | 物品掉落（甜浆果采摘） |

### 下游依赖（依赖本模块的外部模块）

| 模块 | 用途 |
|------|------|
| `world/block/BlockRegistry` | 方块注册 |
| `world/gen/feature/*` | 世界生成特征（树木、巨型蘑菇等） |
| `item/Item` | 对应物品（树叶物品、树苗物品等） |

## 容易踩的坑

### 1. 双格植物半部断裂逻辑

`DoublePlantBlock::updatePostPlacement()` 实现了完整的双格方块完整性检查，与 MC 原版 `DoublePlantBlock.updateShape` 一致：
- 使用 `isLower == isUpDirection` 条件统一处理：下半部分收到上方向更新或上半部分收到下方向更新时，检查另一半是否存在
- 当另一半消失时，当前半部分变为空气（返回 `airState`）
- 下半部分额外检查下方支撑（`isValidPosition`），支撑失效时也变为空气
- 水平方向（非 Y 轴）的邻居变化不触发断裂
- 所有继承 `DoublePlantBlock` 的子类（`LilacBlock`、`RoseBushBlock`、`PeonyBlock`、`SunflowerBlock`、`LargeFernBlock`）自动继承此逻辑

放置时**必须使用 `DoublePlantBlock::placeAt()` 静态方法**同时放置两部分，避免状态不一致。

### 2. 仙人掌周围固体检测

仙人掌周围检测不正确会导致仙人掌被错误移除。**只检测水平四个方向（North/South/East/West），不检测上下**。

### 3. 藤蔓附着检测

藤蔓在更新时检测错误会导致消失。正确做法是检测固体方块的侧面，使用 `_canAttachTo()` 方法。

### 4. 睡莲水面放置

睡莲需要同时检查：下方必须是水，当前位置必须是空气。只检查一方会导致放置逻辑错误。

### 5. 树叶距离计算

树叶使用 DISTANCE 属性（1-7），检测六个方向邻居来计算到原木的最小距离。距离超过6格（DISTANCE=7）的非持久树叶会腐烂。**玩家放置的树叶标记为 PERSISTENT=true 不会腐烂**。

### 6. 竹子高度计算

竹子最高16格，但需要通过 `_getNumBambooBlocksBelow()` 统计下方连续竹子数量来判断是否还能生长。STAGE 属性控制骨粉是否必定生效。

### 7. 甜浆果丛采摘逻辑

- AGE 0-1：不可采摘
- AGE 2：掉落 1-2 个浆果
- AGE 3：掉落 2-3 个浆果
- 采摘后 AGE 重置为 1

狐狸和蜜蜂免疫减速和伤害，其他 LivingEntity 穿过时受减速效果（XZ: 0.8, Y: 0.75）和伤害（AGE > 0 且移动距离 >= 0.003 时造成 1.0 伤害）。

### 8. 蘑菇放置判定逻辑

蘑菇（MushroomBlock）的放置判定分两层：

1. **`Block::canSustainPlant()`**（`PlantType::Cave` 分支）：检查下方方块是否属于 `MUSHROOM_GROW_BLOCK` 标签（菌丝、灰化土、绯红菌岩、诡异菌岩）。只有标签内的方块才返回 true，其他方块（包括泥土和石头）一律返回 false。
2. **`MushroomBlock::isValidPosition()`**：在 `canSustainPlant` 通过后额外检查光照条件。若下方方块属于 `MUSHROOM_GROW_BLOCK` 标签，则无条件允许放置；否则要求下方为固体方块且光照 < 13。

因此，**不要在 `canSustainPlant` 的 `PlantType::Cave` 分支中添加光照检查**——光照检查由 `MushroomBlock::isValidPosition()` 独立完成。

### 9. 花朵迷之炖菜效果

所有 `FlowerBlock` 注册时都带有迷之炖菜效果参数（EffectType + duration），通过 `hasStewEffect()` / `getSuspiciousStewEffect()` / `getEffectDuration()` 查询。效果映射在 `VegetationBlocks.cpp` 和 `PaleGardenBlocks.cpp` 注册时指定。`MooshroomEntity::_getStewEffectFromItem()` 通过 `BlockItemRegistry` 将物品转换为方块再检查是否为 `FlowerBlock` 来获取效果。

### 10. 树苗生长与 WorldGenRegion 适配

`SaplingBlock::grow()` 在 `IWorld&` 上下文中被调用（来自 `randomTick` 或骨粉），但 `TreeFeature::place()` 需要 `WorldGenRegion&`。这通过 `FeaturePlacer` 工具类桥接：

1. `SaplingBlock::grow()` 通过 `IWorld::asServerWorld()` 获取 `ServerWorld*`
2. 从 `ServerChunkManager::tryToGetChunkInMem()` 收集周围 3×3 已加载区块（`ChunkData*` → `IChunk*`）
3. 通过 `FeaturePlacer::createRegion()` 构建临时的 `WorldGenRegion`（使用无步骤验证构造函数，`m_generatingStep=nullptr` 使写入验证跳过）
4. 通过 `FeaturePlacer::populateWorldState()` 填充世界状态（种子、tick、时间等）
5. 将 `WorldGenRegion&` 传递给 `TreeGenerator` 回调，回调内调用 `TreeFeature::place()`

**注意**：`WorldGenRegion` 从 `ChunkData*`（而非 `ChunkPrimer*`）构造时，`dynamic_cast<ChunkPrimer*>` 会返回 nullptr，方块实体管理和液体后处理会被跳过。对于树木生成（仅设置原木和树叶方块，无方块实体和液体），这是可接受的。
