# BlockItem 模块

方块物品实现，将方块包装为可手持/放置的物品。

## 目录结构

```
block/
├── BannerItem.hpp/cpp           # 旗帜物品
├── BedItem.hpp/cpp              # 床物品（重写 getStateForPlacement 检查头部位置可替换性）
├── BlockItem.hpp/cpp            # 方块物品基类（放置逻辑、NBT数据传递）
├── BlockItemRegistry.hpp/cpp    # 方块物品注册表
├── GameMasterBlockItem.hpp/cpp  # 游戏管理员方块物品（权限限制放置）
├── SeedsItem.hpp/cpp            # 种子物品（关联作物方块，右键耕地种植）
└── WallOrFloorItem.hpp/cpp      # 墙上/地面放置物品（告示牌、压力板等）
```

## 内部模块关系

```
Item(基类，item/core/)
  └── BlockItem(方块物品基类)
        ├── BannerItem(旗帜)
        ├── BedItem(床，重写 getStateForPlacement 检查头部位置)
        ├── GameMasterBlockItem(管理员方块，限制放置权限)
        ├── SeedsItem(种子物品，关联作物方块)
        └── WallOrFloorItem(墙上/地面放置)
BlockItemRegistry ──注册──→ BlockItem（方块→物品映射）
```

BlockItem 核心职责：
- 放置逻辑：碰撞检查、替换判断、方向计算
- NBT 数据传递：`applyBlockStateFromNBT` 从物品的 BlockStateTag 恢复方块状态属性，`setTileEntityNBT` 从 BlockEntityTag 恢复方块实体数据（受 `onlyOpsCanSetNbt()` 权限控制）

SeedsItem 核心职责：
- 继承 BlockItem，将种子物品关联到对应的作物方块
- 右键使用时，BlockItem::onItemUse() → tryPlace() → canPlace() 自动调用作物方块的 isValidPosition() 检查耕地和光照
- 种子与作物的映射：WHEAT_SEEDS→Wheat, PUMPKIN_SEEDS→PumpkinStem, MELON_SEEDS→MelonStem, BEETROOT_SEEDS→Beetroots, TORCHFLOWER_SEEDS→TorchflowerCrop, PITCHER_POD→PitcherCrop

GameMasterBlockItem 职责：
- 重写 `getStateForPlacement()`，当玩家没有 `canUseGameMasterBlocks()` 权限时返回 nullptr，阻止放置
- 适用于命令方块、结构方块、拼图方块、屏障方块等管理员专用方块

## 上下游外部依赖关系

**依赖上游：**
- `item/core/` - Item 基类、ItemStack
- `world/block/` - Block、BlockState、方块属性、GameMasterBlock（标记接口）
- `world/blockentity/` - BlockEntity（NBT 写入、onlyOpsCanSetNbt 权限检查）
- `entity/` - Player（OP 权限检查、canUseGameMasterBlocks）
- `util/nbt/` - NBT 读写

**被下游依赖：**
- `item/Items.hpp` - 物品注册
- `server/` - 服务端物品管理、方块交互权限
- `client/` - 客户端渲染

## 容易踩的坑

- BlockItem 放置时会排除放置者实体进行碰撞检查，无碰撞箱方块（水、空气）跳过此检查
- `applyBlockStateFromNBT` 使用 `StateHolder::withValueIndex` 而非 `with()`，因为属性类型在反序列化时未知
- `setTileEntityNBT` 仅当玩家有 OP 权限或 `onlyOpsCanSetNbt()` 返回 false 时才写入方块实体 NBT
- GameMasterBlockItem 在 `getStateForPlacement` 中做权限检查，而非 `canPlace`，因为 `canPlace` 是非虚方法
- GameMasterBlockItem 在 player 为 nullptr 时（如发射器放置）允许放置，与 MC Java 行为一致
- **告示牌/悬挂告示牌注册**：告示牌物品在 `Items::_registerSigns()` 中通过 `WallOrFloorItem` 注册（同时注册站立+墙壁变体），`BlockItemRegistry` 通过 `registerWallSign`/`registerWallHangingSign` lambda 建立方块→物品映射。注册顺序必须是 `Items::initialize()` → `BlockItemRegistry::initializeVanillaBlockItems()`，否则映射会因找不到已注册物品而跳过
- **`registerSimpleBlock` 与已注册物品**：如果某方块已通过 `Items::initialize()` 中的 `registerBlockBackedItem` 注册了物品（如按钮、压力板、门、栅栏等），`registerSimpleBlock` 会检测到已有物品并复用之，不会重复注册
- **种子物品注册**：种子在 `Items::_registerSeeds()` 中通过 `registerItem<SeedsItem>()` 注册，SeedsItem 关联到作物方块。BlockItemRegistry 通过 `registerSeedBlockItem` 建立作物方块→种子物品映射。小麦方块 (minecraft:wheat) 与小麦物品 (minecraft:wheat) 同名但类型不同（方块对应 BlockItem，物品对应普通 Item），`registerSimpleBlock` 会因 WHEAT 物品非 BlockItem 而跳过，这是正确行为
- **锁链物品注册**：MC 1.21+ 将 `minecraft:chain` 重命名为 `minecraft:iron_chain`。铁锁链通过 `registerBlockBackedItem` 在 `Items::initialize()` 中注册，铜锁链8个变种通过 `registerSimpleBlock` 在 `BlockItemRegistry::initializeVanillaBlockItems()` 中注册。所有锁链物品均属于 `ItemTags::CHAINS()` 标签
- **木质书架物品注册**：MC 1.21.4+ 新增12种木质书架变体（oak/spruce/birch/jungle/acacia/dark_oak/mangrove/cherry/pale_oak/bamboo/crimson/warped_shelf）。所有书架物品通过 `registerBlockBackedItem` 在 `Items::_registerBlockItems()` 中注册，`BlockItemRegistry` 通过 `registerSimpleBlock` 建立方块→物品映射。下界木质书架（crimson/warped）属于 `ItemTags::NON_FLAMMABLE_WOOD()` 和 `BlockTags::NON_FLAMMABLE_WOOD()` 标签但仍有 `ignitedByLava()` 属性（与MC原版一致：不可被火焰点燃但可被岩浆点燃）。所有12种书架物品属于 `ItemTags::WOODEN_SHELVES()` 标签，对应方块属于 `BlockTags::WOODEN_SHELVES()` 标签
- **铜方块物品注册**：铜方块（8个氧化/涂蜡变种×7类=56个）全部通过 `registerSimpleBlock` 在 `BlockItemRegistry::initializeVanillaBlockItems()` 中注册，包括铜块、切制铜、切制铜楼梯/台阶、铜格栅、铜灯、凿制铜、铜灯笼。铜门/铜活板门/铜锁链/避雷针在此前已注册。铜格栅（copper_grate）和铜栅栏（copper_bars）是两个不同的方块，后者尚未实现
- **床物品注册**：16色床物品使用自定义 `BedItem` 子类注册（非 `registerSimpleBlock`），在 `Items::_registerBeds()` 中通过 `registerItem<BedItem>()` 注册。`BedItem` 重写 `getStateForPlacement()` 检查头部位置可替换性：若 `placementPos.offset(facing)` 处方块不可替换则返回 `nullptr` 阻止放置，否则返回带正确 `HORIZONTAL_FACING` 的 FOOT 状态。放置后 `BedBlock::onBlockPlacedBy()` 自动在脚部前方放置 HEAD 方块，完成双格结构。床物品最大堆叠数为 1。`BlockItemRegistry` 不需要为床调用 `registerSimpleBlock`，因为床物品已在 `Items::_registerBeds()` 中注册
