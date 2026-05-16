# 服务端事件系统 (Server Event System)

## 概述

本模块实现了服务端事件总线，用于游戏事件的通知和订阅。
事件总线是一个通用的基础设施，设计时考虑了后续迁移其他系统。

## 架构设计

### 事件总线模式

```
事件发布者                           事件总线                           事件订阅者
     │                                 │                                 │
     │  publish(event)                 │                                 │
     ├────────────────────────────────►│                                 │
     │                                 │                                 │
     │                                 │  ┌─────────────────────────────┐│
     │                                 │  │ 分发事件                    ││
     │                                 │  │ 1. 按类型查找处理器         ││
     │                                 │  │ 2. 应用过滤器               ││
     │                                 │  │ 3. 按优先级执行处理器        ││
     │                                 │  └─────────────────────────────┘│
     │                                 │                                 │
     │                                 ├─────────────────────────────────►│
     │                                 │           handler(event)        │
     │                                 │                                 │
```

### 线程安全

事件总线使用互斥锁保护内部状态，支持多线程访问。

## 目录结构

```
server/event/
├── ServerEventBus.hpp           # 事件总线（头文件，模板实现）
├── ServerEventBus.cpp           # 事件总线（单例实例）
├── README.md                    # 本文件
│
└── events/                      # 事件定义
    ├── ServerEvents.hpp         # 所有服务端事件
    ├── BlockEvents.hpp          # 方块相关事件（可选拆分）
    ├── EntityEvents.hpp         # 实体相关事件（可选拆分）
    ├── PlayerEvents.hpp         # 玩家相关事件（可选拆分）
    └── ItemEvents.hpp           # 物品相关事件（可选拆分）
```

## 使用示例

### 订阅事件

```cpp
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"

// 方式1：手动管理订阅
auto handlerId = ServerEventBus::instance().subscribe<BlockBreakEvent>(
    [](const BlockBreakEvent& e) {
        spdlog::info("Block broken at ({}, {}, {})", e.pos.x, e.pos.y, e.pos.z);
    }
);

// 取消订阅
ServerEventBus::instance().unsubscribe(handlerId);

// 方式2：RAII订阅（推荐）
{
    auto subscription = ServerEventBus::instance().makeSubscription<BlockBreakEvent>(
        [](const BlockBreakEvent& e) {
            // 处理事件
        }
    );
    // 离开作用域自动取消订阅
}
```

### 发布事件

```cpp
// 在方块破坏时发布事件
void BlockInteractionManager::handleBlockBreak(PlayerId playerId, const BlockPos& pos) {
    const BlockState* state = m_world->getBlockState(pos);
    ItemStack* tool = player->getMainHandItem();

    BlockBreakEvent event{m_world->currentTick(), playerId, pos, state, tool};
    ServerEventBus::instance().publish(event);
}
```

### 事件优先级

```cpp
// 高优先级先执行
auto highPriority = ServerEventBus::instance().subscribe<EntityDeathEvent>(
    [](const EntityDeathEvent& e) {
        // 先处理，比如记录日志
    },
    100  // 优先级
);

auto normalPriority = ServerEventBus::instance().subscribe<EntityDeathEvent>(
    [](const EntityDeathEvent& e) {
        // 后处理，比如掉落物品
    },
    0  // 默认优先级
);
```

### 事件过滤

```cpp
// 过滤器：只处理玩家的死亡事件
auto filterId = ServerEventBus::instance().addFilter([](const ServerEvent& e) {
    // 返回true继续处理，返回false阻止处理
    return true;
});

// 移除过滤器
ServerEventBus::instance().removeFilter(filterId);
```

## 已定义事件

### 方块事件

| 事件 | 说明 | 触发时机 |
|------|------|---------|
| `BlockBreakEvent` | 方块破坏 | 玩家破坏方块时 |
| `BlockPlaceEvent` | 方块放置 | 玩家放置方块时 |
| `BlockInteractEvent` | 方块交互 | 玩家与方块交互时 |
| `EnterBlockEvent` | 进入方块 | 玩家进入方块时（如传送门） |

### 实体事件

| 事件 | 说明 | 触发时机 |
|------|------|---------|
| `EntityDeathEvent` | 实体死亡 | 实体死亡时 |
| `PlayerKillEntityEvent` | 玩家击杀实体 | 玩家击杀实体时 |
| `PlayerHurtEvent` | 玩家受伤 | 玩家受伤时 |
| `EntityHurtEvent` | 实体受伤 | 实体受伤时 |
| `PlayerEntityInteractEvent` | 玩家与实体交互 | 玩家右键实体时 |

### 玩家事件

