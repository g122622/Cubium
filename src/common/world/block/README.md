# Block 模块

方块系统的核心模块，定义了 Minecraft 中所有方块的基础架构和实现。

## 目录结构

```
block/
├── Block.hpp/cpp           # 方块基类和状态系统
├── BlockPos.hpp            # 方块位置坐标类
├── BlockRegistry.hpp/cpp   # 方块注册表（单例）
├── HarvestTool.hpp         # 挖掘工具类型定义
├── IBucketPickupHandler.hpp # 桶提取接口
├── ILiquidContainer.hpp    # 液体容器接口
├── IWaterLoggable.hpp      # 含水方块接口
├── Material.hpp/cpp        # 材质系统
├── VanillaBlocks.hpp/cpp   # 原版方块静态引用
├── WaterLoggableHelpers.hpp # 含水方块工具函数
└── blocks/                 # 具体方块实现
    ├── AirBlock.hpp/cpp    # 空气方块
    ├── LiquidBlock.hpp/cpp # 液体方块
    ├── RotatedPillarBlock.hpp/cpp # 旋转柱状方块
    ├── SimpleBlock.hpp/cpp # 简单方块基类
    ├── FallingBlock.hpp/cpp # 可下落方块基类（沙子/红沙/砾石）
    ├── ChestBlock.hpp/cpp  # 箱子方块
    ├── HopperBlock.hpp/cpp # 漏斗方块
    ├── FurnaceBlocks.hpp/cpp # 熔炉方块
    ├── DoorBlock.hpp/cpp   # 门方块
    ├── FenceGateBlock.hpp/cpp # 栅栏门方块
    ├── CauldronBlock.hpp/cpp # 炼药锅方块
    ├── EnchantingTableBlock.hpp/cpp # 附魔台方块
    ├── building/           # 建筑方块（楼梯、台阶、墙等）
    │   ├── StairsBlock.hpp/cpp   # 楼梯方块
    │   ├── SlabBlock.hpp/cpp     # 台阶方块
    │   ├── WallBlock.hpp/cpp     # 墙方块
    │   ├── FenceBlock.hpp/cpp    # 栅栏方块
    │   ├── TrapDoorBlock.hpp/cpp # 活板门方块
    │   └── README.md
    ├── redstone/           # 红石方块（详见 redstone/README.md）
    │   ├── RedstoneBlock.hpp/cpp
    │   ├── RedstoneTorchBlock.hpp/cpp
    │   ├── RedstoneWallTorchBlock.hpp/cpp
    │   ├── RedstoneWireBlock.hpp/cpp
    │   ├── RedstoneRepeaterBlock.hpp/cpp
    │   ├── RedstoneComparatorBlock.hpp/cpp
    │   ├── ObserverBlock.hpp/cpp
    │   ├── PistonBlock.hpp/cpp
    │   └── ... 更多红石方块
    └── README.md           # blocks 子目录文档
```

**注意**：红石方块详情请参阅 [blocks/redstone/README.md](blocks/redstone/README.md)

## 文件详细介绍

### Block.hpp/cpp

**职责**：定义方块系统的核心类型，是整个模块的基础。

**主要内容**：

- **`VoxelShapes`**：VoxelShape 工具类，提供常用碰撞形状的静态实例
  - `empty()`：空形状
  - `fullCube()`：完整方块形状
  - `cube(x1, y1, z1, x2, y2, z2)`：创建自定义方块形状

- **`BlockState`**：不可变的方块状态对象，继承自 `StateHolder<Block, BlockState>`
  - 支持属性值的获取、设置、循环（O(1) 复杂度的状态转换）
  - 缓存方块属性（固体、不透明、硬度、挖掘工具等）
  - 提供碰撞形状、遮挡形状、AO 亮度值等查询方法
  - `toModelKey()` 生成稳定的属性键，供资源系统和模型缓存使用；实现上使用轻量字符串拼接，避免 `ostringstream` 带来的额外开销

