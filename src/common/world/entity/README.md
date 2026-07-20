# Entity 管理模块

负责世界中所有实体的生命周期管理、ID 分配、查询服务及更新循环。

## 目录结构

```
src/common/world/entity/
├── EntityManager.hpp    # EntityManager 类声明 - 实体管理器接口（含 UUID 索引）
└── EntityManager.cpp    # EntityManager 类实现
```

## 内部模块关系

本目录仅包含 `EntityManager` 单个类，无内部模块划分。

## 上下游外部依赖关系

### 依赖项（本模块使用的其他模块）

| 依赖 | 路径 | 用途 |
|------|------|------|
| `Entity` | `common/entity/core/Entity.hpp` | 实体基类 |
| `EntityDataManager` | `common/entity/core/EntityDataManager.hpp` | 实体数据同步 |
| `EntityClassification` | `common/entity/core/EntityClassification.hpp` | 实体分类枚举 |
| `VanillaEntityTypeKeys` | `common/entity/registry/VanillaEntityTypeKeys.hpp` | 实体类型指针缓存（const EntityType*） |
| `EntityRegistry` | `common/entity/core/EntityRegistry.hpp` | 实体类型注册表 |
| `Vector3` | `common/util/math/Vector3.hpp` | 3D向量（位置、速度） |
| `AxisAlignedBB` | `common/util/AxisAlignedBB.hpp` | 碰撞箱 |
| `Types` | `common/core/Types.hpp` | 基础类型（EntityId等） |

### 被依赖项（使用本模块的其他模块）

| 模块 | 路径 | 用途 |
|------|------|------|
| `ServerWorld` | `server/world/ServerWorld.hpp` | 服务端世界持有 EntityManager，实现 IWorld::getEntityByUuid() |
| `ClientWorld` | `client/world/ClientWorld.hpp` | 客户端世界持有 EntityManager |
| `EntityTracker` | `server/world/entity/EntityTracker.cpp` | 实体追踪器查询实体 |
| `ItemPickupManager` | `server/world/entity/ItemPickupManager.cpp` | 物品拾取管理器查询实体 |
| `NaturalSpawner` | `server/world/spawn/NaturalSpawner.cpp` | 自然生成器统计实体数量 |
| `DespawnManager` | `server/world/spawn/DespawnManager.cpp` | 消失管理器查询实体 |
| `PacketHandler` | `server/core/PacketHandler.cpp` | 数据包处理时查询实体 |
| 各种 Command | `server/command/commands/*.cpp` | 命令执行时查询实体 |
| `TraderLlamaEntity` | `common/entity/entities/passive/horse/TraderLlamaEntity.cpp` | 通过 UUID 查找拴绳持有者 |
| `EvokerFangsEntity` | `common/entity/entities/projectile/OtherProjectiles.cpp` | 通过 UUID 查找所有者 |
| `AreaEffectCloudEntity` | `common/entity/entities/effect/EffectEntities.cpp` | 通过 UUID 查找所有者 |
| `TrialSpawnerBlockEntity` | `common/world/blockentity/trial/TrialSpawnerBlockEntity.cpp` | 通过 UUID 追踪已生成怪物 |
| `VaultBlockEntity` | `common/world/blockentity/trial/VaultBlockEntity.cpp` | 通过 UUID 查找玩家 |
| `ConduitEntity` | `common/world/blockentity/processing/ConduitEntity.cpp` | 通过 UUID 恢复攻击目标 |

## 容易踩的坑

### 1. 实体所有权问题

`EntityManager::addEntity()` 接受 `std::unique_ptr<Entity>`，调用后原指针不再有效：

```cpp
auto pig = pigType->create(nullptr);
Entity* rawPtr = pig.get();
EntityId id = manager.addEntity(std::move(pig));
// pig 现在是 nullptr！需要通过 id 重新获取
Entity* entity = manager.getEntity(id);
```

### 2. ID 重用问题

实体 ID 会被重用。移除实体后，新添加的实体可能获得相同 ID：

```cpp
EntityId id1 = manager.addEntity(entity1);
manager.removeEntity(id1);
EntityId id2 = manager.addEntity(entity2);
// id2 可能等于 id1！移除实体后必须清除所有对该 ID 的引用
```

### 3. 线程安全与重入

`EntityManager` 使用 `std::recursive_mutex` 支持同线程重入。在 `tick()` 和 `forEachEntity()` 回调内可以安全调用其他查询接口：

```cpp
manager.forEachEntity([&](Entity* entity) {
    // 安全：可以在回调内重入查询
    auto nearby = manager.getEntitiesInRange(entity->position(), 10.0f);
    return true;
});
```

### 4. UUID 索引一致性

`EntityManager` 维护两个索引：`m_entities`（EntityId → Entity）和 `m_uuidToEntity`（UUID → Entity*）。在以下场景需注意一致性：

- **UUID 冲突**：添加 UUID 相同的实体时，UUID 索引会被覆盖，旧映射丢失。此时会输出 spdlog::warn 警告。
- **空 UUID**：UUID 为空字符串的实体不会被索引，`getEntityByUuid("")` 始终返回 nullptr。
- **移除后索引清理**：`removeEntity()` 和 `removeDeadEntities()` 会同步清理 UUID 索引，仅当映射指向当前实体时才移除（防止 UUID 冲突时误删新映射）。
- **setUuid() 不会更新索引**：在 `addEntity()` 之后调用 `Entity::setUuid()` 不会更新 UUID 索引。NBT 反序列化时 UUID 的设置应在 `addEntity()` 之前完成，否则需重新添加实体以更新索引。

### 5. getEntity/getEntityByUuid 返回空指针

`getEntity()` 和 `getEntityByUuid()` 可能返回 `nullptr`，必须检查：

```cpp
if (Entity* entity = manager.getEntityByUuid(someUuid)) {
    entity->setPosition(0, 0, 0);
}
```

### 6. 实体移除时机

`entity->remove()` 只是标记实体为移除状态，实体会在下一次 `tick()` 时被真正移除。如需立即移除：

```cpp
entity->remove();  // 标记
manager.tick();    // 此后实体被移除

// 或立即移除
manager.removeEntity(entity->id());
```

### 7. 空间查询性能

`getEntitiesInRange()` 和 `getEntitiesInAABB()` 当前为 O(n) 遍历，大量实体时性能受限。对于已知 UUID 的查找，应使用 `getEntityByUuid()` 进行 O(1) 查找。
