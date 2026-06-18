#游戏事件系统(GameEvent System)

Minecraft 1.17 +
    引入的游戏事件系统，用于服务端内部事件分发。 与 `WorldEvents`（世界事件 /
        levelEvent，用于广播音效和粒子给客户端）不同， GameEvent
            是服务端内部事件机制，主要用于幽匿感测体（SculkSensor）和幽匿尖啸体（SculkShrieker） 等方块检测振动信号。

        ##目录结构

```text gameevent /
├── GameEvent.hpp #游戏事件类定义（含 Context）
├── GameEvents.hpp #所有原版游戏事件常量定义
├── GameEventListener.hpp #游戏事件监听器接口（含 DeliveryMode、ListenerInfo）
├── GameEventListenerRegistry.hpp #监听器注册表接口和欧几里得实现
├── GameEventListenerRegistry.cpp #注册表实现
├── GameEventDispatcher.hpp #事件分发器（基于区块范围分发）
├── GameEventDispatcher.cpp #分发器实现
├── DynamicGameEventListener.hpp #动态监听器（实体位置变化的监听器）
├── DynamicGameEventListener.cpp #动态监听器实现
├── PositionSource.hpp #位置源接口（方块 /
        实体位置）
├── PositionSource.cpp #位置源实现
├── VibrationSystem.hpp #振动系统（VibrationInfo、Selector、User、Listener、Ticker）
├── VibrationSystem.cpp #振动系统实现
└── README.md
```

        ##核心类

        ## #GameEvent

        游戏事件定义，包含： - `id`: 事件标识符（如 "block_activate"） - `notificationRadius`
    : 通知半径（格），默认 16，少数事件有特殊值

      ## #GameEvent::Context

          游戏事件上下文，携带： - `sourceEntity`: 触发事件的实体（可空） - `affectedState`
    : 受影响的方块状态（可空）

          静态工厂方法： - `Context::of(const Entity*)` -
      仅传入实体 - `Context::of(const BlockState*)` - 仅传入方块状态 - `Context::of(const Entity*, const BlockState*)` -
      同时传入

      ## #GameEventListener

      游戏事件监听器接口，方法：
      - `getListenerSource()`: 返回监听器的位置源
                               - `getListenerRadius()`: 返回检测半径
                                                        - `handleGameEvent()`: 处理接收到的游戏事件
                                                                               - `getDeliveryMode()`
    : 投递模式（Unspecified 或 ByDistance）

      投递模式：
      - `Unspecified`: 事件到达时立即投递
                       - `ByDistance`: 多个监听器按距离排序，最近的优先投递

                                       ## #GameEventListenerRegistry

                                       监听器注册表接口，管理一个区域（通常是区块段）内的监听器：
                                       - `registerListener()`: 注册监听器
                                                               - `unregisterListener()`: 注销监听器
                                                                                         - `visitInRangeListeners()`
    : 访问在事件范围内的监听器

      实现类：
      - `NoopGameEventListenerRegistry`: 空注册表单例（用于未加载区块）
                                         - `EuclideanGameEventListenerRegistry`
    : 基于欧几里得距离的注册表（按区块段存储）

      ## #GameEventDispatcher

      游戏事件分发器，核心分发逻辑： 1. 根据 `event.notificationRadius()` 计算受影响的区块段范围
      2. 遍历受影响段中的注册表
      3. 对每个在范围内的监听器调用 `handleGameEvent()` 4. `ByDistance` 模式的监听器按距离排序后依次投递

      调用链：`ServerWorld::gameEvent()` → `GameEventDispatcher::
          post()` → 遍历区块段 → `EuclideanGameEventListenerRegistry::visitInRangeListeners()` → `GameEventListener::
              handleGameEvent()`

      ## #PositionSource

      监听器位置来源接口： - `BlockPositionSource`: 固定方块位置（用于幽匿感测体等方块实体）
                                                    - `EntityPositionSource`
    : 跟随实体位置（用于监守者、悦灵等实体）

      ## #DynamicGameEventListener

      包装位置可能变化的监听器（实体监听器），自动在区块段间移动注册：
      - `add()`: 添加到世界（注册到当前段）
                 - `remove()`: 从世界中移除（从当前段注销）
                               - `move()`
    : 更新段注册（实体移动时调用）

      ## #VibrationSystem

      振动系统，幽匿感测体
      /
      尖啸体 / 监守者 /
      悦灵的核心：

      ####VibrationInfo 记录一次振动的信息：事件类型、距离、源位置、源实体

      ####VibrationSelector 同一 tick 内多个振动候选的选择器：距离近的优先，距离相同频率高的优先

      ####VibrationSystem::User 振动接收者配置接口（由具体方块实体
      /
      实体实现）： - `getListenerRadius()`: 检测半径
                                            - `getPositionSource()`: 位置源
                                                                     - `canReceiveVibration()`: 是否可以接收振动
                                                                                                - `onReceiveVibration()`
    : 振动到达回调
      - `isValidVibration()`
    : 基本振动验证

      ####VibrationSystem::Listener 实现 `GameEventListener` 接口，`DeliveryMode::ByDistance`： -
      验证振动有效性 -
      添加到 VibrationSelector 候选

      ####VibrationSystem::Ticker 每 tick 驱动振动传播： 1. 没有当前振动时，从选择器选择候选
      2. 设置传播时间 = floor(distance) tick 3. 递减传播时间，归零时调用 `onReceiveVibration()`

    ####VibrationSystem::Data 振动系统运行时状态：当前振动、选择器、传播时间

    ##事件列表

    | 事件 | 通知半径 | 频率 | 用途 | | -- -- --| -- -- -- -- -| -- -- --| -- -- --| | BLOCK_ACTIVATE | 16 |
    5 | 方块激活（拉杆、按钮等） | | BLOCK_ATTACH | 16 | 5 | 方块附着 | | BLOCK_CHANGE | 16 |
    7 | 方块变化（炼药锅水位等） | | BLOCK_CLOSE | 16 | 7 | 方块关闭 | | BLOCK_DEACTIVATE | 16 | 3 | 方块失活 |
    | BLOCK_DESTROY | 16 | 8 | 方块销毁 | | BLOCK_DETACH | 16 | 4 | 方块脱离 | | BLOCK_OPEN | 16 | 7 | 方块打开 |
    | BLOCK_PLACE | 16 | 5 | 方块放置 | | CONTAINER_CLOSE | 16 | 7 | 容器关闭 | | CONTAINER_OPEN | 16 | 7 | 容器打开 |
    | DRINK | 16 | 8 | 饮用 | | EAT | 16 | 8 | 进食 | | ELYTRA_GLIDE | 16 | -| 鞘翅滑翔 | | ENTITY_DAMAGE | 16 |
    11 | 实体受伤 | | ENTITY_DIE | 16 | 11 | 实体死亡 | | ENTITY_DISMOUNT | 16 | 6 | 下坐骑 | | ENTITY_INTERACT | 16 |
    5 | 实体交互 | | ENTITY_MOUNT | 16 | 6 | 上坐骑 | | ENTITY_PLACE | 16 | -| 实体放置 | | ENTITY_ACTION | 16 |
    4 | 实体动作 | | EQUIP | 16 | 11 | 装备更换 | | UNEQUIP | 16 | 11 | 卸下装备 | | EXPLODE | 16 | 14 | 爆炸 | | FLAP |
    16 | 1 | 振翅 | | FLUID_PICKUP | 16 | 4 | 流体拾取 | | FLUID_PLACE | 16 | 5 | 流体放置 | | HIT_GROUND | 16 |
    12 | 落地 | | INSTRUMENT_PLAY | 16 | 10 | 乐器演奏 | | ITEM_INTERACT_FINISH | 16 | 13 | 物品交互完成 |
    | ITEM_INTERACT_START | 16 | 13 | 物品交互开始 | | **JUKEBOX_PLAY * *| **10 * *| **13 * *| **唱片机播放 * *| |
    **JUKEBOX_STOP_PLAY * *| **10 * *| **13 * *| **唱片机停止 * *| | LIGHTNING_STRIKE | 16 | 14 | 闪电击中 |
    | NOTE_BLOCK_PLAY | 16 | 10 | 音符盒演奏 | | PRIME_FUSE | 16 | 3 | 引信点燃 | | PROJECTILE_LAND | 16 |
    2 | 弹射物落地 | | PROJECTILE_SHOOT | 16 | 10 | 弹射物发射 | | SCULK_SENSOR_TENDRILS_CLICKING | 16 |
    9 | 幽匿感测体触须点击 | | SHEAR | 16 | 10 | 剪切 | | **SHRIEK * *| **32 * *| **15 * *| **尖啸 * *| | SPLASH | 16 |
    12 | 溅水 | | STEP | 16 | 1 | 行走 | | SWIM | 16 | 1 | 游泳 | | TELEPORT | 16 | -| 传送 | | RESONATE_1 ~15 | 16 |
    1 ~15 | 共鸣频率 1 - 15 |

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
    | `IWorld::gameEvent()` | `LevelAccessor.gameEvent()` | | `ServerWorld::gameEvent()` | `ServerLevel.gameEvent()` |
    | `ChunkData::getGameEventListenerRegistry()` | `LevelChunk.getListenerRegistry()` |
    | `ChunkData::getOrCreateGameEventListenerRegistry()` | `LevelChunk.getListenerRegistry()` (懒创建) |

    ##数据流

