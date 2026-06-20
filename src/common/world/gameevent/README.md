#游戏事件系统(GameEvent System)

Minecraft 1.17 +
    引入的游戏事件系统，用于服务端内部事件分发。与 `WorldEvents`（世界事件 /
        levelEvent，用于广播音效和粒子给客户端）不同，GameEvent
            是服务端内部事件机制，主要用于幽匿感测体（SculkSensor）和幽匿尖啸体（SculkShrieker）等方块检测振动信号。

        ##目录结构

```text gameevent /
├── GameEvent.hpp #游戏事件类定义（含 Context）
├── GameEvents.hpp #所有原版游戏事件常量定义
├── GameEventListener.hpp #游戏事件监听器接口（含 DeliveryMode、ListenerInfo）
├── GameEventListenerRegistry.hpp #监听器注册表接口
├── GameEventListenerRegistry.cpp #EuclideanGameEventListenerRegistry 实现
├── GameEventDispatcher.hpp #事件分发器（基于区块范围分发）
├── GameEventDispatcher.cpp #分发器实现
├── DynamicGameEventListener.hpp #动态监听器（实体位置变化的监听器）
├── DynamicGameEventListener.cpp #动态监听器实现
├── PositionSource.hpp #位置源接口（方块 /
        实体位置）
├── PositionSource.cpp #位置源实现
├── VibrationSystem.hpp #振动系统（VibrationInfo、Selector、User、Listener、Ticker）
├── VibrationSystem.cpp #振动系统通用实现（频率映射、选择器、isIgnoredBySneaking）
└── README.md
```

        ##内部模块关系

