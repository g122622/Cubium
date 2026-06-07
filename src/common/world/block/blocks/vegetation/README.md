# 植被方块模块 (Vegetation Blocks)

植被方块模块提供所有植物类方块的实现，包括树叶、花朵、树苗、仙人掌、甘蔗、藤蔓等。

## 目录结构

```
vegetation/
├── LeavesBlock.hpp/cpp          # 树叶方块（距离腐烂机制）
├── DoublePlantBlock.hpp/cpp     # 双格植物基类（向日葵、丁香、大型蕨等）
├── TallGrassBlock.hpp/cpp       # 高草/蕨类
├── FlowerBlock.hpp/cpp          # 花朵（蒲公英、玫瑰等）及双格花朵（丁香、牡丹等）
├── SaplingBlock.hpp/cpp         # 树苗（可生长成树木）
├── MushroomBlock.hpp/cpp        # 蘑菇及巨型蘑菇方块
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
│   ├── SaplingBlock
│   └── LilyPadBlock
├── MushroomBlock        # 独立实现
├── HugeMushroomBlock    # 巨型蘑菇组成方块
├── CactusBlock          # 独立实现
├── SugarCaneBlock       # 独立实现
├── VineBlock            # 独立实现
├── SweetBerryBushBlock  # 继承 BushBlock，实现 IGrowable
├── BambooBlock          # 继承 Block，实现 IGrowable
└── BambooSaplingBlock   # 继承 Block，实现 IGrowable
```

## 上下游依赖关系

### 上游依赖（本模块依赖的外部模块）

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/IGrowable` | 可生长接口（BambooBlock、SweetBerryBushBlock） |
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

### 1. 双格植物状态丢失

只更新下半部分而忘记更新上半部分会导致植物状态不一致。**必须使用 `DoublePlantBlock::placeAt()` 静态方法**同时放置两部分。

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