- **`BlockProperties`**：方块属性构建器（流畅接口）
  - `hardness(value)`：设置硬度
  - `resistance(value)`：设置抗性
  - `lightLevel(level)`：设置光照等级（0-15）
  - `noCollision()`：设置无碰撞
  - `notSolid()`：设置非固体
  - `requiresTool()`：设置需要工具采集
  - `flammable(bool)`：设置可燃性
  - `opacity(value)`：设置光照透明度
  - `propagatesSkylightDown(bool)`：设置天空光传播
  - `harvestTool(type)`：设置挖掘工具类型
  - `harvestLevel(level)`：设置挖掘等级
  - `lootTableId(id)`：设置掉落表ID

- **`Block`**：方块基类
  - 静态方法：`getBlock()`, `getBlockState()`, `forEachBlock()`, `forEachBlockState()`
  - 属性查询：`blockLocation()`, `blockId()`, `material()`, `hardness()`, `resistance()`, `lightLevel()`, `opacity()`
  - 虚方法：
    - `getShape()`：渲染形状
    - `getCollisionShape()`：碰撞形状
    - `getOcclusionShape()`：遮挡形状
    - `isAir()`：是否为空气
    - `isSolid()`：是否为固体
    - `isOpaque()`：是否不透明
    - `getOpacity()`：光照透明度
    - `propagatesSkylightDown()`：天空光传播
    - `isSolidSide()`：实体面检查
    - `getFluidState()`：流体状态
    - `tick()`：计划刻处理
    - `randomTick()`：随机刻处理
    - `neighborChanged()`：邻居更新
    - `onBlockAdded()`：放置处理
    - `onBlockRemoved()`：移除处理
    - `isReplaceable()`：方块是否可被替换（支持双层台阶等）

### BlockPos.hpp

**职责**：定义方块位置坐标类，用于精确定位方块在世界中的位置。

**主要内容**：

- 三维整数坐标 `(x, y, z)`
- 算术运算：`+`, `-`, `*`, `+=`, `-=`
- 比较运算：`==`, `!=`, `<`
- 方向偏移：`up()`, `down()`, `north()`, `south()`, `east()`, `west()`, `offset()`
- 转换方法：`toVector3()`, `center()`, `toId()`
- 区块相关：`chunkX()`, `chunkZ()`, `localX()`, `localZ()`, `sectionIndex()`
- 哈希支持：支持作为 `std::unordered_map` 的键

### BlockRegistry.hpp/cpp

**职责**：单例模式的方块注册表，管理所有方块的注册和查找。

**主要内容**：

- **注册方法**：`registerBlock<BlockType>(id, args...)`
- **查找方法**：
  - `getBlock(blockId)`：按数字ID查找
  - `getBlock(resourceLocation)`：按资源位置查找
  - `getBlockState(stateId)`：按状态ID查找
  - `get(resourceLocation)`：便捷方法，返回默认状态
  - `airState()`：获取空气方块状态
- **遍历方法**：`forEachBlock()`, `forEachBlockState()`
- **统计方法**：`blockCount()`, `stateCount()`

**ID分配规则**：
- `minecraft:air` 始终获得 ID 0
- 其他方块从 ID 1 开始递增分配

### Material.hpp/cpp

**职责**：定义方块材质系统，描述方块的基础物理属性。

**主要内容**：

- **`Material`**：不可变材质类
  - `blocksMovement()`：是否阻挡移动
  - `isFlammable()`：是否可燃
  - `isLiquid()`：是否为液体
  - `isSolid()`：是否为固体
  - `isReplaceable()`：是否可替换
  - `isOpaque()`：是否不透明
  - `getPushReaction()`：推动反应类型
  - `materialColor()`：材质颜色索引

- **预定义材质**（26种）：
  - 基础：`AIR`, `STRUCTURE_VOID`, `ROCK`, `EARTH`, `WOOD`, `PLANT`, `REPLACEABLE_PLANT`
  - 液体：`WATER`, `LAVA`
  - 特殊：`LEAVES`, `GLASS`, `ICE`, `WOOL`, `SAND`, `IRON`, `SNOW`, `SLIME`, `TNT`, `SPONGE`, `CORAL`, `WEB`
  - 功能：`REDSTONE_LIGHT`, `PISTON`, `DECORATION`, `PORTAL`, `OCEAN_PLANT`, `SEA_GRASS`, `FIRE`
  - 工具相关：`ANVIL`, `GOURD`, `TALL_PLANTS`, `BAMBOO`, `NETHER_WOOD`, `MOSS`

