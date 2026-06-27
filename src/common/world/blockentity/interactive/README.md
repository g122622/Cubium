#交互类方块实体(Interactive Block Entities)

交互类方块实体，提供玩家交互功能（告示牌编辑、唱片播放、附魔等）和机械功能（活塞、发射器等）。

##目录结构

``` interactive /
├── BannerEntity.hpp / cpp #旗帜方块实体（图案存储、最多6层）
├── BannerPattern.hpp / cpp #旗帜图案类型定义
├── BeehiveBlockEntity.hpp / cpp #蜂巢方块实体（蜜蜂存储、蜂蜜等级管理）
├── DispenserBlockEntity.hpp / cpp #发射器方块实体基类（9格存储、战利品表填充）
├── DropperBlockEntity.hpp / cpp #投掷器方块实体（继承DispenserBlockEntity）
├── EnchantingTableEntity.hpp / cpp #附魔台方块实体（附魔力量计算、书本动画）
├── EndGatewayEntity.hpp / cpp #末地折跃门方块实体（传送逻辑、两套冷却时间）
├── JukeboxEntity.hpp / cpp #唱片机方块实体（唱片播放、比较器信号、1槽位，通过MusicDiscItem获取信号强度）
├── LecternEntity.hpp / cpp #讲台方块实体（书本展示、翻页红石脉冲、比较器信号）
├── PistonBlockEntity.hpp / cpp #活塞方块实体（方块移动动画）
├── ShelfBlockEntity.hpp / cpp #书架方块实体（3槽位物品存储、比较器3位二进制信号、侧链连接）
├── SignEntity.hpp /
        cpp #告示牌方块实体（富文本存储、点击事件、涂蜡保护）
└── README.md
```

        ##内部模块关系

``` BlockEntity(父模块基类)
       ↑
       ├──────────────────────┬──────────────────────┬─────────────────┐
       │                      │                      │                 │ ContainerBlockEntity EnchantingTableEntity
        PistonBlockEntity SignEntity
       ↑                      │ BannerEntity
       │                      │ JukeboxEntity LootableContainerBlockEntity   │ LecternEntity
       ↑                      │ EndGatewayEntity
       │                      │ DispenserBlockEntity ──继承──→ DropperBlockEntity
```

        DispenserBlockEntity 提供
        9 格物品存储和随机选择物品发射功能，DropperBlockEntity 继承自它，区别在于投掷器只投掷物品而无特殊行为。

        ##上下游外部依赖关系

        ## #上游依赖（谁使用了这个模块）

    - `world / block / blocks /` - 方块类（SignBlock、PistonBlock、JukeboxBlock 等）创建和访问对应的方块实体
    - `world / chunk /` - 区块加载时反序列化方块实体 - `server /` -
    服务器处理玩家交互时访问方块实体

    ## #下游依赖（这个模块依赖了谁）

    - `world / blockentity / BlockEntity.hpp` - 方块实体基类
    - `world / blockentity / ContainerBlockEntity.hpp` - 容器基类（JukeboxEntity）
    - `world / blockentity / core / LootableContainerBlockEntity.hpp` - 战利品表容器基类（DispenserBlockEntity）
    - `world / blockentity / core / SimpleInventory.hpp` - 简单背包实现
    - `util / text / ITextComponent.hpp` - 富文本组件（SignEntity） - `util / color / DyeColor.hpp` -
    染料颜色（BannerEntity）

    ##容易踩的坑

    ## #1. PistonBlockEntity 的 m_pistonState 是非拥有指针

`m_pistonState` 指向方块注册表中的稳定状态对象，绝不能在 PistonBlockEntity
    中释放它。该指针仅在活塞方块实体生命周期内有效引用。

    ## #2. DispenserBlockEntity::addItemStack 返回值

    返回的是** 槽位索引**（0 -
    8）或 -
    1（无空槽位），而非剩余物品堆。这与一般的物品添加接口不同。

        ## #3. DispenserBlockEntity::getDispenseSlot 使用储水池采样算法

        该算法确保每个非空槽位被选中的概率相等。遍历所有槽位时，以 1 /
        n 的概率替换当前选中（n 为已遍历的非空槽数）。不要误以为是简单的随机选择。

        ## #4. SignEntity 的 NBT 修改权限

`onlyOpsCanSetNbt()` 返回 true，意味着告示牌的 NBT 数据只能由 OP
        级玩家修改。点击事件中的 `RunCommand` 以玩家自身权限等级执行（上限为 2）。

        ## #5. LecternEntity 的 NBT 修改权限

`onlyOpsCanSetNbt()` 返回 true，讲台的 NBT 数据只能由 OP
        级玩家修改（MC Java 中 Lectern 属于 `OP_ONLY_CUSTOM_DATA` 集合）。

        ## #5. EndGatewayEntity 的两套冷却时间与结构生成

    - `TELEPORT_COOLDOWN = 100 tick`：传送后的冷却时间 - `TRIGGER_COOLDOWN = 40 tick`：触发后设置的冷却时间

        两者用途不同，不要混淆。

        `createGatewayStructure()` 生成 3x5x3 十字框架结构。顶/底盖（dy=±2）仅中心列为基岩，十字臂层（dy=±1）为十字形基岩框架，中心层（dy=0）为折跃门方块+空气。与 MC 原版 EndGatewayFeature.place() 一致。

        `_generateExitPortal()` 在出口折跃门方块实体上设置返回位置（`setExitPortal(m_pos, false)`），形成双向传送链。

        ## #6. SignEntity 的涂蜡状态

