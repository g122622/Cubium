# Item Tag 模块

物品标签系统，用于物品分组，支持配方匹配和功能判断。

## 目录结构

```
tag/
├── ItemTag.hpp           # 物品标签类（物品集合）
├── ItemTag.cpp           # 物品标签实现
├── ItemTagLoader.hpp     # 物品标签 JSON 加载器（数据包加载）
├── ItemTagLoader.cpp     # 物品标签加载器实现
├── ItemTags.hpp          # 物品标签注册表（FLOWERS、CARPETS、DAMPENS_VIBRATIONS、FIRE_RESISTANT、SWORDS、AXES、PICKAXES、SHOVELS、HOES、BREAKS_DECORATED_POTS、CHAINS、BARS、WOODEN_DOORS、DOORS、WOODEN_TRAPDOORS、TRAPDOORS、NON_FLAMMABLE_WOOD、WOODEN_SHELVES、SHULKER_BOXES、VILLAGER_PLANTABLE_SEEDS 等）
├── ItemTags.cpp          # 标签表与便捷访问器（标签内容 100% 来自数据包）
└── README.md             # 本文件
```

## 内部模块关系

```
┌──────────────────────┐
│      ItemTags        │  注册表（静态方法）
│  FLOWERS()           │
│  CARPETS()           │
│  DAMPENS_VIBRATIONS()│
│  FIRE_RESISTANT()    │
│  SHULKER_BOXES()     │
│  WOODEN_DOORS()      │
│  DOORS()             │
│  WOODEN_TRAPDOORS()  │
│  TRAPDOORS()         │
│  NON_FLAMMABLE_WOOD()│
│  VILLAGER_PLANTABLE  │
│   _SEEDS()           │
│  registerTag()       │
└────────┬─────────────┘
         │ 管理
         ▼
┌─────────────────┐
│     ItemTag     │  单个标签（物品集合）
│  add()          │
│  addAll()       │
│  clear()        │
│  contains()     │
│  getItems()     │
│  isReplace()    │
└─────────────────┘

┌──────────────────────┐
│   ItemTagLoader      │  数据包 JSON 加载器
│  loadFromDataPack    │  → 从数据包加载标签到 ItemTags
│  Repository()        │
│  loadFromResource    │  → 从单个资源包加载标签
│  Pack()              │
│  loadFromJson()      │  → 解析单个标签 JSON
└──────────────────────┘
```

## 数据包加载

物品标签支持从数据包动态加载，遵循 MC Java 的标签加载语义。

### 标签 JSON 格式

```json
{
  "replace": false,
  "values": [
    "minecraft:diamond",
    "#minecraft:arrows",
    { "id": "minecraft:invalid_item", "required": false }
  ]
}
```

- **直接物品引用**: `"minecraft:diamond"` — 添加指定物品到标签
- **标签引用**: `"#minecraft:arrows"` — 递归引用其他标签的所有物品
- **对象格式**: `{"id": "...", "required": false}` — MC 1.21 TagEntry 格式，`required=false` 时缺失条目静默跳过
- **replace**: 当为 `true` 时，清空已有标签内容后追加（用于数据包覆盖）

### 加载路径

数据包中物品标签的文件路径为 `data/<namespace>/tags/item/<path>.json`，例如：
- `data/minecraft/tags/item/flowers.json` → `minecraft:flowers`
- `data/minecraft/tags/item/arrows.json` → `minecraft:arrows`
- `data/minecraft/tags/item/enchantable/weapon.json` → `minecraft:enchantable/weapon`

### 多数据包合并语义

当多个数据包定义同名标签时：
1. 按数据包优先级从低到高遍历
2. 默认追加模式：新数据包的标签内容追加到已有内容之后
3. `replace=true` 时：清空已有标签内容，仅使用当前数据包的内容

### 两阶段加载

`loadFromDataPackRepository()` 和 `loadFromResourcePack()` 使用两阶段加载，
确保 `#` 标签引用在被引用标签尚未解析时仍能正确解析：

**第一阶段**（收集原始数据）：
- 解析所有 JSON 文件，提取原始条目（不解析 `#` 标签引用）
- 将尚不存在的标签注册为空标签占位符到 `ItemTags`

**第二阶段**（按依赖顺序解析引用并填充）：
- 使用递归依赖解析确保被引用的标签先于引用者被填充
- 当标签 A 引用 `#B` 时，先递归解析 B 再读取 B 的物品
- 通过 `resolved`/`resolving` 集合检测循环依赖

两阶段加载解决了标签间的加载顺序依赖问题：例如 `flowers.json` 引用
`#minecraft:small_flowers` 时，即使 `small_flowers` 在 `flowers` 之后被遍历，
递归依赖解析会先填充 `small_flowers` 的内容，再解析 `flowers` 的引用。

注意：`loadFromJson()` 是单次使用的便捷方法，不经过两阶段加载，
标签引用只能解析到已经在 `ItemTags` 中注册的标签。

### 初始化流程

1. `ItemTags::initialize()` 只把状态标记为已初始化——**标签内容 100% 来自数据包**，
   不存在任何编译期快照
