# 游戏事件系统 (GameEvent System)

Minecraft 1.17+ 引入的游戏事件系统，用于服务端内部事件分发。
与 `WorldEvents`（世界事件/levelEvent，用于广播音效和粒子给客户端）不同，
GameEvent 是服务端内部事件机制，主要用于幽匿感测体（SculkSensor）和幽匿尖啸体（SculkShrieker）
等方块检测振动信号。

## 目录结构

```text
gameevent/
├── GameEvent.hpp      # 游戏事件类定义（含 Context 内部类）
├── GameEvents.hpp     # 所有原版游戏事件常量定义
└── README.md
```

## 核心类

### GameEvent

游戏事件定义，包含：
- `id`: 事件标识符（如 "block_activate"）
- `notificationRadius`: 通知半径（格），默认 16，少数事件有特殊值

### GameEvent::Context

游戏事件上下文，携带：
- `sourceEntity`: 触发事件的实体（可空）
- `affectedState`: 受影响的方块状态（可空）

静态工厂方法：
- `Context::of(const Entity*)` - 仅传入实体
- `Context::of(const BlockState*)` - 仅传入方块状态
- `Context::of(const Entity*, const BlockState*)` - 同时传入

## 事件列表

| 事件 | 通知半径 | 用途 |
|------|---------|------|
| BLOCK_ACTIVATE | 16 | 方块激活（拉杆、按钮等） |
| BLOCK_ATTACH | 16 | 方块附着 |
| BLOCK_CHANGE | 16 | 方块变化（炼药锅水位等） |
| BLOCK_CLOSE | 16 | 方块关闭 |
| BLOCK_DEACTIVATE | 16 | 方块失活 |
| BLOCK_DESTROY | 16 | 方块销毁 |
| BLOCK_DETACH | 16 | 方块脱离 |
| BLOCK_OPEN | 16 | 方块打开 |
| BLOCK_PLACE | 16 | 方块放置 |
| CONTAINER_CLOSE | 16 | 容器关闭 |
| CONTAINER_OPEN | 16 | 容器打开 |
| DRINK | 16 | 饮用 |
| EAT | 16 | 进食 |
| ELYTRA_GLIDE | 16 | 鞘翅滑翔 |
| ENTITY_DAMAGE | 16 | 实体受伤 |
| ENTITY_DIE | 16 | 实体死亡 |
| ENTITY_DISMOUNT | 16 | 下坐骑 |
| ENTITY_INTERACT | 16 | 实体交互 |
| ENTITY_MOUNT | 16 | 上坐骑 |
| ENTITY_PLACE | 16 | 实体放置 |
| ENTITY_ACTION | 16 | 实体动作 |
| EQUIP | 16 | 装备更换 |
| UNEQUIP | 16 | 卸下装备 |
| EXPLODE | 16 | 爆炸 |
| FLAP | 16 | 振翅 |
| FLUID_PICKUP | 16 | 流体拾取 |
| FLUID_PLACE | 16 | 流体放置 |
| HIT_GROUND | 16 | 落地 |
| INSTRUMENT_PLAY | 16 | 乐器演奏 |
| ITEM_INTERACT_FINISH | 16 | 物品交互完成 |
| ITEM_INTERACT_START | 16 | 物品交互开始 |
| **JUKEBOX_PLAY** | **10** | **唱片机播放** |
| **JUKEBOX_STOP_PLAY** | **10** | **唱片机停止** |
| LIGHTNING_STRIKE | 16 | 闪电击中 |
| NOTE_BLOCK_PLAY | 16 | 音符盒演奏 |
| PRIME_FUSE | 16 | 引信点燃 |
| PROJECTILE_LAND | 16 | 弹射物落地 |
| PROJECTILE_SHOOT | 16 | 弹射物发射 |
| SCULK_SENSOR_TENDRILS_CLICKING | 16 | 幽匿感测体触须点击 |
| SHEAR | 16 | 剪切 |
| **SHRIEK** | **32** | **尖啸** |
| SPLASH | 16 | 溅水 |
| STEP | 16 | 行走 |
| SWIM | 16 | 游泳 |
| TELEPORT | 16 | 传送 |
| RESONATE_1~15 | 16 | 共鸣频率 1-15 |

## 与 MC 原版的对应关系

| 本项目 | MC 1.21.11 |
|--------|------------|
| `GameEvent` | `net.minecraft.world.level.gameevent.GameEvent` |
| `GameEvent::Context` | `net.minecraft.world.level.gameevent.GameEvent.Context` |
| `GameEvents` 命名空间 | `GameEvent` 中的静态常量 |
| `IWorld::gameEvent()` | `LevelAccessor.gameEvent()` |

## 设计差异

1. **注册表**: MC 使用动态注册表系统（Registry/Holder），本项目使用 `inline const` 全局常量。
2. **事件分发**: MC 使用 GameEventDispatcher + GameEventListener 系统进行分区域分发，
   本项目当前 IWorld::gameEvent() 提供基础接口，完整的分发系统（含幽匿感测体检测）
   待后续实现。
3. **Context**: MC 使用 Java record 不可变对象，本项目使用包含 const 指针的类，
   功能等价但更符合 C++ 惯例。

## 待实现

- [ ] GameEventListener 接口
- [ ] GameEventDispatcher（基于区块范围的事件分发）
- [ ] GameEventListenerRegistry / EuclideanGameEventListenerRegistry
- [ ] VibrationSystem（振动系统，幽匿感测体/尖啸体的核心）
- [ ] PositionSource / BlockPositionSource / EntityPositionSource
- [ ] DynamicGameEventListener
