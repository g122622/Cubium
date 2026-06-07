# 服务端事件系统 (Server Event System)

## 概述

本模块实现了服务端事件总线，用于游戏事件的通知和订阅。事件总线是一个通用的发布-订阅基础设施，支持类型安全的事件订阅、自动取消订阅、事件优先级和事件过滤。

## 目录结构

```
server/event/
├── ServerEventBus.hpp           # 事件总线（单例，模板实现）
├── ServerEventBus.cpp           # 单例实例定义
├── README.md                    # 本文件
│
└── events/                      # 事件定义
    └── ServerEvents.hpp         # 所有服务端事件定义（40+ 事件类型）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      ServerEventBus                          │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  subscribe<EventT>() → HandlerId                     │   │
│  │  unsubscribe(HandlerId)                              │   │
│  │  publish(EventT)                                     │   │
│  │  makeSubscription<EventT>() → Subscription (RAII)    │   │
│  │  addFilter/removeFilter                              │   │
│  └─────────────────────────────────────────────────────┘   │
│                           │                                  │
│                           ▼                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  events/ServerEvents.hpp                             │   │
│  │  - BlockBreakEvent, BlockPlaceEvent, ...            │   │
│  │  - EntityDeathEvent, PlayerKillEntityEvent, ...     │   │
│  │  - PlayerLoginEvent, InventoryChangedEvent, ...     │   │
│  │  - 共 40+ 事件类型，继承自 ServerEvent 基类          │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

**事件分发流程**：
1. 发布者调用 `publish(event)`
2. 事件总线按类型查找处理器
3. 应用过滤器（如有）
4. 按优先级（数值越大越先执行）调用处理器

## 上下游外部依赖关系

### 本模块依赖

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基本类型定义（PlayerId、EntityId 等） |
| `common/entity/core/Entity.hpp` | Entity 类（事件参数） |
| `common/item/core/ItemStack.hpp` | ItemStack 类（事件参数） |
| `common/world/block/BlockPos.hpp` | BlockPos 类型（事件参数） |
| `common/world/GlobalPos.hpp` | GlobalPos 类型（事件参数） |

### 被依赖（计划集成点）

| 模块 | 发布的事件 |
|------|-----------|
| `server/interaction/BlockInteractionManager` | BlockBreakEvent, BlockPlaceEvent |
| `server/player/ServerPlayer` | PlayerLoginEvent, PlayerLogoutEvent, InventoryChangedEvent |
| `common/entity/core/LivingEntity` | EntityDeathEvent, EntityHurtEvent |
| `server/advancement/AdvancementEventHandler` | 订阅 InventoryChangedEvent 触发成就 |
| `server/application/MinecraftServer` | 初始化和关闭 AdvancementEventHandler |

**注意**：当前事件系统已定义完成，但尚未与游戏逻辑集成。事件定义和总线实现均已就绪，等待各模块在适当位置调用 `publish()` 发布事件。

## 容易踩的坑

### 1. 事件是值类型

事件应该是轻量级的值类型，避免存储复杂对象指针。事件发布后会被复制分发给所有订阅者。

### 2. 线程安全

`ServerEventBus` 使用互斥锁保护内部状态，支持多线程访问。但事件处理器中不应阻塞主线程。

### 3. 订阅生命周期管理

推荐使用 RAII 订阅（`makeSubscription`），避免手动管理订阅导致的内存泄漏：
```cpp
// 推荐：RAII 自动管理
auto subscription = ServerEventBus::instance().makeSubscription<BlockBreakEvent>(handler);

// 不推荐：手动管理容易遗漏
auto id = ServerEventBus::instance().subscribe<BlockBreakEvent>(handler);
// 忘记 unsubscribe() 会导致泄漏
```

### 4. 事件是单向通知

事件是通知机制，不是请求-响应机制。不要期望通过事件修改游戏状态或返回值。

### 5. 取消事件

`ServerEvent::cancel()` 仅对 beforeEvent 有意义（脚本系统使用）。取消后游戏逻辑将跳过原始操作，但后续的 beforeEvent 处理器仍会被调用。

### 6. 优先级顺序

优先级数值越大越先执行。默认优先级为 0。需要先处理的事件（如日志记录）应设置较高优先级。
