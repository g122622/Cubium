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
    void setServer(IServer* server);     // 设置服务器接口（必须）
    void setPlayerManager(core::PlayerManager* pm); // 用于通过 UUID 查找玩家
    void initialize();  // 订阅事件
    void shutdown();    // 取消订阅

private:
    ServerPlayer* getServerPlayer(PlayerId playerId);  // 从 PlayerId 获取 ServerPlayer
    void onInventoryChanged(const InventoryChangedEvent& e);
    void onPlayerKillEntity(const PlayerKillEntityEvent& e);
    void onPlayerLogin(const PlayerLoginEvent& e);
    void onBlockPlaced(const BlockPlaceEvent& e);
    void onCuredZombieVillager(const CuredZombieVillagerEvent& e);
};
```

#### 架构说明

AdvancementEventHandler 需要从 PlayerId 获取 ServerPlayer 以触发成就检测。

**调用链：**

```
PlayerId (事件携带)
       │
       ▼
IServer::playerEntityManager()
       │
       ▼
ServerPlayerEntityManager::getPlayerEntity(playerId, world)
       │
       ▼
Player*
       │
       ▼
Player::asServerPlayer()
       │
       ▼
ServerPlayer*
```

**关键点：**

1. **不使用 PlayerManager**：`PlayerManager::getPlayer()` 返回 `ServerPlayerData`，这是网络会话数据结构，不持有 `ServerPlayer` 引用。

2. **使用 ServerPlayerEntityManager**：这是正确的路径，它维护 `PlayerId ↔ EntityId` 映射，并能从 `EntityManager` 获取 `Player` 实体。

3. **初始化顺序**：必须在 `initialize()` 之前调用 `setServer(this)`。

**示例：**

```cpp
// MinecraftServer::initializeInteractionManagers()
m_advancementEventHandler.setServer(this);
m_advancementEventHandler.initialize();
```

## 事件集成

### 已集成的事件

| 事件 | 触发器 | 状态 |
|------|--------|------|
| `InventoryChangedEvent` | `InventoryChangedTrigger` | ✅ 已完成 |
| `PlayerKillEntityEvent` | `PlayerKilledEntityTrigger` | ✅ 已完成 |
| `BlockPlaceEvent` | `PlacedBlockTrigger` | ✅ 已完成 |
| `CuredZombieVillagerEvent` | `CuredZombieVillagerTrigger` + 村庄声望更新 | ✅ 已完成 |
| `ChanneledLightningEvent` | `ChanneledLightningTrigger` | ✅ 已完成 |
| `ConsumeItemEvent` | `ConsumeItemTrigger` | ✅ 已完成 |
| `ItemDurabilityEvent` | `ItemDurabilityTrigger` | ✅ 已完成 |
| `EnchantItemEvent` | `EnchantedItemTrigger` | ✅ 已完成 |
| `FilledBucketEvent` | `FilledBucketTrigger` | ✅ 已完成 |
| `BredAnimalsEvent` | `BredAnimalsTrigger` | ✅ 已完成 |
| `PlayerLoginEvent` | 玩家成就初始化 | 预留 |

### 事件处理器架构

AdvancementEventHandler 使用两种方式获取玩家：

1. **通过 PlayerId（事件携带）**：用于 `InventoryChangedEvent`、`PlayerKillEntityEvent`、`BlockPlaceEvent`
   - 使用 `IServer::playerEntityManager()` 获取 `ServerPlayerEntityManager`
   - 通过 `getPlayerEntity(playerId, world)` 获取 `Player*`
   - 转换为 `ServerPlayer*`

2. **通过 UUID（事件携带）**：用于 `CuredZombieVillagerEvent`
   - 使用 `PlayerManager::findByUuid(uuid)` 获取 `ServerPlayerData*`
   - 从 `ServerPlayerData::playerId` 获取 `PlayerId`
   - 再通过 `getServerPlayer(playerId)` 获取 `ServerPlayer*`

### 村庄声望更新

当玩家治愈僵尸村民时，除了触发成就，还会更新村庄声望：

**声望值参考 MC 1.16.5:**

| 流言类型 | 添加值 | 声誉影响 | 说明 |
|---------|--------|---------|------|
| MajorPositive | 20 | +100 声誉 | 治愈僵尸村民获得 |
| MinorPositive | 25 | +25 声誉 | 治愈僵尸村民获得 |

**实现位置：** `AdvancementEventHandler::updateVillageReputationOnCure()`

```cpp
// 治愈僵尸村民时更新村庄声望
// 参考 MC 1.16.5: VillagerEntity.updateReputation(IReputationType.ZOMBIE_VILLAGER_CURED)
void updateVillageReputationOnCure(const std::string& starterUuid, Entity* villager)
{
    // 1. 获取村民所在村庄
    Village* village = villageManager->getVillageAt(villagerPosition);

    // 2. 如果村民在村庄内，更新声望
    if (village != nullptr) {
        village->addGossip(playerId, VillageGossipType::MajorPositive, 20);
        village->addGossip(playerId, VillageGossipType::MinorPositive, 25);
    }
}
```

**注意：** 只有当村民在村庄范围内时才更新声望，符合 MC 1.16.5 行为。

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

成就系统测试位于：
- `tests/advancement/AdvancementTest.cpp` - 触发器实例创建和检测、物品谓词匹配、槽位计数检测
- `tests/server/advancement/AdvancementEventHandlerTest.cpp` - getServerPlayer 架构验证、事件订阅生命周期
- `tests/server/advancement/VillageReputationTest.cpp` - 村庄声望更新、治愈僵尸村民声望测试

运行测试：
```bash
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="*Advancement*"
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="AdvancementEventHandlerTest*"
```

### AdvancementEventHandler 测试覆盖

| 测试用例 | 描述 |
|---------|------|
| SetServerNotNull | 验证 setServer() 可以设置非空指针 |
| SetServerNullptr | 验证 setServer(nullptr) 不崩溃 |
| SetServerMultipleTimes | 验证多次设置服务器接口 |
| GetServerPlayerWithoutServer | 验证未设置服务器时的行为 |
| GetServerPlayerWithNullServer | 验证服务器为 nullptr 时的行为 |
| InitializeShutdown | 验证初始化和关闭生命周期 |
| InitializeMultipleTimes | 验证多次初始化 |
| ShutdownWithoutInitialize | 验证未初始化时关闭 |
| SetPlayerManagerCompat | 验证向后兼容的 setPlayerManager() |
| SetBothServerAndPlayerManager | 验证同时设置两个 |
| ArchitectureGetServerPlayerPath | 验证调用链架构 |
| EventSubscriptionLifecycle | 验证事件订阅生命周期 |

## 参考

- MC 1.16.5: `net.minecraft.server.PlayerAdvancements`
- MC 1.16.5: `net.minecraft.advancements.CriteriaTriggers`
- 服务端事件系统: `src/server/event/README.md`