- **`MaterialBuilder`**：材质构建器（流畅接口）

### HarvestTool.hpp

**职责**：定义挖掘工具类型常量，用于方块和工具系统之间的通信。

**工具类型**：
- `None (0)`：无需工具
- `Pickaxe (1)`：镐
- `Axe (2)`：斧
- `Shovel (3)`：锹
- `Hoe (4)`：锄
- `Sword (5)`：剑
- `Shears (6)`：剪刀

### ILiquidContainer.hpp

**职责**：定义液体容器接口，实现此接口的方块可以容纳液体。

**主要方法**：
- `canContainFluid(IWorld&, BlockPos, BlockState, Fluid&)`：检查是否可以容纳指定流体
- `receiveFluid(IWorld&, BlockPos, BlockState, FluidState&)`：接收流体
- `containsFluid(IWorld&, BlockPos, BlockState)`：检查是否包含流体

### IWaterLoggable.hpp

**职责**：定义含水方块接口，实现此接口的方块可以被水浸没（如栅栏、墙、楼梯、台阶等）。

**继承关系**：`IWaterLoggable` 继承自 `ILiquidContainer` 和 `IBucketPickupHandler`

**主要方法**：
- `getFluidState()`：获取方块内的流体状态（当WATERLOGGED为true时返回水流体状态）
- `isWaterlogged()`：检查方块是否含水
- `canContainFluid()`：检查是否可以容纳指定流体（仅接受水）
- `receiveFluid()`：接收流体（将WATERLOGGED设为true）
- `pickupFluid()`：从方块中提取流体（桶操作）
- `containsFluid()`：检查是否包含指定流体

**已实现的含水方块**（共14种）：
| 方块类 | 文件路径 | 特殊逻辑 |
|--------|----------|----------|
| StairsBlock | building/StairsBlock | 标准实现 |
| SlabBlock | building/SlabBlock | 双层台阶不能含水 |
| WallBlock | building/WallBlock | 标准实现 |
| FenceBlock | building/FenceBlock | 标准实现 |
| TrapDoorBlock | building/TrapDoorBlock | toggle()处理含水 |
| LanternBlock | decorative/LanternBlock | HANGING属性 |
| ChainBlock | decorative/ChainBlock | AXIS属性 |
| LadderBlock | decorative/LadderBlock | FACING属性 |
| ScaffoldingBlock | decorative/ScaffoldingBlock | DISTANCE+BOTTOM属性 |
| CampfireBlock | decorative/CampfireBlock | 含水时熄灭 |
| SeaPickleBlock | ocean/SeaPickleBlock | PICKLES属性 |
| ChestBlock | ChestBlock | TYPE属性（双箱连接）|
| PaneBlock | decorative/PaneBlock | 玻璃板/铁栏杆 |
| CoralBlock系列 | coral/CoralBlock | 离水变死珊瑚 |

**未实现的含水方块**（待开发）：
| 方块类 | 说明 |
|--------|------|
| ConduitBlock | 潮涌核心方块（已存在于DriedKelpBlock.hpp，需要补充IWaterLoggable） |
| SignBlock / WallSignBlock | 告示牌（站立/墙面），实现已创建待配置编译路径 |
| ButtonBlock系列 | 石质/木质按钮（含水时触发） |
| PressurePlateBlock系列 | 压力板（含水时触发） |
| DaylightDetectorBlock | 阳光探测器 |
| RedstoneWireBlock | 红石线（含水会断开） |

**实现含水方块的步骤**：
1. 继承 `IWaterLoggable` 接口
2. 添加 `WATERLOGGED` 属性到状态容器
3. 在 `getStateForPlacement()` 中检测水并设置WATERLOGGED
4. 在 `updatePostPlacement()` 中调度流体tick
5. 实现 `getFluidState()` 返回水流体状态

### IBucketPickupHandler.hpp

**职责**：定义桶提取接口，允许方块响应桶的提取操作。

**主要方法**：
- `pickupFluid()`：从方块中提取流体，返回提取的流体物品

### WaterLoggableHelpers.hpp

**职责**：提供含水方块的通用工具函数，消除重复代码。