`SignEntity` 的 `isWaxed` 属性用于保护告示牌文字不被修改。涂蜡后 `setLine`、`setLines`、`clearLines`、`setLineFromLegacy` 均被拒绝。涂蜡交互由 `AbstractSignBlock::onBlockActivated()` 中检测蜜脾手持物品触发，同时 `HoneycombItem::onItemUse()` 也实现了告示牌涂蜡路径。`setWaxed()` 仅在状态变化时返回 true 并标记 dirty。NBT 序列化中布尔值以 `i8` 存储。

        ## #7. BannerEntity 序列化键名差异

        JSON 序列化（`load`/`save`，用于区块存档）和 NBT 序列化（`loadFromNBT`/`saveToNBT`，用于 Java
        存档和结构模板）使用不同的键名：

    | 数据 | JSON 键名 | NBT 键名 | | -- -- --| -- -- -- -- -- -| -- -- -- -- --| | 底色 | `base_color` | `Base` |
    | 图案列表 | `patterns` | `Patterns` | | 图案类型 | `pattern` | `Pattern` | | 图案颜色 | `color` | `Color` |
    | 自定义名称 | `custom_name` | `CustomName` |

    自定义名称在两种格式中均存储为 ITextComponent 的 JSON 字符串（`toJson()
        .dump()`），读取时解析失败会回退为纯文本组件。

    ## #7. BannerEntity 图案层数限制

    最大图案层数为 `MAX_PATTERNS = 6`，`addPattern()` 会检查并拒绝超过限制的添加。

        ## #7. EnchantingTableEntity 附魔力量计算

        有效书架必须满足：
        - 水平距离为2（即
    | x
    |= = 2 或 | z |= = 2，且 | x |≤2、 | z |≤2） - 垂直距离为 0 或 1 - 书架与附魔台之间（中间位置 = 附魔台 +
        (offset.x / 2,
            offset.y,
            offset.z / 2)）的方块必须可被替换（`canBeReplaced()`，对应MC的ENCHANTMENT_POWER_TRANSMITTER标签）
        -
        书架本身必须属于 `ENCHANTMENT_POWER_PROVIDER` 标签（不仅限于原版书架）

            共30个候选书架位置（MC的BOOKSHELF_OFFSETS）。每个有效书架增加1点附魔力量，最大15点。

            附魔力量在以下时机重新计算： 1. 附魔台放置时：`EnchantingTableBlock::
                onBlockAdded` 调度1tick延迟（因为方块实体尚未创建），在 `tick` 中计算
            2. 邻居方块变化时：`EnchantingTableBlock::neighborChanged` 直接触发重新计算 3. 书架放置
            / 移除时：`BookshelfBlock::onBlockAdded /
            onBlockRemoved` 主动扫描5x3x5范围通知附魔台

`isValidBookshelf()` 是公开静态方法，供 `EnchantmentContainer` 复用，避免重复实现书架检测逻辑。

            ## #8. LecternEntity 页码从 0 开始

	`getPage()` 返回的页码从 0 开始，而非 1。红石比较器信号计算为 `min(page + 1, 15)`。

	翻页时自动触发红石脉冲：`setPage()`/`nextPage()`/`prevPage()` 在页码变化时调用 `_signalPageChange()` → `LecternBlock::pulse()`，发出2tick的红石脉冲信号。从NBT加载时不会触发脉冲（`m_world` 为空时跳过）。

            ## #9. JukeboxEntity 继承 ContainerBlockEntity 而非 LootableContainerBlockEntity

            唱片机不支持战利品表填充，只有 1 个槽位存放唱片。与 DispenserBlockEntity 不同。

            ## #10. JukeboxEntity::setRecord() 需要传入 IWorld
    &

`setRecord(const ItemStack& record,
        IWorld&
            world)` 方法内部会调用 `startPlaying()`/`stopPlaying()`，这些方法需要 `IWorld` 来广播音效事件。调用方（JukeboxBlock）负责更新 `HAS_RECORD` 方块状态。

                ## #11. JukeboxEntity 歌曲自动结束和粒子效果

                JukeboxEntity 使用 JukeboxSongPlayer 管理播放状态，支持：
            - 歌曲自动结束：通过 JukeboxSongs 注册表获取歌曲长度，当 `ticksSinceSongStarted
        >= lengthInTicks + 20` 时自动停止 - 音符粒子效果：每20tick（1秒）在唱片机上方生成音符粒子 -
            游戏事件：每20tick触发 GameEvent.JUKEBOX_PLAY（已通过 IWorld::gameEvent() 接口实现），停止时触发 GameEvent
                .JUKEBOX_STOP_PLAY（当前 ServerWorld::gameEvent() 为空操作占位，待 GameEventDispatcher 实现后自动生效）
            -
            存档恢复：从存档加载时通过 `setSongWithoutPlaying()` 恢复播放进度

            ## #12. MusicDiscItem 信号强度映射

            JukeboxEntity::getComparatorSignal() 通过 `dynamic_cast<MusicDiscItem*>` 获取信号强度。如果唱片不是
            MusicDiscItem 类型（如旧版直接用 Item 基类注册的），信号强度为 0。

    ## #13. BeehiveBlockEntity 天气/夜间释放阻止

`_releaseOccupant()` 在非紧急释放时检查天气和时间条件：雨天 (`isRaining()`)、雷暴 (`isThundering()`) 或夜间 (`!isDaytime()`) 不会放出蜜蜂。紧急释放（火灾等 `BeeReleaseStatus::Emergency`）不受此限制，蜜蜂会被强制放出。当天气/夜间阻止释放时，`_releaseOccupant()` 返回 `false`，蜜蜂保留在蜂巢中等待下次重试。