2. 服务端 `RegistryBootstrap` 依次执行 `ItemTags::clear()` →
   `ItemTagLoader::loadFromDataPackRepository()`，标签表整体由数据包重建
3. 客户端不自行加载数据包：单人游戏由同进程的内置服务端加载

## 上下游外部依赖关系

### 上游依赖

| 模块 | 依赖内容 |
|------|---------|
| `item/core/Item.hpp` | `Item` 类定义 |
| `item/core/ItemStack.hpp` | `ItemStack` 类定义 |
| `item/core/ItemRegistry.hpp` | 物品注册表（初始化时获取物品指针；加载器解析物品名称时查找） |
| `core/ResourceLocation.hpp` | 资源位置标识符 |
| `resource/DataPackRepository.hpp` | 数据包仓库（ItemTagLoader 使用） |
| `resource/pack/IResourcePack.hpp` | 资源包接口（ItemTagLoader 使用） |

### 下游依赖

| 模块 | 依赖内容 |
|------|---------|
| 实体繁殖逻辑 | `FLOWERS` 标签判断蜜蜂繁殖物品 |
| 羊驼装饰槽位 | `CARPETS` 标签判断有效装饰 |
| ItemEntity 振动阻尼 | `DAMPENS_VIBRATIONS` 标签判断羊毛物品是否阻尼振动 |
| ItemEntity 伤害处理 | `FIRE_RESISTANT` 标签判断防火物品是否免疫火焰伤害 |
| ItemStack::canBeHurtBy | `FIRE_RESISTANT` 标签检查物品是否可被火焰伤害源伤害 |
| 农民村民种植 | `VILLAGER_PLANTABLE_SEEDS` 标签判断农民可在耕地上种植的种子（小麦种子、胡萝卜、马铃薯、甜菜种子、火把花种子、瓶草荚果），对应 MC 1.21.11 `HarvestFarmland.plantCrop` |
| 铜傀儡雕像方块物品 | `COPPER_GOLEM_STATUES` 标签判断铜傀儡雕像物品变体（未涂蜡/涂蜡 × 4 个氧化等级） |
| 铜傀儡剪切 | `SHEARABLE_FROM_COPPER_GOLEM` 标签判断铜傀儡天线槽（`EquipmentSlot::Saddle`）物品是否可被剪刀剪下（MC 1.21.11 原版仅包含 poppy），由 `CopperGolemEntity::isShearable()` 调用 |
| 配方系统（未来） | `Ingredient` 将支持标签匹配 |
| 服务端初始化 | `MinecraftServer::initializeRegistries()` 调用 `ItemTagLoader` |

## 容易踩的坑

### 1. 初始化顺序依赖

`ItemTagLoader::loadFromDataPackRepository()` **必须**在以下初始化之后调用，否则数据包里
引用的方块物品尚未注册，会被判成「未知物品」，整个标签被拒绝：

1. `VanillaBlocks::initialize()` - 方块注册
2. `Items::initialize()` - 物品注册
3. `BlockItemRegistry::instance().initializeVanillaBlockItems()` - 方块物品注册

初始化入口：`RegistryBootstrap::initializeAll()`。

### 1.1 便捷访问器的缓存指针会被 `clear()` 打失效

`ItemTags::FLOWERS()` 等访问器把 `ItemTag*` 缓存在 static 局部变量里，`clear()` 必须让这些
缓存失效，否则访问器永远返回过期内容。**守卫绝不能解引用缓存指针**：对象此时已释放，
`tag->getId()` 会读到垃圾 `std::string` 并在 `ResourceLocation::hash` 崩溃（同进程连跑多个
服务器实例时实测 `ACCESS_VIOLATION - Read access at 0xffffffffffffffff`）。当前实现：

- `clear()` 先把标签内容清空、再把对象移入退役区（`retiredTags()`）冻结地址——避免分配器把
  同一地址复用给之后注册的**另一个**标签，令守卫误判存活；
- `liveTags()` 记录当前表内对象地址，`_isLive()` 只查集合、不解引用。

新增访问器必须照抄 `_isLive` 守卫。

### 1.2 不要内置标签快照

历史上 `ItemTags::initialize()` 用 `VanillaItemTags.gen.cpp` 把数据包展开成编译期表，它与
数据包会无声漂移（数据包含 cherry/copper 系物品，快照里没有），且玩家无法定制。该文件已
删除：数据包是唯一来源。

### 2. 标签判断使用方式

`Item::isIn(tag)` 和 `ItemTag::contains(item)` 功能相同，推荐使用前者更直观。检查 `ItemStack` 时需要先判空：

```cpp
if (!stack.isEmpty() && stack.getItem()->isIn(tag)) { ... }
```

### 3. 扩展标签

新增内置标签时，需要在 `ItemTags.cpp` 的 `initialize()` 中注册，并通过静态方法暴露访问入口。如果数据包中有对应的标签 JSON 文件，数据包加载会自动追加或替换内置默认值。

### 4. 数据包标签中的未知物品

数据包标签 JSON 中引用的物品如果在 `ItemRegistry` 中不存在：
- `required=true`（默认）：会输出警告日志并跳过
- `required=false`：静默跳过

这保证了数据包可以引用尚未实现的物品而不影响启动。
