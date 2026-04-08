# Block 模块

方块系统的核心模块，定义了 Minecraft 中所有方块的基础架构和实现。

## 目录结构

```
block/
├── Block.hpp/cpp           # 方块基类和状态系统
├── BlockPos.hpp            # 方块位置坐标类
├── BlockRegistry.hpp/cpp   # 方块注册表（单例）
├── HarvestTool.hpp         # 挖掘工具类型定义
├── ILiquidContainer.hpp    # 液体容器接口
├── Material.hpp/cpp        # 材质系统
├── VanillaBlocks.hpp/cpp   # 原版方块静态引用
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
- `canContainFluid()`：检查是否可以容纳指定流体
- `receiveFluid()`：接收流体
- `containsFluid()`：检查是否包含流体

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
