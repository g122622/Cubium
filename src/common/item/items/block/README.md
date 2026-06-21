#BlockItem 模块

方块物品实现，将方块包装为可手持 /
        放置的物品。

        ##目录结构

``` block /
├── BannerItem.hpp /
        cpp #旗帜物品
├── BlockItem.hpp / cpp #方块物品基类（放置逻辑、NBT数据传递）
├── BlockItemRegistry.hpp / cpp #方块物品注册表
├── GameMasterBlockItem.hpp / cpp #游戏管理员方块物品（权限限制放置）
└── WallOrFloorItem.hpp / cpp #墙上 /
        地面放置物品（告示牌、压力板等）
```

        ##内部模块关系

``` Item(基类，item / core /)
  └── BlockItem(方块物品基类)
        ├── BannerItem(旗帜)
        ├── GameMasterBlockItem(管理员方块，限制放置权限)
        └── WallOrFloorItem(墙上 / 地面放置) BlockItemRegistry ──注册──→ BlockItem（方块→物品映射）
```

        BlockItem 核心职责：
    - 放置逻辑：碰撞检查、替换判断、方向计算 -
    NBT 数据传递：`applyBlockStateFromNBT` 从物品的 BlockStateTag 恢复方块状态属性，`setTileEntityNBT` 从 BlockEntityTag
    恢复方块实体数据（受 `onlyOpsCanSetNbt()` 权限控制）

    GameMasterBlockItem 职责：
    - 重写 `getStateForPlacement()`，当玩家没有 `canUseGameMasterBlocks()` 权限时返回 nullptr，阻止放置 -
    适用于命令方块、结构方块、拼图方块、屏障方块等管理员专用方块

        ##上下游外部依赖关系

            ** 依赖上游：* *
        - `item / core /` -
    Item 基类、ItemStack - `world / block /` - Block、BlockState、方块属性、GameMasterBlock（标记接口）
    - `world / blockentity /` - BlockEntity（NBT 写入、onlyOpsCanSetNbt 权限检查）
    - `entity /` - Player（OP 权限检查、canUseGameMasterBlocks） - `util / nbt /` -
    NBT 读写

            ** 被下游依赖：* *
        - `item / Items.hpp` -
    物品注册 - `server /` - 服务端物品管理、方块交互权限 - `client /` -
    客户端渲染

    ##容易踩的坑

    - BlockItem 放置时会排除放置者实体进行碰撞检查，无碰撞箱方块（水、空气）跳过此检查
    - `applyBlockStateFromNBT` 使用 `StateHolder::withValueIndex` 而非 `with()`，因为属性类型在反序列化时未知
    - `setTileEntityNBT` 仅当玩家有 OP 权限或 `onlyOpsCanSetNbt()` 返回 false 时才写入方块实体 NBT
    - GameMasterBlockItem 在 `getStateForPlacement` 中做权限检查，而非 `canPlace`，因为 `canPlace` 是非虚方法
    - GameMasterBlockItem 在 player 为 nullptr 时（如发射器放置）允许放置，与 MC Java 行为一致 -
    **告示牌 / 悬挂告示牌注册**：告示牌物品在 `Items::_registerSigns()` 中通过 `WallOrFloorItem` 注册（同时注册站立 +
    墙壁变体），`BlockItemRegistry` 通过 `registerWallSign`/`registerWallHangingSign` lambda
        建立方块→物品映射。注册顺序必须是 `Items::initialize()` → `BlockItemRegistry::
            initializeVanillaBlockItems()`，否则映射会因找不到已注册物品而跳过
    -
    **`registerSimpleBlock` 与已注册物品**：如果某方块已通过 `Items::
         initialize()` 中的 `registerBlockBackedItem` 注册了物品（如按钮、压力板、门、栅栏等），`registerSimpleBlock` 会检测到已有物品并复用之，不会重复注册
