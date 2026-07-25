#交互类方块实体(Interactive Block Entities)

交互类方块实体，提供玩家交互功能（告示牌编辑、唱片播放、附魔等）和机械功能（活塞、发射器等）。

##目录结构

``` interactive /
├── BannerEntity.hpp / cpp #旗帜方块实体（图案存储、最多6层）
├── BannerPattern.hpp / cpp #旗帜图案类型定义
├── DecoratedPotPattern.hpp / cpp #饰纹陶罐图案类型定义（24种图案：Blank+20考古学+3试炼密室）
├── DecoratedPotBlockEntity.hpp / cpp #饰纹陶罐方块实体（四面图案PotDecorations、1格物品容器、摇晃动画、比较器信号）
├── BeehiveBlockEntity.hpp / cpp #蜂巢方块实体（蜜蜂存储、蜂蜜等级管理）
├── BellBlockEntity.hpp / cpp #钟方块实体（摇晃动画、共振机制、灾厄村民发光、村民HEARD_BELL_TIME记忆）
├── DispenserBlockEntity.hpp / cpp #发射器方块实体基类（9格存储、战利品表填充）
├── DropperBlockEntity.hpp / cpp #投掷器方块实体（继承DispenserBlockEntity）
├── EnchantingTableEntity.hpp / cpp #附魔台方块实体（附魔力量计算、书本动画）
├── EndGatewayEntity.hpp / cpp #末地折跃门方块实体（传送逻辑、两套冷却时间）
├── JukeboxEntity.hpp / cpp #唱片机方块实体（唱片播放、比较器信号、1槽位，通过MusicDiscItem获取信号强度）
├── LecternEntity.hpp / cpp #讲台方块实体（书本展示、翻页红石脉冲、比较器信号）
├── PistonBlockEntity.hpp / cpp #活塞方块实体（方块移动动画）
├── ShelfBlockEntity.hpp / cpp #书架方块实体（3槽位物品存储、比较器3位二进制信号、侧链连接）
├── CopperGolemStatueBlockEntity.hpp / cpp #铜傀儡雕像方块实体（CUSTOM_NAME 存储、removeStatue 生成铜傀儡实体）
├── BrushableBlockEntity.hpp / cpp #可刷方块实体（可疑沙/可疑沙砾，考古战利品表、刷扫计数与冷却、DUSTED状态、完成后转换为普通方块）
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

        `_generateExitPortal()` 的区块扫描算法对齐 MC Java 的 `TheEndGatewayBlockEntity.findExitPortalXZPosTentative`：从主岛沿方向向量前进 1024 格后，先回退跳过非空区块（最多 16 次），再前进跳过空区块（最多 16 次），每次步进一个区块宽度（16 格）。扫描通过 `IWorld::getOrLoadChunk()` 同步加载目标区块，再用私有静态助手 `_isChunkEmpty(const ChunkData*)` 判空——遍历全部 `CHUNK_SECTIONS` 个区段，存在非空区段即视为非空区块，nullptr 区块视为空区块。这对应 MC Java 的 `isChunkEmpty` + `getHighestFilledSectionIndex() == -1` 语义。出口位置 Y 坐标固定为 75，再由 `_findHighestBlock` 向上 10 格放置折跃门结构。

        ## #6. SignEntity 的涂蜡状态与编辑者追踪

`SignEntity` 的 `isWaxed` 属性用于保护告示牌文字不被修改。涂蜡后 `setLine`、`setLines`、`clearLines`、`setLineFromLegacy` 均被拒绝。涂蜡交互由 `AbstractSignBlock::onBlockActivated()` 中检测蜜脾手持物品触发，同时 `HoneycombItem::onItemUse()` 也实现了告示牌涂蜡路径。`setWaxed()` 仅在状态变化时返回 true 并标记 dirty。NBT 序列化中布尔值以 `i8` 存储。

`SignEntity` 还实现了编辑者追踪机制（对应 MC Java 的 `SignBlockEntity.playerWhoMayEdit`）：
- `m_playerWhoMayEdit`：存储当前允许编辑的玩家 UUID，空字符串表示无编辑者
- `otherPlayerIsEditing()`：检查是否有其他玩家正在编辑，用于 `AbstractSignBlock::onBlockActivated()` 中阻止涂蜡和编辑
- `setAllowedPlayerEditor()` / `clearAllowedPlayerEditor()`：设置/清除编辑锁
- `hasEditableText()`：检查告示牌文本是否可编辑（涂蜡返回 false），用于 `AbstractSignBlock::onBlockActivated()` 中决定是否打开编辑器
- `playerIsTooFarAwayToEdit()`：检查编辑者是否距离过远或已离线（MC Java 的 `isWithinBlockInteractionRange(blockPos, 4.0)`）
- `tick()` / `needsTick()`：当有编辑者时启用 tick，每 tick 检查编辑者是否超出范围，自动清除编辑锁
- 编辑者状态是运行时瞬态数据，不持久化到存档（与 MC Java 一致）
- UUID 字符串比较而非 `Player*` 指针，避免悬垂指针风险

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

## #14. BellBlockEntity 摇晃/共振状态机与客户端-服务端职责合并

BellBlockEntity 将 MC Java 的 `serverTick` 与 `clientTick` 合并为单一 `tick(IWorld&)`，根据 `world.isClientSide()` 在共振到期时分支：服务端调用 `_makeRaidersGlow` 对 48 格内 RAIDERS 施加 60 tick 发光效果，客户端调用 `_showBellParticles` 发射粒子。

关键常量（与 MC 1.21.11 BellBlockEntity.java 对齐）：
- `DURATION = 50`：摇晃动画持续 tick
- `TICKS_BEFORE_RESONATION = 5`：摇晃开始后多久开始检测共振
- `MAX_RESONATION_TICKS = 40`：共振持续 tick
- `GLOW_DURATION = 60`：发光效果持续 tick
- `MIN_TICKS_BETWEEN_SEARCHES = 60`：两次实体搜索最小间隔（节流）
- `SEARCH_RADIUS = 48.0f`：实体搜索 AABB 半径
- `HEAR_BELL_RADIUS = 32.0f`：听到钟声的半径（村民记忆写入 + 灾厄村民检测）
- `HIGHLIGHT_RAIDERS_RADIUS = 48.0f`：发光效果施加半径

敲击触发流程：
1. `BellBlock::attemptToRing` 调用 `BellBlockEntity::onHit(world, direction)`
2. `onHit` 设置敲击方向、启动摇晃，并通过 `world.blockEvent(pos, block, 1, dir.get3DDataValue())` 同步到客户端
3. 服务端与客户端的 `triggerEvent(1, type)` 被调用：重新搜索附近实体（`_updateEntities`）、重置共振、设置敲击方向、启动摇晃
4. `tick` 推进摇晃计时；`ticks >= 5` 且 `resonationTicks == 0` 且 `_areRaidersNearby` 时进入共振（播放 BELL_RESONATE 音效）
5. 共振到期（`resonationTicks >= 40`）：服务端施加发光，客户端发射粒子

## #15. BellBlockEntity 实体搜索节流与村民记忆写入

`_updateEntities(IWorld&)` 在以下条件重新搜索 48 格 AABB 内的 LivingEntity：
- 距离上次搜索超过 `MIN_TICKS_BETWEEN_SEARCHES = 60` tick
- 或 `m_nearbyEntities` 为空（首次或加载后）

搜索后，仅服务端对 32 格内的村民（`VillagerEntity`）写入 `HEARD_BELL_TIME` 记忆（值为当前 `getGameTime()`），触发村民"躲藏"行为。村民通过 `dynamic_cast` 筛选，非村民实体忽略。

## #16. BellBlockEntity 粒子系统（EntityEffect 带颜色粒子）

`_showBellParticles` 对应 MC 原版的 `showBellParticles`，通过 `IWorld::addEntityEffectParticle` 发射带 ARGB 颜色的 `EntityEffect` 粒子，对应 MC Java 的 `ColorParticleOption.create(ParticleTypes.ENTITY_EFFECT, color)`。

### 颜色递增序列
- 初始颜色计数器：`16700985`
- 每次发射粒子前：`colorCounter += 5`（对应 MC `MutableInt.addAndGet(5)`）
- 颜色通过 `EntityEffectParticleData`（仅 ARGB，无 scale）携带

### 粒子数量公式
`particleCount = clamp((nearbyCount - 21) / -2, 3, 15)`，其中 `nearbyCount` 是 48 格内的所有附近实体数量（而非仅灾厄村民）。

### 数据管线
1. **客户端**：`IWorld::addEntityEffectParticle` → `ClientWorld::addEntityEffectParticle` → 创建 `EntityEffectParticleData` → `ParticleManager::addPendingParticle` → 数据工厂 `EntityEffectParticle::createWithColor`
2. **服务端**（虽 `_showBellParticles` 仅在客户端调用，但 `addEntityEffectParticle` 接口可被其他场景复用）：`ServerWorld::addEntityEffectParticle` → `m_onBroadcastEntityEffectParticle` 回调 → `MinecraftServer::broadcastEntityEffectParticleInRange` → 广播 `ir::play::LevelParticles` 发送给范围内玩家
3. **客户端接收**：`ClientPlayVisitor` 处理 `ir::play::LevelParticles` → `onEntityEffectParticle` 回调 → `ClientApplicationNetwork` 创建 `EntityEffectParticleData` → `ParticleManager::addPendingParticle`

## #17. CopperGolemStatueBlockEntity 雕像复活与身体/头部朝向同步

`removeStatue(const BlockState& state)` 将铜傀儡雕像复活为铜傀儡实体，对应 MC 1.21.11 `CopperGolemStatueBlockEntity.removeStatue` + `initCopperGolem`。

### 朝向同步流程

1. 从方块状态的 `HORIZONTAL_FACING` 属性计算 yaw（South=0, West=90, North=180, East=270，对应 MC `Direction.toYRot()`）
2. 调用 `entity->setPosition(center.x, y, center.z)` 设置位置（对应 MC `snapTo` 的坐标部分）
3. 调用 `entity->setRotation(yaw, 0.0f)` 设置 yaw/pitch（对应 MC `snapTo` 的朝向部分）
4. 调用 `entity->setYBodyRot(yaw)` / `entity->setYHeadRot(yaw)` 同步身体与头部朝向到 FACING 方向（对应 MC `initCopperGolem` 中的 `yHeadRot = getYRot()` / `yBodyRot = getYRot()`）

`Entity` 基类的 `setYBodyRot` / `setYHeadRot` 为空实现，`LivingEntity`（含 `CopperGolemEntity`）重写后写入 `m_renderYawOffset` / `m_rotationYawHead` 字段，因此对任意实体类型调用都安全。详见 `src/common/entity/core/README.md` 中 "setYBodyRot / setYHeadRot 虚方法" 章节。

### 其他初始化

- 转移 `CUSTOM_NAME`（对应 MC `coppergolem.setCustomName(this.components().get(DataComponents.CUSTOM_NAME))`）
- 调用 `CopperGolemEntity::spawnFromStatue(Unaffected)` 设置初始氧化等级并播放生成音效（对应 MC `playSpawnSound()`）

## #18. BrushableBlockEntity 刷扫机制与考古战利品表

`BrushableBlockEntity` 对应 MC 1.21.11 `net.minecraft.world.level.block.entity.BrushableBlockEntity`，为可疑沙（`minecraft:suspicious_sand`）/可疑沙砾（`minecraft:suspicious_gravel`）方块提供考古刷扫功能。

### 核心状态

| 成员 | 类型 | 说明 |
| -- -- --| -- -- --| -- -- --|
| `m_brushCount` | `i32` | 累计刷扫次数，达到 `REQUIRED_BRUSHES_TO_BREAK (10)` 时完成 |
| `m_coolDownEndsAtTick` | `i64` | 刷扫冷却结束 tick，每次成功刷扫后设为 `gameTime + 10` |
| `m_brushCountResetsAtTick` | `i64` | 刷扫计数重置 tick，每次 `brush()` 设为 `gameTime + 40` |
| `m_hitDirection` | `optional<Direction>` | 首次刷扫命中方向，用于物品掉落位置偏移 |
| `m_item` | `ItemStack` | 缓存的考古物品（`unpackLootTable()` 一次性生成） |
| `m_lootTable` / `m_lootTableSeed` | `ResourceLocation` / `i64` | 考古战利品表引用与种子 |

### 刷扫流程（`brush()`）

1. 首次调用记录 `hitDirection`
2. 更新 `brushCountResetsAtTick = gameTime + 40`（每次调用都更新）
3. 冷却期内（`gameTime < coolDownEndsAtTick`）返回 false
4. 设置 `coolDownEndsAtTick = gameTime + 10`
5. 调用 `unpackLootTable()` 一次性生成物品
6. `++brushCount`，若 `>= 10` 调用 `brushingCompleted()` 返回 true
7. 否则调度 2 tick 后的方块 tick（用于 `checkReset` 与下落检测）
8. 按需更新 DUSTED 方块状态（0-3）

### DUSTED 完成度映射（`getCompletionState()`）

- 0：`brushCount == 0`
- 1：`brushCount < 3`
- 2：`brushCount < 6`
- 3：`brushCount >= 6`

### 完成处理（`brushingCompleted()`）

1. 调用 `dropContent()` 掉落物品（在命中方向相邻位置生成 ItemEntity，数量为 `split(nextInt(21) + 10)`）
2. 触发 `BRUSH_BLOCK_COMPLETE (3008)` 世界事件
3. 将方块替换为 `BrushableBlock::getTurnsInto()` 的默认状态（可疑沙→沙，可疑沙砾→沙砾）

### 计数重置（`checkReset()`）

由 `BrushableBlock::tick()`（方块计划刻）调用：
- 当 `brushCount != 0` 且 `gameTime >= brushCountResetsAtTick` 时，`brushCount = max(0, brushCount - 2)`，更新 DUSTED，设置 `brushCountResetsAtTick = gameTime + 4`
- `brushCount == 0` 时完全重置（清空 `hitDirection` 与计时器）

### 战利品表生成（`unpackLootTable()`）

使用 `LootParameterSets::archaeology()` 参数集，设置 `BLOCK_POS` / `THIS_ENTITY` / `TOOL` / `BLOCK_ENTITY` 参数，取生成列表的第一个物品。生成后清空 `m_hasLootTable` 标记防止重复生成。

### 序列化

- JSON（区块存档）：`LootTable` / `LootTableSeed` / `item` / `hit_direction` / `brush_count` / `brush_count_resets_at_tick` / `cooldown_ends_at_tick`
- NBT（结构模板 / 客户端同步）：同上键名

### 集成点

- `BrushableBlock`（`world/block/blocks/functional/TrailsBlocks.hpp`）：重写 `hasBlockEntity()` / `createBlockEntity()` / `tick()`，`tick()` 中调用 `checkReset()` 后委托 `FallingBlock::tick()` 执行下落检测
- `BrushItem`（`item/items/special/BrushItem.cpp`）：`onUseTick()` 中调用 `brush()`，完成时调用 `LivingEntity::hurtAndBreak(stack, 1, ...)` 消耗耐久
- `DesertPyramidStructure`（`world/gen/structure/structures/DesertPyramidStructure.cpp`）：在宝藏室地板放置可疑沙并调用 `setLootTable("minecraft:archaeology/desert_pyramid", seed)` 挂载考古战利品表