``` GameEvent +
    GameEvents ←── VibrationSystem
     ↑                       ├── Data(运行时状态)
     ↑                       ├── User(接收者配置接口) GameEventListener ←──────────├── Listener(实现 GameEventListener)
     ↑                       └── Ticker(tick 驱动) GameEventListenerRegistry ←── DynamicGameEventListener
    GameEventDispatcher ←──────── ServerWorld::gameEvent() PositionSource ←───────────── User::getPositionSource()
```

    ##上下游外部依赖关系

    - **上游（本目录被谁依赖）**：`server / world / ServerWorld`（调用 `gameEvent()`）、`server / world / gameevent
        /`（服务端实现依赖本目录头文件）、`common / world / block /`（方块实体注册监听器）
    -
    **下游（本目录依赖谁）**：`common / entity / core / Entity`（源实体检查）、`common / world / gameevent /
        GameEvent`（事件定义）

        ##容易踩的坑

        1. *
        *isIgnoredBySneaking 与 GameEvents 常量的一致性**：`isIgnoredBySneaking()` 硬编码了6个事件 ID
         字符串（`hit_ground`、`projectile_shoot`、`step`、`swim`、`item_interact_start`、`item_interact_finish`），必须与 `GameEvents
             .hpp` 中对应常量的 ID 完全一致。如果新增或修改了 GameEvent 常量的 ID 字符串，需要同步检查此函数。 2. *
        *isValidVibration 中的
        dampensVibrations 依赖**：`Entity::dampensVibrations()` 默认返回 `false`，只有 `ItemEntity`（羊毛
        /
        地毯物品）重写为检查 `ItemTags::DAMPENS_VIBRATIONS`。未来新增 WardenEntity
        时也需重写此方法返回 `true`。 3. ** BlockTags::DAMPENS_VIBRATIONS
        必须包含地毯**：该标签不仅包含16色羊毛，还包含16色地毯方块（对齐 MC 原版 `#minecraft
    : dampens_vibrations = #minecraft : wool +
                                        #minecraft
    : wool_carpets`）。如果未来新增地毯颜色，需同步更新此标签。 4. **requiresAdjacentChunksToBeTicking 的检查逻辑**：`receiveVibration()` 中调用 `areAdjacentChunksTicking(world, listenerBlockPos)` 检查监听器位置周围 3x3 区块是否全部处于 BlockTicking 级别且已加载。注意两点：（1）检查位置是监听器位置而非振动源位置；（2）检查级别是 BlockTicking（level ≤ 32）而非 EntityTicking（level ≤ 31）。检查不通过时返回 false 但不清除当前振动，下次 tick 会重试。 5. **reloadVibrationParticle 的设置时机**：`tryReloadVibrationParticle()` 依赖 `data.shouldReloadVibrationParticle()` 标志，该标志必须从存档加载时设为 `true`。MC 原版在 `Data.CODEC` 反序列化时硬编码 `reloadVibrationParticle = true`。实现 SculkSensorBlockEntity、SculkShriekerBlockEntity、WardenEntity、AllayEntity 的 NBT 加载时，应使用 `Data(currentVibration, selector, travelTime, true)` 构造函数或调用 `setReloadVibrationParticle(true)`，否则区块重新加载后不会重发振动粒子效果。

      ##核心类

      ## #GameEvent

      游戏事件定义，包含： - `id`: 事件标识符（如 `"block_activate"`） - `notificationRadius`
    : 通知半径（格），默认 16，少数事件有特殊值

      ## #GameEvent::Context

      游戏事件上下文，携带：
      - `sourceEntity`: 触发事件的实体（可空）
                        - `affectedState`
    : 受影响的方块状态（可空）

      静态工厂方法：
      - `Context::of(const Entity*)` — 仅传入实体
      - `Context::of(const BlockState*)` — 仅传入方块状态
      - `Context::of(const Entity*, const BlockState*)` — 同时传入

          ## #VibrationSystem

          振动系统，幽匿感测体
          / 尖啸体 / 监守者 /
          悦灵的核心：

          ####VibrationInfo 记录一次振动的信息：事件类型、距离、源位置、源实体

          ####VibrationSelector 同一 tick 内多个振动候选的选择器：距离近的优先，距离相同频率高的优先

          ####VibrationSystem::User 振动接收者配置接口（由具体方块实体
          / 实体实现）：
      - `getListenerRadius()`: 检测半径
                               - `getPositionSource()`: 位置源
                                                        - `canReceiveVibration()`: 是否可以接收振动
                                                                                   - `onReceiveVibration()`
    : 振动到达回调
      - `isValidVibration()`: 基本振动验证（频率、旁观者、潜行、阻尼、方块阻尼）
                              - `requiresAdjacentChunksToBeTicking()`: 是否需要相邻区块在 tick（幽匿感测体返回 true）
                                                                       - `canTriggerAvoidVibration()`
    : 是否可触发规避振动成就（默认 false）
      - `onDataChanged()`: 数据变化回调

                           ####VibrationSystem::Listener 实现 `GameEventListener` 接口，`DeliveryMode::ByDistance`： -
                           验证振动有效性 -
                           添加到 VibrationSelector 候选

                           ####VibrationSystem::Ticker 每 tick 驱动振动传播： 1. `tryReloadVibrationParticle()` — 区块重载后重发振动粒子
                           2. 没有当前振动时，从选择器选择候选
                           3. 设置传播时间 = floor(distance) tick，在振动源位置发送 `ParticleTypeId::Vibration` 粒子
                           4. 递减传播时间，归零时调用 `onReceiveVibration()`

                                             ####VibrationSystem::Data 振动系统运行时状态：当前振动、选择器、传播时间

                                             ## #VibrationSystem::isValidVibration 检查流程

                                             服务端 `VibrationSystemServer.cpp` 中的实现，按顺序执行以下检查：

                                             1. *
            *频率检查 * *：事件频率为 0 则拒绝（非振动事件） 2. * *旁观者检查 * *：源实体为旁观者模式玩家则拒绝 3. *
            *潜行忽略检查 *
            *：源实体正在潜行（`isSteppingCarefully()`）且事件可被潜行忽略（`isIgnoredBySneaking()`）则拒绝。若 `canTriggerAvoidVibration()` 返回 true 且源实体为玩家，则触发 `AvoidVibrationTrigger`（`minecraft:avoid_vibration`）进度 4. *
            *实体阻尼检查 * *：源实体 `dampensVibrations()` 返回 true 则拒绝（如监守者、羊毛物品实体） 5. *
            *方块阻尼检查 * *：受影响方块属于 `BlockTags::DAMPENS_VIBRATIONS`（羊毛 /
            地毯）则拒绝

            ## #VibrationSystem::isIgnoredBySneaking

            静态方法，判断事件是否可被潜行忽略。对齐 MC 原版 `GameEventTags
                .IGNORE_VIBRATIONS_SNEAKING`，包含以下6个事件：
        - `hit_ground` — 落地 - `projectile_shoot` — 弹射物发射 - `step` — 行走 - `swim` — 游泳
        - `item_interact_start` — 物品交互开始 - `item_interact_finish` — 物品交互完成

        当源实体正在潜行时，这6个事件不会触发振动信号。其他事件（如 BLOCK_PLACE、ENTITY_DAMAGE 等）不受潜行影响。

        ##事件列表

    | 事件 | 通知半径 | 频率 | 用途 | | -- -- --| -- -- -- -- --| -- -- --| -- -- --| | BLOCK_ACTIVATE | 16 |
    5 | 方块激活（拉杆、按钮等） | | BLOCK_ATTACH | 16 | 5 | 方块附着 | | BLOCK_CHANGE | 16 |
    7 | 方块变化（炼药锅水位等） | | BLOCK_CLOSE | 16 | 7 | 方块关闭 | | BLOCK_DEACTIVATE | 16 | 3 | 方块失活 |
    | BLOCK_DESTROY | 16 | 8 | 方块销毁 | | BLOCK_DETACH | 16 | 4 | 方块脱离 | | BLOCK_OPEN | 16 | 7 | 方块打开 |
    | BLOCK_PLACE | 16 | 5 | 方块放置 | | CONTAINER_CLOSE | 16 | 7 | 容器关闭 | | CONTAINER_OPEN | 16 | 7 | 容器打开 |
    | DRINK | 16 | 8 | 饮用 | | EAT | 16 | 8 | 进食 | | ELYTRA_GLIDE | 16 | 1 | 鞘翅滑翔 | | ENTITY_DAMAGE | 16 |
    11 | 实体受伤 | | ENTITY_DIE | 16 | 11 | 实体死亡 | | ENTITY_DISMOUNT | 16 | 6 | 下坐骑 | | ENTITY_INTERACT | 16 |
    5 | 实体交互 | | ENTITY_MOUNT | 16 | 6 | 上坐骑 | | ENTITY_PLACE | 16 | 5 | 实体放置 | | ENTITY_ACTION | 16 |
    4 | 实体动作 | | EQUIP | 16 | 11 | 装备更换 | | UNEQUIP | 16 | 11 | 卸下装备 | | EXPLODE | 16 | 14 | 爆炸 | | FLAP |
    16 | 1 | 振翅 | | FLUID_PICKUP | 16 | 4 | 流体拾取 | | FLUID_PLACE | 16 | 5 | 流体放置 | | HIT_GROUND | 16 |
    12 | 落地 | | INSTRUMENT_PLAY | 16 | 10 | 乐器演奏 | | ITEM_INTERACT_FINISH | 16 | 13 | 物品交互完成 |
    | ITEM_INTERACT_START | 16 | 13 | 物品交互开始 | | JUKEBOX_PLAY | 10 | 13 | 唱片机播放 | | JUKEBOX_STOP_PLAY | 10 |
    13 | 唱片机停止 | | LIGHTNING_STRIKE | 16 | 14 | 闪电击中 | | NOTE_BLOCK_PLAY | 16 | 10 | 音符盒演奏 | | PRIME_FUSE
    | 16 | 3 | 引信点燃 | | PROJECTILE_LAND | 16 | 2 | 弹射物落地 | | PROJECTILE_SHOOT | 16 | 10 | 弹射物发射 |
    | SCULK_SENSOR_TENDRILS_CLICKING | 16 | 9 | 幽匿感测体触须点击 | | SHEAR | 16 | 10 | 剪切 | | SHRIEK | 32 |
    15 | 尖啸 | | SPLASH | 16 | 12 | 溅水 | | STEP | 16 | 1 | 行走 | | SWIM | 16 | 1 | 游泳 | | TELEPORT | 16 | 1 | 传送
    | | RESONATE_1 ~15 | 16 | 1 ~15 | 共鸣频率 1 - 15 |

    ##与 MC 原版的对应关系

    | 本项目 | MC 1.21.11 | | -- -- -- --| -- -- -- -- -- --|
    | `GameEvent` | `net.minecraft.world.level.gameevent.GameEvent` |
    | `GameEvent::Context` | `net.minecraft.world.level.gameevent.GameEvent.Context` | | `GameEvents` 命名空间
    | `GameEvent` 中的静态常量 | | `GameEventListener` | `net.minecraft.world.level.gameevent.GameEventListener` |
    | `GameEventListenerRegistry` | `net.minecraft.world.level.gameevent.GameEventListenerRegistry` |
    | `EuclideanGameEventListenerRegistry` | `net.minecraft.world.level.gameevent.EuclideanGameEventListenerRegistry` |
    | `GameEventDispatcher` | `net.minecraft.world.level.gameevent.GameEventDispatcher` |
    | `DynamicGameEventListener` | `net.minecraft.world.level.gameevent.DynamicGameEventListener` |
    | `PositionSource` | `net.minecraft.world.level.gameevent.PositionSource` |
    | `BlockPositionSource` | `net.minecraft.world.level.gameevent.BlockPositionSource` |
    | `EntityPositionSource` | `net.minecraft.world.level.gameevent.EntityPositionSource` |
    | `VibrationSystem` | `net.minecraft.world.level.gameevent.vibrations.VibrationSystem` |
    | `VibrationInfo` | `net.minecraft.world.level.gameevent.vibrations.VibrationInfo` |
    | `VibrationSelector` | `net.minecraft.world.level.gameevent.vibrations.VibrationSelector` |
    | `isIgnoredBySneaking()` | `GameEventTags.IGNORE_VIBRATIONS_SNEAKING` 标签 |
    | `Entity::dampensVibrations()` | `Entity.dampensVibrations()` |
    | `User::canTriggerAvoidVibration()` + `AvoidVibrationTrigger` | `CriteriaTriggers.AVOID_VIBRATION` |
    | `Ticker::tryReloadVibrationParticle()` | `VibrationSystem.Ticker.reloadVibrationParticle()` |
    | `ServerWorld::addParticle(ParticleTypeId::Vibration)` | `ServerLevel.sendParticles(VibrationParticleOption)` |
    | `IWorld::gameEvent()` | `LevelAccessor.gameEvent()` | | `ServerWorld::gameEvent()` | `ServerLevel.gameEvent()` |

    ##数据流