**工具函数**：
```cpp
namespace mc::waterloggable {

// 获取水流体实例（带缓存）
[[nodiscard]] fluid::Fluid* getWaterFluid();

// 检查流体状态是否为水
[[nodiscard]] bool isWaterFluidState(const fluid::FluidState* fluidState);

// 检查流体是否为水
[[nodiscard]] bool isWaterFluid(const fluid::Fluid& fluid);

// 检查流体状态是否为水源
[[nodiscard]] bool isWaterSourceFluidState(const fluid::FluidState* fluidState);

// 调度水流体的 tick
void scheduleWaterTick(IWorld& world, const BlockPos& pos);

// 获取水流体状态（用于 getFluidState 实现）
[[nodiscard]] const fluid::FluidState* getWaterFluidState(const BlockState& state);

// 检测放置位置是否应该含水（用于 getStateForPlacement）
[[nodiscard]] bool shouldWaterlogAt(const IWorld& world, const BlockPos& pos);

// 检测放置位置是否有任何水（包括流动水）
[[nodiscard]] bool hasAnyWaterAt(const IWorld& world, const BlockPos& pos);

} // namespace mc::waterloggable
```

**使用示例**：
```cpp
// 在 getStateForPlacement 中
bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

// 在 updatePostPlacement 中
if (state.get(BlockStateProperties::WATERLOGGED())) {
    waterloggable::scheduleWaterTick(world, currentPos);
}

// 实现 getFluidState
const fluid::FluidState* getFluidState(const BlockState& state) const {
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}
```

### VanillaBlocks.hpp/cpp

**职责**：提供所有原版方块的静态指针，便于快速访问。

**方块分类**：
- **基础方块**：AIR, STONE, GRASS_BLOCK, DIRT, COBBLESTONE, OAK_PLANKS, WATER, LAVA, BEDROCK, SAND, GRAVEL
- **石头变种**：GRANITE, POLISHED_GRANITE, DIORITE, POLISHED_DIORITE, ANDESITE, POLISHED_ANDESITE
- **泥土变种**：COARSE_DIRT, PODZOL
- **砂岩系列**：SANDSTONE, CHISELED_SANDSTONE, CUT_SANDSTONE, RED_SANDSTONE
- **矿石方块**：GOLD_ORE, IRON_ORE, COAL_ORE, DIAMOND_ORE, EMERALD_ORE, LAPIS_ORE, REDSTONE_ORE, COPPER_ORE
- **下界矿石**：NETHER_QUARTZ_ORE, NETHER_GOLD_ORE, ANCIENT_DEBRIS
- **矿物方块**：GOLD_BLOCK, IRON_BLOCK, LAPIS_BLOCK, EMERALD_BLOCK, REDSTONE_BLOCK, DIAMOND_BLOCK
- **建筑方块**：BRICKS, MOSSY_COBBLESTONE, BOOKSHELF, TNT, SPONGE, WET_SPONGE, CRAFTING_TABLE
- **羊毛（16色）**：WHITE_WOOL ~ BLACK_WOOL
- **木板变种**：SPRUCE_PLANKS, BIRCH_PLANKS, JUNGLE_PLANKS, ACACIA_PLANKS, DARK_OAK_PLANKS
- **原木和树叶**：OAK_LOG, SPRUCE_LOG, BIRCH_LOG, JUNGLE_LOG, ACACIA_LOG, DARK_OAK_LOG 及对应树叶；并补齐 OAK_WOOD~DARK_OAK_WOOD、STRIPPED_*_LOG、STRIPPED_*_WOOD
- **植被方块**：SHORT_GRASS, TALL_GRASS, FERN, DANDELION, POPPY 等花卉
- **树苗**：OAK_SAPLING ~ DARK_OAK_SAPLING
- **石砖系列**：STONE_BRICKS, MOSSY_STONE_BRICKS, CRACKED_STONE_BRICKS, CHISELED_STONE_BRICKS
- **石英系列**：QUARTZ_BLOCK, CHISELED_QUARTZ_BLOCK, QUARTZ_PILLAR, QUARTZ_ORE
- **海晶系列**：PRISMARINE, PRISMARINE_BRICKS, DARK_PRISMARINE, SEA_LANTERN，以及 PRISMARINE/DARK_PRISMARINE 的 stairs/slab 变种
- **紫珀系列**：PURPUR_BLOCK, PURPUR_PILLAR
- **末地系列**：END_STONE_BRICKS, END_ROD
- **染色玻璃（16色）**：WHITE_STAINED_GLASS ~ BLACK_STAINED_GLASS
- **混凝土（16色）**：WHITE_CONCRETE ~ BLACK_CONCRETE
- **混凝土粉末（16色）**：WHITE_CONCRETE_POWDER ~ BLACK_CONCRETE_POWDER
- **陶瓦（17色）**：WHITE_TERRACOTTA ~ BLACK_TERRACOTTA, TERRACOTTA
- **下界方块**：SOUL_SAND, SOUL_SOIL, BASALT, POLISHED_BASALT, BLACKSTONE, POLISHED_BLACKSTONE, CRYING_OBSIDIAN, MAGMA, NETHER_WART_BLOCK
- **自然方块扩展**：CLAY, MYCELIUM, GRASS_PATH, PACKED_ICE, SLIME_BLOCK, CACTUS, DEAD_BUSH, LILY_PAD, VINE, COBWEB, SUGAR_CANE
- **海洋方块扩展**：SEA_PICKLE, KELP, KELP_PLANT, SEAGRASS, TALL_SEAGRASS，并补齐 BUBBLE_COLUMN、TURTLE_EGG
- **珊瑚方块扩展**：TUBE/BRAIN/BUBBLE/FIRE/HORN 的 coral_block、coral_fan、coral_wall_fan，并补齐 dead_* 对应 block/fan/wall_fan

