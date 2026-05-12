# 服务端成就系统 (Server Advancement System)

## 概述

本模块实现了服务端成就系统的核心组件，包括：
- 玩家成就进度管理
- 触发器实例化
- 事件处理器集成

## 目录结构

```
server/advancement/
├── PlayerAdvancements.hpp/cpp     # 玩家成就进度管理
├── TriggerInstantiation.hpp       # 触发器实例化工具
├── AdvancementEventHandler.hpp    # 事件处理器（订阅事件触发成就）
└── README.md                      # 本文件
```

## 核心类

### PlayerAdvancements

玩家成就进度管理器，负责：
- 追踪玩家所有成就的进度
- 管理触发器监听器
- 持久化进度数据
- 授予/撤销成就

```cpp
// 获取玩家成就
auto* advancements = player->getAdvancements();

// 授予成就条件
advancements->grantCriterion(advancement, "diamond");

// 检查是否完成
if (advancements->isDone(advancement)) {
    spdlog::info("Player completed advancement!");
}
```

### TriggerInstantiation

触发器实例化工具，提供：
- 从 JSON 加载触发器实例的统一接口
- 触发器实例工厂方法

```cpp
// 使用工厂方法创建触发器实例
auto instance = InventoryChangedTrigger::hasItems(
    ItemPredicate::create().item("minecraft:diamond")
);
```

### AdvancementEventHandler

事件处理器，订阅服务端事件并触发相应的成就触发器：

```cpp
class AdvancementEventHandler {
public:
    void initialize();  // 订阅事件
    void shutdown();    // 取消订阅

private:
    void onInventoryChanged(const InventoryChangedEvent& e);
    void onPlayerKillEntity(const PlayerKillEntityEvent& e);
    void onPlayerLogin(const PlayerLoginEvent& e);
};
```

## 事件集成

### 已集成的事件

| 事件 | 触发器 | 状态 |
|------|--------|------|
| `InventoryChangedEvent` | `InventoryChangedTrigger` | 已完成 |
| `PlayerKillEntityEvent` | `PlayerKilledEntityTrigger` | 预留 |
| `PlayerLoginEvent` | 玩家成就初始化 | 预留 |

### 事件流程

```
玩家物品栏变化
       │
       ▼
PlayerInventory::setItem()
       │
       ▼
inventoryChangeCallback()
       │
       ▼
ServerPlayer 发布 InventoryChangedEvent
       │
       ▼
ServerEventBus 分发事件
       │
       ▼
AdvancementEventHandler::onInventoryChanged()
       │
       ▼
InventoryChangedTrigger::triggerWithPredicate()
       │
       ▼
检查玩家的成就监听器
       │
       ▼
条件满足时授予成就
```

## 使用示例

### 在 MinecraftServer 中初始化

```cpp
// MinecraftServer.cpp
void MinecraftServer::initialize() {
    // ... 其他初始化 ...
    
    // 初始化成就事件处理器
    m_advancementEventHandler.initialize();
}

void MinecraftServer::shutdown() {
    // 关闭成就事件处理器
    m_advancementEventHandler.shutdown();
    
    // ... 其他关闭 ...
}
```

### 自定义触发器集成

```cpp
// 1. 在 AdvancementEventHandler 中添加订阅
m_myEventSubscription = ServerEventBus::instance().makeSubscription<MyEvent>(
    [this](const MyEvent& e) {
        onMyEvent(e);
    }
);

// 2. 实现事件处理方法
void AdvancementEventHandler::onMyEvent(const MyEvent& e) {
    auto* trigger = CriterionTriggers::instance().getTrigger<MyTrigger>();
    if (trigger) {
        trigger->triggerWithPredicate(*e.player->getAdvancements(), [&](const auto& instance) {
            return instance.test(e.someCondition);
        });
    }
}
```

## 与 common/advancement 的关系

```
common/advancement/                    server/advancement/
     │                                      │
     ├─ 触发器定义 ◄───────────────────── 触发器实例化
     │  (ICriterionTrigger)                 (TriggerInstantiation)
     │                                      │
     ├─ 触发器实例 ◄───────────────────── 触发器触发
     │  (ICriterionInstance)                (使用 triggerWithPredicate)
     │                                      │
     └─ 成就管理 ◄─────────────────────── 玩家进度
        (AdvancementManager)               (PlayerAdvancements)
                                            │
                                            └─ 事件订阅
                                               (AdvancementEventHandler)
```

## 测试

成就系统测试位于 `tests/advancement/AdvancementTest.cpp`，包括：
- 触发器实例创建和检测
- 物品谓词匹配
- 槽位计数检测
- 序列化/反序列化

运行测试：
```bash
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="*Advancement*"
```

## 参考

- MC 1.16.5: `net.minecraft.server.PlayerAdvancements`
- MC 1.16.5: `net.minecraft.advancements.CriteriaTriggers`
- 服务端事件系统: `src/server/event/README.md`
