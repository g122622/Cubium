# Entity 管理模块

负责世界中所有实体的生命周期管理、ID 分配、查询服务及更新循环。

## 目录结构

```
src/common/world/entity/
├── EntityManager.hpp    # EntityManager 类声明 - 实体管理器接口（含 UUID 索引、3D section 空间索引）
├── EntityManager.cpp    # EntityManager 类实现
└── spatial/             # 3D section(16³) 空间索引（实体分桶/类型分桶/玩家专表）
    ├── EntitySectionBucket.hpp/.cpp   # 单 section 容器（全实体列表 + 按类型子列表懒加载）
    ├── EntitySpatialIndex.hpp/.cpp    # 主索引类（section 分桶/实时迁移/查询/玩家专表/区块列取实体）
    └── README.md                       # 空间索引设计说明（结构树/查询算法/上下游/坑）
```

## 内部模块关系

- `EntityManager`：实体生命周期/ID 分配/UUID 索引/查询服务/tick 循环的外壳，持 `EntitySpatialIndex m_spatialIndex` 成员，所有空间/类型/玩家查询委托给索引。
- `EntitySpatialIndex`：3D section 空间索引，把实体按 AABB 中心所在 section 分桶，查询从 O(全服实体) 降到 O(覆盖 section 数 × section 内实体数)。详见 `spatial/README.md`。
- `EntitySectionBucket`：单个 16³ section 内的实体集合，对齐 Java `ClassInstanceMultiMap`，全实体列表为真源、按类型子列表懒加载。

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
| `ServerPlayRouter` | `server/network/ServerPlayRouter.cpp` | 入站 Play 包分发时查询实体（替代已删除的 PacketHandler） |
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
auto* ecsRegistry = world->entityRegistry();
auto pig = pigType->create(world, *ecsRegistry);
Entity* rawPtr = pig.get();
EntityId id = manager.addEntity(std::move(pig));
// pig 现在是 nullptr！需要通过 id 重新获取
Entity* entity = manager.getEntity(id);
```

### 2. ID 永不复用

实体 ID 单调递增、永不复用（`allocateId()` 为 `m_nextId++`）。移除实体后其 ID 不会被新实体复用：

```cpp
EntityId id1 = manager.addEntity(entity1);
manager.removeEntity(id1);
EntityId id2 = manager.addEntity(entity2);
// id2 一定大于 id1，绝不等于 id1。不复用可避免客户端缓存的旧 ClientEntity
// （typeId 不可变、网格按 ID 缓存）被错误套用到新实体上。
```

注：若构造实体时显式设置了已存在的 id（或 id=0），`addEntity` 会改用 `allocateId()` 重新分配，避免冲突。

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

`getEntitiesInRange()`/`getEntitiesInAABB()`/`getEntitiesByType()` 走 3D section 空间索引（`EntitySpatialIndex`）：按 AABB/球覆盖的 section 三重循环枚举，每 section 内逐实体精筛，复杂度从 O(全服实体) 降到 O(覆盖 section 数 × section 内实体数)。`getPlayers()` 走玩家专表 O(玩家数)。对于已知 UUID 的查找，仍应使用 `getEntityByUuid()` O(1)。

调用方接口签名不变，~40 处调用点自动受益于索引。不再有"全扫后 `if type==X continue`"的反模式——类型查询走各 section 的按类型子列表（懒加载）。

### 8. 空间索引实时性

实体空间归属由 `EntitySpatialIndex` 实时维护，无需调用方手动注册：

- **addEntity** 时按实体当前 AABB 中心一次性登记到对应 section，并按类型加入玩家专表（PLAYER）。
- **实体 move** 经 `Entity::reapplyPosition()`（位置变更统一收口）末尾的 `m_entityManager->_onEntityPositionChanged` 通知索引，跨 section 移动立即迁移。同一 tick 内移动后查询读到新位置。
- **removeEntity**/`_removeDeadEntitiesInternal` 时从索引移除，空 section 立即回收。

注意：`entity->remove()` 仅标记 `m_removed=true`，下次 tick 的 `_removeDeadEntitiesInternal` 才从索引移除，故标记到清理之间死亡实体仍可能在索引中被枚举到——查询层用 `isRemoved()` 双保险过滤。`Entity` 持 `EntityManager*` 反向指针（非 `m_world`）使通知在 `m_world=nullptr` 的测试场景也工作。详见 `spatial/README.md`。

### 9. 区块卸载取实体

区块卸载/关机保存取实体改走 `EntityManager::spatialIndex().getEntityIdsInChunkColumn(cx, cz)`（遍历该 chunk 列 24 个 section 合并实体 ID），按实体**当前坐标**所在 section 取列——比原按 tracker 归属（可能滞后）更准确（实体真实在哪存哪）。