**使用方法**：
```cpp
// 初始化（游戏启动时调用一次）
VanillaBlocks::initialize();

// 使用方块
Block* stone = VanillaBlocks::STONE;
const BlockState& state = stone->defaultState();
```

### blocks/ 子目录

#### AirBlock.hpp/cpp

**职责**：定义空气方块，无碰撞、非固体、非不透明。

**特点**：
- `isAir()` 始终返回 `true`
- `isSolid()` 返回 `false`
- `isOpaque()` 返回 `false`
- 渲染形状和碰撞形状均为空

#### SimpleBlock.hpp/cpp

**职责**：简单方块基类，无状态属性的方块。

**特点**：
- 继承自 `Block`
- 自动创建空状态容器
- 大多数基础方块（石头、泥土等）继承此类

#### RotatedPillarBlock.hpp/cpp

**职责**：旋转柱状方块基类，支持轴属性（X/Y/Z）。

**特点**：
- 用于原木、柱状玄武岩等可绕Y轴旋转的方块
- 拥有 `AXIS` 属性
- 提供 `getAxis()` 和 `withAxis()` 方法

#### LiquidBlock.hpp/cpp

**职责**：液体方块，与流体系统关联。

**特点**：
- 与 `FlowingFluid` 关联
- 重写 `getFluidState()` 返回对应的流体状态
- 方块等级（0-15）与流体等级（1-8）的映射
- 支持流体tick调度
- 重写 `ticksRandomly()` / `randomTick()`，把随机 tick 继续传给关联流体
- 使用 `Fluid::getTickDelay(world)` 调度流体 tick，岩浆节奏会跟随维度变化

## 模块整体职责

1. **方块类型定义**：定义所有方块类型及其行为
2. **状态管理**：管理方块状态属性系统
3. **材质系统**：定义方块的物理属性
4. **注册系统**：管理方块的注册和查找
5. **碰撞检测**：提供方块碰撞形状
6. **光照系统**：提供方块光照透明度信息
7. **挖掘系统**：定义方块挖掘工具和等级

## 输入和输出

### 输入

- 方块属性配置（`BlockProperties`）
- 材质定义（`Material`）
- 状态属性定义（`IProperty`）
- 流体系统（`FluidState`）
- 世界上下文（`IWorld`, `IBlockReader`）

### 输出

- 方块实例（`Block`）
- 方块状态（`BlockState`）
- 碰撞形状（`CollisionShape`）
- 光照信息（透明度、天空光传播）
- 挖掘信息（工具类型、挖掘等级）

## 依赖项

### 内部依赖

