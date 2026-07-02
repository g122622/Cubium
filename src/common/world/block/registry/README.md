# registry/ - 原版方块注册

原版方块的静态引用和注册实现。每个方块类别拆分为独立的头文件和源文件，便于维护和编译。

## 目录结构

```
registry/
├── AgriculturalBlocks.hpp/.cpp # 农作物方块：小麦、胡萝卜、马铃薯、甜菜根、可可豆
├── VanillaBlocks.hpp/.cpp       # 主入口，VanillaBlocks 类继承所有分类结构体
├── BaseBlocks.hpp/.cpp          # 基础方块、矿石、矿物、原木、木板、冰、玻璃等
├── BuildingBlocks.hpp/.cpp      # 建筑、功能、含水、石砖、虫蚀、石英、海晶、紫珀、骨块等
├── BuildingVariantBlocks.hpp/.cpp # 楼梯、台阶、墙、栅栏、门、栅栏门、活板门、染色玻璃板、特殊方块
├── BambooBlocks.hpp/.cpp        # 竹子方块系列
├── CandleBlocks.hpp/.cpp        # 蜡烛方块系列（17色蜡烛+17色蜡烛蛋糕，CandleBlock + CandleCakeBlock）
├── CaveBlocks.hpp/.cpp          # 洞穴方块（紫水晶、滴水石、苔藓等）
├── CherryBlocks.hpp/.cpp        # 樱花木系列
├── ColoredBlocks.hpp/.cpp       # 染色方块：羊毛、地毯、染色玻璃、混凝土、陶瓦、床（16色BedBlock）、潜影盒（16色ShulkerBoxBlock+无色）
├── CopperBlocks.hpp/.cpp        # 铜方块系列（含氧化阶段）
├── DeepslateBlocks.hpp/.cpp     # 深板岩系列
├── GardenBlocks.hpp/.cpp        # 花园方块
├── MangroveBlocks.hpp/.cpp      # 红树林系列
├── MudBlocks.hpp/.cpp           # 泥土系列
├── NaturalBlocks.hpp/.cpp       # 自然方块：冰变种、粘液、珊瑚、海洋方块、仙人掌、蜂巢/蜂箱等
├── NetherBlocks.hpp/.cpp        # 下界方块、末地方块、下界扩展植物、绯红/诡异木板及衍生方块
├── PaleGardenBlocks.hpp/.cpp    # 苍白花园系列
├── RedstoneBlocks.hpp/.cpp      # 红石方块、铁轨方块
├── SculkBlocks.hpp/.cpp         # 幽匿系列
├── ShelfBlocks.hpp/.cpp         # 书架方块系列（1.21.4+ 12种木质变体，3槽位物品存储，侧链连接）
├── SignBannerBlocks.hpp/.cpp    # 告示牌、旗帜
├── TrailsBlocks.hpp/.cpp        # 足迹方块
├── TrialBlocks.hpp/.cpp         # 试炼密室方块
├── TuffBlocks.hpp/.cpp          # 凝灰岩系列
├── VegetationBlocks.hpp/.cpp    # 植被：草、花、蘑菇、树苗、南瓜西瓜、甜浆果丛
├── WildBlocks.hpp/.cpp          # 野生方块
└── README.md
```

## 内部模块关系

`VanillaBlocks` 类通过多重继承组合所有分类结构体，每个结构体位于 `mc::block_registry` 命名空间中：

```
VanillaBlocks
├── 继承所有分类结构体（BaseBlocks, BuildingBlocks, NetherBlocks 等）
├── 静态 Block* 指针 → 指向 BlockRegistry 中注册的方块实例
└── initialize() → 按顺序调用各 registerXxxBlocks() 函数
```

各分类结构体相互独立，仅通过 `BlockRegistry` 单例进行方块注册和存储。

## 上下游依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `world/block/Block.hpp` | 方块基类定义 |
| `world/block/BlockRegistry.hpp` | 方块注册表单例 |
| `world/block/blocks/` | 具体方块类型实现 |
| `world/fluid/` | 流体注册（WATER, LAVA） |

### 下游依赖（谁依赖本模块）

| 模块 | 用途 |
|------|------|
| 世界生成 (`world/gen/`) | 生成器放置方块（158+ 文件引用） |
| 物品系统 (`item/`) | 物品与方块对应 |
| 实体系统 (`entity/`) | 实体与方块交互 |
| 服务器启动 (`MinecraftServer.cpp`) | 初始化时调用 `VanillaBlocks::initialize()` |
| 渲染器 (`renderer/`) | 方块渲染 |
| 红石系统 (`world/redstone/`) | 红石信号计算 |

## 容易踩的坑

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
