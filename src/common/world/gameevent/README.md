# 游戏事件系统 (GameEvent System)

服务端内部的游戏事件分发机制。与 `WorldEvents`（世界事件 / levelEvent，用于向客户端广播音效与粒子）
不同，GameEvent 不发送网络包，而是把事件分发给附近的 `GameEventListener`（如幽匿感测体、
幽匿尖啸体、监守者、悦灵），驱动振动感知。

## 目录结构

```
src/common/world/gameevent/
├── GameEvent.hpp                  # 游戏事件定义（id + 通知半径）与 GameEvent::Context（源实体 / 受影响方块状态）
├── GameEvents.hpp                 # 所有原版游戏事件常量 + getGameEventById() 反序列化查找
├── GameEvents.cpp                 # getGameEventById() 实现（延迟初始化的 unordered_map 查找表）
├── GameEventTag.hpp               # 游戏事件标签（成员为 const GameEvent*）
├── GameEventTags.hpp/cpp          # 游戏事件标签注册表（内置默认值 + getTag/registerTag/forEachTag）
├── GameEventTagLoader.hpp/cpp     # 数据包标签加载器（data/<ns>/tags/game_event/）
├── GameEventListener.hpp          # 监听器接口（DeliveryMode、ListenerInfo）
├── GameEventListenerRegistry.hpp  # 监听器注册表接口
├── DynamicGameEventListener.hpp   # 动态监听器（实体位置变化时重注册）
├── PositionSource.hpp             # 位置源接口（方块 / 实体位置）
├── VibrationSystem.hpp            # 振动系统（Data / Selector / User / Listener / Ticker）
├── VibrationSystem.cpp            # 振动系统通用实现（频率映射、选择器、isIgnoredBySneaking）
└── README.md
```

服务端实现（依赖 `ServerWorld` / 区块存储）位于 `src/server/world/gameevent/`：
`GameEventListenerRegistry.cpp`（Euclidean 实现）、`GameEventDispatcher.cpp`（按区块段范围分发）、
`DynamicGameEventListener.cpp`、`PositionSource.cpp`、`VibrationSystemServer.cpp`（`isValidVibration`
与需要世界上下文的 Ticker/Listener）。

## 内部模块关系

```
GameEvents ──▶ VibrationSystem ──▶ GameEventListener ──▶ GameEventListenerRegistry
   ▲                │                      ▲                      ▲
   │                ├── Data（运行时状态）   │                      │
   │                ├── User（接收者配置）    │                      │
   │                └── Ticker（tick 驱动）  │                      │
GameEventDispatcher ◀── ServerWorld::gameEvent()          DynamicGameEventListener
   │                                                              │
   └── PositionSource ◀── User::getPositionSource() ──────────────┘

GameEventTags ◀── VibrationSystem::isIgnoredBySneaking()
      ▲
GameEventTagLoader（数据包覆盖）
```

## 上下游外部依赖关系

**上游（谁依赖本目录）**：
- `common/entity/*`（各类实体在行为中 `world->gameEvent(...)` 发布事件）
- `common/world/block/blocks/*`（方块开合 / 变化 / 放置等发布事件）
- `common/item/items/*`、`common/sound/jukebox/*`（物品交互、唱片机事件）
- `server/world/ServerWorld`（`gameEvent()` 入口）、`server/world/gameevent/`（服务端实现）

**下游（本目录依赖谁）**：
- `common/entity/core/Entity`（源实体检查、`dampensVibrations()`）
- `common/world/gameevent/GameEvent`（事件定义）
- `common/resource/tag/GenericTagLoader`（标签加载骨架）
- `common/resource/repository/DataPackRepository`（数据包访问）

## 容易踩的坑

### 1. isIgnoredBySneaking 由 GameEventTags 标签驱动

`VibrationSystem::isIgnoredBySneaking()` 查询 `GameEventTags::IGNORE_VIBRATIONS_SNEAKING()`
（成员为 `const GameEvent*`，不再做字符串比较）。内置默认值在 `GameEventTags::initialize()` 中给出，
可被数据包 `data/<ns>/tags/game_event/ignore_vibrations_sneaking.json` 覆盖。新增 / 修改 GameEvent
常量时须同步检查 `GameEventTags::initialize()` 的默认成员集合。

**依赖顺序**：`GameEventTags::initialize()` 必须早于 `GameEventTagLoader::loadFromDataPackRepository()`
（数据包在默认值之上追加 / 替换）。`RegistryBootstrap::initializeAll()` 已按此顺序接线。

**未初始化时标签为空**：任何调用 `isIgnoredBySneaking()` 的测试 / 代码，必须先确保 `GameEventTags::initialize()`
被调用，否则查询恒返回 false（曾使 `VibrationSystemTest` 的 13 个用例静默失败）。

### 2. GameEventTag 是扁平集合，不支持 #tag 嵌套

`GameEventTag` 内部为 `unordered_set<const GameEvent*>`，不支持标签引用另一个标签。数据包侧的
`#minecraft:xxx` 引用由加载器展开；但 `GameEventTags::initialize()` 里的内置默认值若需要"引用其他标签"
（如 `warden_can_listen` = `vibrations` + `shriek` + `#shrieker_can_listen`），须手动合并成员
（同 `BlockTags` 中 `lava_pool_resetting` 一类标签的合并写法）。

### 3. isValidVibration 的 dampensVibrations 依赖

`Entity::dampensVibrations()` 默认返回 `false`，只有 `ItemEntity`（羊毛 / 地毯物品）重写为检查
`ItemTags::DAMPENS_VIBRATIONS`，`WardenEntity` 重写返回 `true`。新增类似实体须重写此方法。

### 4. BlockTags::DAMPENS_VIBRATIONS 必须包含地毯

该标签 = `#wool` + `#wool_carpets`（对齐原版）。新增地毯颜色时须同步更新。

### 5. requiresAdjacentChunksToBeTicking 的检查逻辑

`receiveVibration()` 中调用 `areAdjacentChunksTicking(world, listenerBlockPos)` 检查监听器位置周围
3×3 区块是否全部处于 BlockTicking 级别（level ≤ 32，注意不是 EntityTicking 的 ≤ 31）且已加载。
检查位置是**监听器位置**而非振动源位置；不通过时返回 false 但不清除当前振动，下次 tick 重试。

### 6. reloadVibrationParticle 的设置时机

`tryReloadVibrationParticle()` 依赖 `data.shouldReloadVibrationParticle()` 标志，该标志必须从存档加载时
设为 `true`（对应原版 `Data.CODEC` 反序列化时硬编码 `reloadVibrationParticle = true`）。
实现 SculkSensorBlockEntity / SculkShriekerBlockEntity / WardenEntity / AllayEntity 的 NBT 加载时，
须使用 `Data(currentVibration, selector, travelTime, true)` 或调 `setReloadVibrationParticle(true)`，
否则区块重新加载后不会重发振动粒子。
