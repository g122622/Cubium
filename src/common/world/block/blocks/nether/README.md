# 下界方块模块 (Nether Blocks)

下界方块模块提供下界相关方块的实现。

## 目录结构

```
nether/
├── README.md                   # 本文档
├── EnderChestBlock.hpp/cpp     # 末影箱方块（含水、水平朝向、开盖动画）
├── FireBlock.hpp               # 普通火焰方块（可蔓延）
├── SoulFireBlock.hpp           # 灵魂火焰方块（蓝色火焰，更高伤害）
├── NetherPortalBlock.hpp       # 下界传送门方块
├── NetherWartBlock.hpp         # 下界疣方块（可生长）
├── NyliumBlock.hpp             # 绯红/诡异菌岩方块
├── MagmaBlock.hpp              # 岩浆块方块
├── NetherSproutsBlock.hpp      # 下界苗方块（小型装饰植物）
└── NetherRootsBlock.hpp        # 下界菌索方块（绯红/诡异根须）
```

## 方块类型

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `EnderChestBlock` | 末影箱（含水、水平朝向、开盖动画） | HORIZONTAL_FACING, WATERLOGGED |
| `FireBlock` | 普通火焰，可蔓延 | AGE_0_15, NORTH/SOUTH/EAST/WEST/UP |
| `SoulFireBlock` | 灵魂火焰（继承 FireBlock） | 同 FireBlock |
| `NetherPortalBlock` | 下界传送门 | HORIZONTAL_AXIS |
| `NetherWartBlock` | 下界疣（可生长） | AGE_0_3 |
| `NyliumBlock` | 绯红/诡异菌岩 | 无 |
| `MagmaBlock` | 岩浆块 | 无 |
| `NetherSproutsBlock` | 下界苗（继承 BushBlock） | 无 |
| `NetherRootsBlock` | 下界菌索（继承 BushBlock） | 无 |

## 内部模块关系

```
FireBlock (基类)
    └── SoulFireBlock (继承 FireBlock，限制只能放在灵魂沙/灵魂土上)

BushBlock (来自 agricultural/ 模块)
    ├── NetherSproutsBlock (可放置在菌岩、灵魂土上)
    └── NetherRootsBlock (可放置在菌岩、灵魂土上)
```

- `FireBlock`：火焰基类，实现蔓延、点燃、实体碰撞伤害等核心逻辑。提供静态方法 `getFireState()` 根据下方方块自动选择火焰类型
- `SoulFireBlock`：继承 FireBlock，重写 `isValidPosition()` 限制基座，重写 `canBurn()` 禁止蔓延
- `NetherWartBlock`：独立方块，4阶段生长，只能种在灵魂沙上
- `NyliumBlock`：独立方块，光照过高时退化为下界岩
- `MagmaBlock`：独立方块，水中生成气泡柱，踩踏时造成烫脚伤害
- `NetherPortalBlock`：独立方块，检测框架有效性、实体传送
- `NetherSproutsBlock` / `NetherRootsBlock`：继承 BushBlock，扩展 `canSustain()` 支持菌岩和灵魂土，返回 PlantType::Nether
- `NetherWartBlock`：独立实现 Block + IPlantable，返回 PlantType::Nether，只能种在灵魂沙上

## 上下游外部依赖关系

### 上游依赖

| 依赖模块 | 用途 |
|----------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/block/BlockTags` | 方块标签（SOUL_FIRE_BASE_BLOCKS、FIRE 等） |
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性（AGE_0_15、HORIZONTAL_AXIS 等） |
| `physics/collision/CollisionShape` | 碰撞形状 |
| `blocks/agricultural/BushBlock` | 植物基类（NetherSproutsBlock、NetherRootsBlock 继承） |
| `entity/Entity` / `entity/LivingEntity` | 实体交互（火焰伤害） |

### 下游依赖

| 模块 | 用途 |
|------|------|
| `VanillaBlocks` | 注册原版下界方块实例 |
| `BlockRegistry` | 方块注册表 |
| `item/FlintAndSteelItem` | 打火石决定生成普通火还是灵魂火 |
| `world/explosion/Explosion` | 爆炸生成火焰时选择类型 |
| `world/fluid/fluids/LavaFluid` | 岩浆点燃火焰时选择类型 |
| `entity/entities/projectile/AbstractFireballEntity` | 火球放置火焰时选择类型 |
| 世界生成 | 下界生物群系生成 |

## 容易踩的坑

### 1. 火焰蔓延必须检查游戏规则

`FireBlock::tick()` 必须先检查 `doFireTick` 游戏规则，否则禁用火焰蔓延时仍会蔓延。

### 2. 灵魂火不能蔓延

`SoulFireBlock::canBurn()` 返回 false，禁止蔓延到其他方块。普通火可以通过 `FireInfoRegistry` 配置蔓延参数，灵魂火则完全跳过。

### 3. 下界疣只能种在灵魂沙

`NetherWartBlock::isValidPosition()` 检查下方是否为灵魂沙（`soul_sand`），不是 `soul_soil`。这与灵魂火的基座不同。

### 4. 菌岩退化条件

`NyliumBlock` 只在随机 tick 时检查光照，不是每个 tick 都检查。退化使用 `randomTick()`，需要确保 `ticksRandomly()` 返回 true。

### 5. 岩浆块气泡柱延迟和烫脚伤害

`MagmaBlock::neighborChanged()` 检测到上方有水后，调度 20 tick 延迟才生成气泡柱，不是立即生成。这是 MC 1.16.5 的行为。

`MagmaBlock::onEntityWalk()` 对非潜行的活体生物造成 1 点烫脚伤害（`DamageType::HotFloor`）。注意：岩浆块 **不响应随机刻**（`ticksRandomly()` 返回 false），气泡柱的视觉效果由 `BubbleColumnBlock::animateTick()` 产生。

### 6. 下界苗/菌索的支撑面

`NetherSproutsBlock` 和 `NetherRootsBlock` 继承自 `BushBlock`，需要重写 `canSustain()` 以支持菌岩和灵魂土，否则只能放在 BushBlock 默认支持的地面（草地、泥土等）。

### 8. 火焰类型选择必须使用 FireBlock::getFireState()

所有需要放置火焰的场景（爆炸、岩浆点火、火球、打火石等）必须使用 `FireBlock::getFireState(world, pos)` 而非直接使用 `VanillaBlocks::FIRE->defaultState()`。该方法根据下方方块是否为灵魂沙/灵魂土自动返回普通火或灵魂火，对应 MC 原版 `BaseFireBlock.getState()`。

`NetherPortalBlock::isValidPosition()` 检查六个方向是否有传送门方块或黑曜石，如果检测逻辑不完整，传送门方块可能意外消失或残留。