``` 游戏逻辑调用 world
            ->gameEvent(event, pos, context)
  → ServerWorld::gameEvent() → GameEventDispatcher::post()
    → 计算受影响的区块段范围(notificationRadius → section coord range)
    → 遍历受影响段 → ChunkData::getGameEventListenerRegistry(sectionY)
    → EuclideanGameEventListenerRegistry::visitInRangeListeners()
      → (距离过滤 + PositionSource 解析)
    → GameEventListener::handleGameEvent()
      → (对于 VibrationSystem.Listener)isValidVibration() 验证
        → 频率检查 → 旁观者检查 → 潜行忽略检查（+ AvoidVibrationTrigger 进度触发）→ 实体阻尼检查 → 方块阻尼检查
      → VibrationSelector::addCandidate() → 选择最佳振动
      → (每 tick 由 VibrationSystem.Ticker 处理)
        → VibrationSystem.Ticker::tick() → tryReloadVibrationParticle() → 递减传播时间 → receiveVibration()
          → requiresAdjacentChunksToBeTicking 区块级别检查
          → onReceiveVibration() 具体接收者响应
```

        ##区块段存储

        游戏事件监听器按区块段存储在 `ChunkData::m_gameEventListenerRegistries` 中。
        每个段（16x16x16）有独立的 `EuclideanGameEventListenerRegistry`，当注册表为空时自动从映射中移除以节省内存。

        方块实体注册
        /
        注销监听器的流程： 1. 方块实体创建 → 通过 `Block::getGameEventListener()` 获取监听器
        2. 调用 `chunk.getOrCreateGameEventListenerRegistry(sectionY, world)
            .registerListener(listener)` 3. 方块实体移除 → 调用 `registry.unregisterListener(
                listener)` 4. 注册表为空时 → OnEmptyAction 回调自动从映射中移除

        ##设计差异

        1. *
        *注册表 * *：MC 使用动态注册表系统（Registry / Holder），本项目使用 `inline const` 全局常量。 2. * *事件分发 *
        *：MC 使用 `LevelChunk` 存储 `Int2ObjectMap<GameEventListenerRegistry>`，本项目使用 `ChunkData::
            m_gameEventListenerRegistries`（`unordered_map<i32, unique_ptr<...>>`），功能等价但更符合 C++ 惯例。 3. *
        *VibrationSystem *
        *：MC 使用复合接口（包含 Data、User、Listener、Ticker 内部类型），本项目保持相同的结构但使用 C
        ++ 的类继承和组合模式。 4. *
        *Context * *：MC 使用 Java record 不可变对象，本项目使用包含 const 指针的类，功能等价但更符合 C++ 惯例。 5. *
        *振动频率映射 * *：MC 使用注册表查找，本项目使用字符串比较的静态函数，功能等价但避免了注册表依赖。 6. *
        *GameEventTags *
        *：MC 使用数据驱动的标签系统（`GameEventTags
             .IGNORE_VIBRATIONS_SNEAKING` 等），本项目通过 `isIgnoredBySneaking()` 硬编码实现，因为项目中的 GameEvent
         使用 `const char *` ID 而非注册表对象，建立标签系统的收益不足以抵消复杂度。