``` 游戏逻辑调用 world
            ->gameEvent(event, pos, context)
    ↓ ServerWorld::gameEvent() → GameEventDispatcher::post()
    ↓ 计算受影响的区块段范围(notificationRadius → section coord range)
    ↓ 遍历受影响段 → ChunkData::getGameEventListenerRegistry(sectionY)
    ↓ EuclideanGameEventListenerRegistry::visitInRangeListeners()
    ↓ (距离过滤 + PositionSource 解析)
    ↓ GameEventListener::handleGameEvent()
    ↓ (对于 VibrationSystem.Listener)VibrationSelector::addCandidate() → 选择最佳振动
    ↓ (每 tick 由 VibrationSystem.Ticker 处理)VibrationSystem.Ticker::tick() → 递减传播时间 → onReceiveVibration()
    ↓ 具体接收者响应（激活幽匿感测体、触发尖啸体等）
```

        ##区块段存储

        游戏事件监听器按区块段存储在 `ChunkData::m_gameEventListenerRegistries` 中。
        每个段（16x16x16）有独立的 `EuclideanGameEventListenerRegistry`，当注册表为空时 自动从映射中移除以节省内存。

        方块实体注册
        /
        注销监听器的流程： 1. 方块实体创建 → 通过 `Block::getGameEventListener()` 获取监听器
        2. 调用 `chunk.getOrCreateGameEventListenerRegistry(sectionY, world)
            .registerListener(listener)` 3. 方块实体移除 → 调用 `registry.unregisterListener(
                listener)` 4. 注册表为空时 → OnEmptyAction 回调自动从映射中移除

        ##设计差异

        1. *
        *注册表 * * : MC 使用动态注册表系统（Registry / Holder），本项目使用 `inline const` 全局常量。 2. * *事件分发 *
        * : MC 使用 `LevelChunk` 存储 `Int2ObjectMap<GameEventListenerRegistry>`， 本项目使用 `ChunkData::
                m_gameEventListenerRegistries`（`unordered_map<i32, unique_ptr<...>>`）， 功能等价但更符合 C++ 惯例。
            3. *
        *VibrationSystem *
        * : MC 使用复合接口（包含 Data、User、Listener、Ticker 内部类型）， 本项目保持相同的结构但使用 C
            ++ 的类继承和组合模式。 4. *
        *Context * * : MC 使用 Java record 不可变对象，本项目使用包含 const 指针的类， 功能等价但更符合 C++ 惯例。 5. *
        *振动频率映射 * * : MC 使用注册表查找，本项目使用字符串比较的静态函数， 功能等价但避免了注册表依赖。
