# 方块注册与状态索引

## 目录结构树

```text
registry/
├── vanilla/  # 原版内容的装配与注册数据
├── BlockRegistry.cpp  # 模块接口与实现
└── BlockRegistry.hpp  # 方块注册表
```

## 内部模块关系

BlockRegistry 持有方块及状态索引，vanilla 装配各行为族的原版实例；地图颜色快照仅由注册层绑定到公共模型。

## 上下游外部依赖关系

Block、BlockState 与各具体方块提供注册对象；服务器启动、物品映射、存档和协议编码查询注册结果。

## 容易踩的坑

- 方块和状态由注册表持有，发放的引用必须保持稳定。
- 形状工厂显式区分 box 的归一化坐标和 fromPixelBox 的像素坐标。
- 声音事件标识、方块族规则、LevelEventId 与模拟 GameEvent 分别管理。
- 原版地图颜色策略在注册时绑定，公共模型不直接引用原版快照。

### 1. 初始化顺序依赖

`VanillaBlocks::initialize()` 调用顺序有依赖关系，不可随意调整：
- `registerBaseBlocks()` 必须最先调用（AIR、WATER、LAVA 等基础方块被后续引用）
- `registerBuildingBlocks()` 在 `registerBuildingVariantBlocks()` 之前（楼梯/台阶引用原方块）
- `registerCandleBlocks()` 中蜡烛方块必须先于蜡烛蛋糕方块注册（CandleCakeBlock 构造函数需要引用对应 CandleBlock 实例作为 `candleBlock` 参数）
- `BlockTags::initialize()` 必须在所有方块注册后调用

#### NetherBlocks 内部初始化顺序

`registerNetherBlocks()` 内部同样有严格的顺序依赖。基础方块必须在其楼梯/台阶/墙变体之前注册，因为 `StairsBlock` 构造时需要引用基础方块的 `defaultState()`：

```
NETHER_BRICKS → NETHER_BRICK_STAIRS / NETHER_BRICK_SLAB / NETHER_BRICK_WALL
RED_NETHER_BRICKS → RED_NETHER_BRICK_STAIRS / RED_NETHER_BRICK_SLAB / RED_NETHER_BRICK_WALL
END_STONE_BRICKS → END_STONE_BRICK_STAIRS / END_STONE_BRICK_SLAB / END_STONE_BRICK_WALL
CRIMSON_PLANKS → CRIMSON_STAIRS（StairsBlock 引用木板 defaultState）
WARPED_PLANKS → WARPED_STAIRS（StairsBlock 引用木板 defaultState）
```

违反此顺序将导致 `StairsBlock` 构造时访问空指针（`nullptr->defaultState()`），引发 SEH 异常崩溃。

### 2. 静态指针初始化

所有 `Block*` 静态成员在 `initialize()` 前为 `nullptr`，访问前必须确保已初始化：

```cpp
// 危险：可能在初始化前访问
static Block* block = VanillaBlocks::STONE;  // nullptr!

// 安全：在 initialize() 后访问
void foo() {
    Block* block = VanillaBlocks::STONE;  // OK
}
```

### 3. 重复注册静默返回

重复注册同一资源位置的方块会返回已存在的方块，新属性被忽略。调试时注意日志中的警告。

### 4. 跨分类引用

部分方块引用其他分类的方块状态，例如：
- 珊瑚方块引用 `AIR`（来自 BaseBlocks）
- 楼梯/台阶引用基础方块状态

新增分类时需确认依赖关系并在正确位置注册。

### 5. BlockTags 初始化时机

`BlockTags::initialize()` 在 `VanillaBlocks::initialize()` 末尾调用，方块标签查询必须在之后进行。

### 6. CandleBlocks 注册顺序

`registerCandleBlocks()` 内部有严格的注册顺序依赖：17个蜡烛方块（CandleBlock）必须先于17个蜡烛蛋糕方块（CandleCakeBlock）注册，因为 CandleCakeBlock 构造函数的第二个参数是对应的蜡烛方块指针：

```
CANDLE → CANDLE_CAKE（CandleCakeBlock 引用 CandleBlocks::CANDLE）
WHITE_CANDLE → WHITE_CANDLE_CAKE
...（16色同理）
```

蜡烛方块属性：`Material::DECORATION, noCollision, notSolid, BlockSoundTypes::CANDLE, hardness=0.1, resistance=0.1`
蜡烛蛋糕属性：`Material::CAKE, notSolid, BlockSoundTypes::CLOTH, hardness=0.5, resistance=0.5`

标签：蜡烛属于 `CANDLES` 标签，蜡烛蛋糕属于 `CANDLE_CAKES` 标签。

### 7. FlowerPotBlocks 注册顺序与内容物依赖

`registerFlowerPotBlocks()` 注册空花盆（`minecraft:flower_pot`）和 37 种 `potted_*` 盆栽变体。每个 `potted_*` 方块在构造时传入对应的"内容物方块"指针（如 `potted_poppy` 传入 `VanillaBlocks::POPPY`），并自动注册到 `FlowerPotBlock::s_pottedByContent` 反查映射表中。

**依赖关系**：所有内容物方块（树苗、花、蘑菇、蕨、仙人掌、竹子、下界菌、根等）必须先于 `registerFlowerPotBlocks()` 注册。当前调用顺序为 `registerFlowerPotBlocks()` 位于 `registerCandleBlocks()` 之后，此时 VegetationBlocks、NaturalBlocks、NetherBlocks、CaveBlocks 等内容物来源分类已全部就绪。

**物品映射**：38 个花盆方块共享同一个 `minecraft:flower_pot` 物品（通过 `BlockItemRegistry::registerSimpleBlock` 重用机制自动派生物品名），中键选取已盆栽花盆时返回内容物对应的物品（见 `FlowerPotBlock::getCloneItemStack`）。

**标签**：所有花盆方块属于 `FLOWER_POTS` 标签；`potted_warped_fungus` 额外属于 `HOGLIN_REPELLENTS` 标签。