```
block/
├── core/Types.hpp          # 基础类型定义
├── core/Constants.hpp      # 常量定义（区块高度等）
├── resource/ResourceLocation.hpp  # 资源位置
├── physics/collision/CollisionShape.hpp  # 碰撞形状
├── util/property/          # 状态属性系统
│   ├── StateHolder.hpp
│   ├── StateContainer.hpp
│   ├── DirectionProperty.hpp
│   └── Properties.hpp
├── util/Direction.hpp      # 方向定义
├── util/math/MathUtils.hpp # 数学工具
├── util/math/Vector3.hpp   # 向量类型
├── world/IWorld.hpp        # 世界接口
├── world/fluid/            # 流体系统
│   ├── Fluid.hpp
│   ├── FlowingFluid.hpp
│   └── FluidRegistry.hpp
├── entity/loot/            # 掉落表系统
│   ├── LootTable.hpp
│   └── LootTableManager.hpp
└── util/math/random/IRandom.hpp  # 随机数接口
```

### 外部依赖

- `<memory>`：智能指针
- `<vector>`：动态数组
- `<unordered_map>`：哈希映射
- `<functional>`：函数对象
- `<string>`：字符串

## 使用方法

### 注册新方块

```cpp
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/SimpleBlock.hpp"

// 注册简单方块
auto& stone = BlockRegistry::instance().registerBlock<SimpleBlock>(
    ResourceLocation("minecraft:stone"),
    BlockProperties(Material::ROCK)
        .hardness(1.5f)
        .resistance(6.0f)
        .requiresTool()
);

// 注册带属性的方块
auto& log = BlockRegistry::instance().registerBlock<RotatedPillarBlock>(
    ResourceLocation("minecraft:oak_log"),
    BlockProperties(Material::WOOD)
        .hardness(2.0f)
);
```

### 使用方块状态

```cpp
#include "world/block/Block.hpp"
#include "world/block/VanillaBlocks.hpp"

// 获取默认状态
const BlockState& state = VanillaBlocks::OAK_LOG->defaultState();

// 获取属性值
Axis axis = state.get(RotatedPillarBlock::AXIS());

// 设置属性值（返回新状态）
const BlockState& newState = state.with(RotatedPillarBlock::AXIS(), Axis::Y);

// 检查方块类型
if (state.is(VanillaBlocks::OAK_LOG)) {
    // 是橡木原木
}

// 获取碰撞形状
const CollisionShape& shape = state.getCollisionShape();

// 检查光照属性
i32 opacity = state.getOpacity();
bool propagatesLight = state.propagatesSkylightDown();
```

### 查询方块信息

```cpp
// 按ID查找
Block* block = BlockRegistry::instance().getBlock(1);

// 按资源位置查找
Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stone"));

// 获取方块默认状态
const BlockState* state = BlockRegistry::instance().get(ResourceLocation("minecraft:stone"));

// 遍历所有方块
BlockRegistry::instance().forEachBlock([](Block& block) {
    // 处理每个方块
});

// 遍历所有状态
BlockRegistry::instance().forEachBlockState([](const BlockState& state) {
    // 处理每个状态
});
```

### 使用材质

```cpp
// 使用预定义材质
BlockProperties props(Material::ROCK);

// 创建自定义材质
Material customMaterial = MaterialBuilder()
    .solid()
    .flammable()
    .opaque()
    .build();
```

### 方块位置操作

```cpp
BlockPos pos(10, 64, -5);

// 获取相邻位置
BlockPos above = pos.up();
BlockPos below = pos.down();
BlockPos north = pos.north();

// 根据方向偏移
BlockPos offsetPos = pos.offset(Direction::North, 3);

// 转换为区块坐标
ChunkCoord cx = pos.chunkX();
ChunkCoord cz = pos.chunkZ();

// 转换为64位ID（用于哈希表键）
u64 id = pos.toId();
```

## 容易踩的坑

### 1. 状态不可变性

**问题**：`BlockState::with()` 返回的是新状态，不会修改原状态。

```cpp
// 错误：没有使用返回值
state.with(property, value);  // 状态未改变！

// 正确：使用返回的新状态
const BlockState& newState = state.with(property, value);
```

### 2. 材质比较

**问题**：材质比较应该使用地址比较，不是值比较。