| 事件 | 说明 | 触发时机 |
|------|------|---------|
| `PlayerLoginEvent` | 玩家登录 | 玩家加入服务器时 |
| `PlayerLogoutEvent` | 玩家登出 | 玩家离开服务器时 |
| `PlayerRespawnEvent` | 玩家重生 | 玩家重生时 |
| `PlayerSleepEvent` | 玩家睡眠 | 玩家上床睡觉时 |
| `PlayerWakeUpEvent` | 玩家起床 | 玩家起床时 |
| `PlayerLocationEvent` | 玩家位置 | 玩家位置变化时 |
| `DimensionChangeEvent` | 维度变化 | 玩家切换维度时 |

### 物品事件

| 事件 | 说明 | 触发时机 |
|------|------|---------|
| `InventoryChangedEvent` | 物品栏变化 | 物品栏槽位变化时 |
| `ItemPickupEvent` | 物品拾取 | 玩家拾取物品时 |
| `ItemDropEvent` | 物品丢弃 | 玩家丢弃物品时 |
| `ItemUseEvent` | 物品使用 | 玩家使用物品时 |
| `ConsumeItemEvent` | 物品消耗 | 玩家消耗物品时（进食等） |
| `ItemDurabilityEvent` | 耐久变化 | 物品耐久变化时 |
| `PlayerDestroyItemEvent` | 物品销毁 | 物品因使用而损坏或消耗完毕时 |
| `EnchantItemEvent` | 附魔 | 玩家附魔物品时 |

### 效果事件

| 事件 | 说明 | 触发时机 |
|------|------|---------|
| `EffectChangedEvent` | 效果变化 | 玩家获得/失去效果时 |
| `BrewedPotionEvent` | 酿造药水 | 酿造台完成酿造时 |

### 生物事件

| 事件 | 说明 | 触发时机 |
|------|------|---------|
| `BredAnimalsEvent` | 动物繁殖 | 动物繁殖时 |
| `TameAnimalEvent` | 动物驯服 | 玩家驯服动物时 |
| `SummonedEntityEvent` | 召唤实体 | 实体被召唤时 |

### 其他事件

| 事件 | 说明 | 触发时机 |
|------|------|---------|
| `VillagerTradeEvent` | 村民交易 | 玩家与村民交易时 |
| `ConstructBeaconEvent` | 信标构建 | 信标激活时 |
| `UsedTotemEvent` | 使用不死图腾 | 玩家使用不死图腾时 |
| `LevitationEvent` | 漂浮 | 玩家漂浮时 |
| `NetherTravelEvent` | 下界旅行 | 通过下界传送门时 |
| `FishingRodHookedEvent` | 钓鱼竿钩住 | 钓鱼竿钩住实体时 |
| `ShotCrossbowEvent` | 弩射击 | 玩家用弩射击时 |
| `TargetHitEvent` | 标靶命中 | 箭命中标靶时 |

## 集成点

需要在以下位置发布事件：

### 方块交互
- `src/server/interaction/BlockInteractionManager.cpp`
  - 发布 `BlockBreakEvent`
  - 发布 `BlockPlaceEvent`

### 实体系统
- `src/common/entity/core/LivingEntity.cpp`
  - 发布 `EntityDeathEvent`
  - 发布 `EntityHurtEvent`

### 玩家系统
- `src/server/player/ServerPlayer.cpp`
  - 发布 `PlayerLoginEvent`
  - 发布 `PlayerLogoutEvent`
  - 发布 `PlayerRespawnEvent`
  - **已集成**: 发布 `InventoryChangedEvent`（通过 `inventoryChangeCallback`）

### 物品栏
- `src/common/entity/inventory/PlayerInventory.cpp`
  - **已集成**: 通过 `inventoryChangeCallback` 回调机制通知物品栏变化

### 成就系统
- `src/server/advancement/AdvancementEventHandler.hpp`
  - **已集成**: 订阅 `InventoryChangedEvent` 并触发 `InventoryChangedTrigger`
  - 订阅 `PlayerKillEntityEvent`（预留）
  - 订阅 `PlayerLoginEvent`（预留）

### 服务端主循环
- `src/server/application/MinecraftServer.cpp`
  - **已集成**: 初始化和关闭 `AdvancementEventHandler`

## 设计原则

1. **值类型**：事件应该是轻量级的值类型，避免复杂对象
2. **不可变**：事件发布后不应被修改
3. **单向**：事件是通知机制，不是请求-响应机制
4. **异步安全**：事件处理器不应阻塞主线程
5. **解耦**：发布者和订阅者不应直接依赖

## 参考

- MC 1.16.5 Forge 事件系统
- 观察者模式
- 发布-订阅模式