```cpp
// 正确：使用预定义材质引用
if (block.material() == Material::ROCK) {
    // ...
}

// 错误：不要创建新的材质实例进行比较
Material myRock = MaterialBuilder().solid().opaque().build();
if (block.material() == myRock) {  // 始终为 false！
    // ...
}
```

### 3. 空气方块ID

**问题**：`minecraft:air` 始终分配 ID 0，其他方块从 ID 1 开始。在协议编码和存储时需注意。

### 4. BlockState 缓存

**问题**：`BlockState` 在构造时会缓存属性值，如果方块属性在构造后被修改，缓存不会更新。

```cpp
// 错误：构造后不应修改方块属性
// BlockProperties 在 Block 构造后即固定
```

### 5. 重复注册

**问题**：重复注册同一资源位置的方块会返回已存在的方块，而不是创建新的。

```cpp
// 两次注册同一ID返回相同方块
auto& block1 = BlockRegistry::instance().registerBlock<SimpleBlock>(
    ResourceLocation("test:block"),
    BlockProperties(Material::ROCK)
);
auto& block2 = BlockRegistry::instance().registerBlock<SimpleBlock>(
    ResourceLocation("test:block"),
    BlockProperties(Material::WOOD)  // 属性被忽略！
);
// block1 和 block2 是同一个方块
```

### 6. 状态ID与方块ID

**问题**：状态ID和方块ID是不同的概念。
- 方块ID：标识方块类型
- 状态ID：标识方块的特定状态（包含属性值）

```cpp
u32 blockId = block.blockId();       // 方块ID
u32 stateId = state.stateId();       // 状态ID
```

### 7. 光照透明度

**问题**：`opacity` 和 `propagatesSkylightDown` 是两个独立的属性。
- `opacity`：光线衰减程度（0-15）
- `propagatesSkylightDown`：是否传播天空光（如树叶、水）

```cpp
// 玻璃：透明但不传播天空光
BlockProperties(Material::GLASS).opacity(0).propagatesSkylightDown(false);

// 树叶：透明且传播天空光
BlockProperties(Material::LEAVES).opacity(0).propagatesSkylightDown(true);
```

### 8. LiquidBlock 等级映射

**问题**：方块等级和流体等级的映射关系：
- 方块 level=0 → 流体 level=8（源头）
- 方块 level=1-7 → 流体 level=1-7
- 方块 level=8-15 → 流体 level=8, falling=true

### 9. 挖掘工具类型同步

**问题**：`HarvestTool` 命名空间中的常量必须与 `item::tool::ToolType` 枚举值保持同步。

```cpp
// HarvestTool.hpp
constexpr u8 Pickaxe = 1;  // 必须与 ToolType::Pickaxe 值相同
```

### 10. SaplingBlock 和 TreeFeature 支撑方块一致性

**问题**：树苗和树木生成特性必须就根支撑方块达成一致，否则会出现"可以放置但不能生长"的不匹配。

**解决方案**：如果你扩展一侧，必须在同一更改中扩展另一侧。

### 11. MushroomBlock 自然 tick 用途

**问题**：`MushroomBlock` 自然 tick 用于低光传播，而不是巨型蘑菇构建。

**解决方案**：将巨型蘑菇生成保留在特性层，这样方块保持可测试和本地化。

### 12. IceBlock 融化与破坏路径分离

**问题**：`IceBlock::randomTick()` 和 `onBlockRemoved()` 职责混淆会导致方块状态不一致。

**解决方案**：
- `IceBlock::randomTick()` 只负责融化
- `onBlockRemoved()` 只负责破坏后的替换
- 不要再让随机刻回调 `onBlockRemoved()`
- 不要把同一坐标的写回放在旧方块回调之前

### 13. CropBlock 骨粉增长随机数

**问题**：`CropBlock` 的骨粉增长使用全局 `rand()` 会导致不确定性。

**解决方案**：骨粉增长必须从世界种子和方块位置派生随机数，不能再回到全局 `rand()`。

### 14. FarmlandBlock 降雨补湿条件

**问题**：`FarmlandBlock` 的降雨补湿如果只检查 `isRaining()` 而不检查具体位置，测试世界会出现伪阳性。

**解决方案**：`FarmlandBlock` 的降雨补湿要同时看 `isRaining()` 和 `canRainAt(pos.up())`。

### 15. WeatherUtils 降水判定

**问题**：天气降水判定只看温度会导致沙漠等无降水生物群系错误下雨。

**解决方案**：`WeatherUtils::canRainAt()` / `canSnowAt()` 需要结合生物群系的 `BiomeClimate::Precipitation::None` 以及温度阈值一起判断；沙漠、蘑菇岛、恶地等无降水生物群系必须在注册数据里显式标记为 `None`。

### 16. PaneBlock 连接形状

**问题**：`PaneBlock` 连接形状如果使用单个中心形状占位符或像素空间盒子坐标，会破坏碰撞和渲染测试。

**解决方案**：`PaneBlock` 连接形状按 4 位掩码缓存并使用规范化坐标，不要回退到单个中心形状占位符。

## 涉及的测试用例

测试文件：`tests/common/test_block.cpp`

### 测试分类

1. **Material 测试**
   - `MaterialTest.PredefinedMaterials`：预定义材质属性验证
   - `MaterialTest.MaterialBuilder`：材质构建器功能验证

2. **BlockProperties 测试**
   - `BlockPropertiesTest.BasicProperties`：基础属性验证
   - `BlockPropertiesTest.ChainProperties`：链式调用验证
   - `BlockPropertiesTest.SpecialFlags`：特殊标志验证
   - `BlockPropertiesTest.Strength`：强度属性验证

3. **StateContainer 测试**
   - `StateContainerTest.EmptyContainer`：空容器测试
   - `StateContainerTest.SingleProperty`：单属性测试
   - `StateContainerTest.MultipleProperties`：多属性测试
   - `StateContainerTest.GetProperty`：属性查询测试

4. **BlockState 测试**
   - `BlockStateTest.GetProperty`：属性获取测试
   - `BlockStateTest.SetProperty`：属性设置测试
   - `BlockStateTest.SetPropertySameValue`：相同值设置测试
   - `BlockStateTest.CycleProperty`：属性循环测试
   - `BlockStateTest.HasProperty`：属性存在检查
   - `BlockStateTest.StateId`：状态ID唯一性测试
   - `BlockStateTest.ToString`：字符串转换测试
   - `BlockStateTest.MultiplePropertiesInteraction`：多属性交互测试
   - `BlockStateTest.Caching`：状态缓存测试

5. **Block 测试**
   - `BlockTest.BasicProperties`：基础属性测试
   - `BlockTest.DefaultState`：默认状态测试
   - `BlockTest.IsAir`：空气判断测试
   - `BlockTest.StateCount`：状态计数测试

6. **BlockRegistry 测试**
   - `BlockRegistryTest.RegisterBlock`：方块注册测试
   - `BlockRegistryTest.GetBlockById`：按ID查找测试
   - `BlockRegistryTest.GetBlockByLocation`：按资源位置查找测试
   - `BlockRegistryTest.DuplicateRegistrationReturnsExistingBlock`：重复注册测试
   - `BlockRegistryTest.GetBlockState`：状态查找测试
   - `BlockRegistryTest.ForEachBlock`：遍历方块测试
   - `BlockRegistryTest.ForEachBlockState`：遍历状态测试
   - `BlockRegistryTest.BasicBlocksRegistration`：基础方块注册验证
   - `BlockRegistryTest.OreBlocksRegistration`：矿石方块注册验证
   - `BlockRegistryTest.LogBlocksRegistration`：原木方块注册验证
   - `BlockRegistryTest.StoneVariantsRegistration`：石头变种注册验证
   - `BlockRegistryTest.VegetationBlocksRegistration`：植被方块注册验证
   - `BlockRegistryTest.NetherBlocksRegistration`：下界方块注册验证
   - `BlockRegistryTest.UniqueBlockIds`：唯一ID验证

7. **VanillaBlocks 测试**
   - `VanillaBlocksTest.Initialization`：初始化测试，验证所有原版方块已正确注册

8. **BlockStateComparison 测试**
   - `BlockStateComparisonTest.IsComparisonWorks`：is() 方法比较测试

## 参考资料

- Minecraft 1.16.5 源码：`net.minecraft.block.Block`
- Minecraft Wiki：[Block](https://minecraft.fandom.com/wiki/Block)
