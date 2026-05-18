# EntityManager 模块

## 目录结构

```
src/common/world/entity/
├── EntityManager.hpp    # EntityManager 类声明
└── EntityManager.cpp    # EntityManager 类实现
```

## 文件详解

### EntityManager.hpp

**职责**: 定义 `EntityManager` 类，负责世界中所有实体的生命周期管理。

**主要内容**:

| 方法类别 | 方法 | 说明 |
|---------|------|------|
| **创建/销毁** | `addEntity()` | 添加实体，自动分配ID或使用实体现有ID |
| | `removeEntity()` | 移除实体并返回所有权，释放ID供重用 |
| | `hasEntity()` | 检查实体是否存在 |
| | `entityCount()` | 获取实体数量 |
| **查询** | `getEntity()` | 通过ID获取实体指针 |
| | `getEntitiesInAABB()` | 获取碰撞箱内的所有实体 |
| | `getEntitiesInRange()` | 获取范围内的所有实体 |
| | `getEntitiesByType()` | 获取指定类型的所有实体 |
| | `forEachEntity()` | 遍历所有实体 |
| **更新** | `tick()` | 更新所有实体，移除已标记的死亡实体 |
| | `removeDeadEntities()` | 移除所有已标记为移除的实体 |
| **ID分配** | `allocateId()` | 分配新实体ID（内部方法） |
| | `releaseId()` | 释放实体ID供重用（内部方法） |

**线程安全**: 所有公共方法都使用 `std::recursive_mutex` 保护，支持多线程访问，并允许同线程在 `tick()`/`forEachEntity()` 回调内重入调用查询接口。

### EntityManager.cpp

**职责**: 实现 `EntityManager` 类的所有方法。

**实现细节**:

1. **ID分配策略**:
   - 从 `m_nextId = 1` 开始递增分配
   - 被移除的ID存入 `m_freeIds` 池中供重用
   - 优先从空闲池分配，池空时才递增分配新ID

2. **实体存储**:
   - 使用 `std::unordered_map<EntityId, std::unique_ptr<Entity>>` 存储
   - 拥有实体的所有权（`unique_ptr`）

3. **范围查询优化**:
   - `getEntitiesInRange()` 使用距离平方避免开方运算
   - `getEntitiesInAABB()` 使用 `AxisAlignedBB::intersects()` 快速判断

4. **清理策略**:
   - `tick()` 中自动清理 `isRemoved() == true` 的实体
   - `removeDeadEntitiesInternal()` 假设已持有锁，避免死锁

## 文件关系图

```
                    ┌─────────────────────┐
                    │    Types.hpp        │
                    │  (EntityId, 等)     │
                    └─────────┬───────────┘
                              │
                              ▼
┌─────────────────┐    ┌─────────────────────┐    ┌─────────────────────┐
│   Vector3.hpp   │───▶│  EntityManager.hpp  │◀───│  AxisAlignedBB.hpp  │
│   (位置/速度)    │    │   (实体管理器)       │    │    (碰撞箱)          │
└─────────────────┘    └─────────┬───────────┘    └─────────────────────┘
                                 │
                                 │ 依赖
                                 ▼
                       ┌─────────────────────┐
                       │     Entity.hpp      │
                       │   (实体基类)         │
                       │                     │
                       │  - LegacyEntityType │
                       │  - EntityPose       │
                       │  - EntityFlags      │
                       │  - 位置/速度/旋转    │
                       │  - 碰撞箱/状态      │
                       └─────────┬───────────┘
                                 │
                                 │ 依赖
                                 ▼
                       ┌─────────────────────┐
                       │ EntityDataManager   │
                       │   (数据同步管理)     │
                       └─────────────────────┘
```

## 模块整体说明

### 整体职责

`EntityManager` 模块负责：

1. **实体生命周期管理**: 创建、存储、销毁世界中的所有实体
2. **实体ID分配**: 自动分配唯一ID，支持ID重用以避免溢出
3. **实体查询服务**: 提供多种查询方式（按ID、按位置、按类型）
4. **实体更新循环**: 每tick调用所有实体的 `tick()` 方法
5. **死亡实体清理**: 自动移除标记为 `removed` 的实体

### 输入

| 输入类型 | 来源 | 说明 |
|---------|------|------|
| `std::unique_ptr<Entity>` | 调用方 | 要添加的实体对象 |
| `EntityId` | 调用方 | 用于查询/移除的实体ID |
| `AxisAlignedBB` | 调用方 | 用于空间查询的碰撞箱 |
| `Vector3` + `range` | 调用方 | 用于范围查询的中心点和半径 |
| `EntityTypeId` | 调用方 | 用于类型查询的实体类型ID |

### 输出

| 输出类型 | 说明 |
|---------|------|
| `Entity*` | 实体指针，用于访问实体属性和方法 |
| `std::unique_ptr<Entity>` | 被移除的实体所有权 |
| `std::vector<Entity*>` | 查询结果列表 |
| `EntityId` | 新分配的实体ID |
| `size_t` | 实体数量 |

### 依赖项

| 依赖 | 路径 | 用途 |
|------|------|------|
| `Entity` | `common/entity/Entity.hpp` | 实体基类 |
| `EntityDataManager` | `common/entity/EntityDataManager.hpp` | 实体数据同步 |
| `Vector3` | `common/util/math/Vector3.hpp` | 3D向量（位置、速度） |
| `AxisAlignedBB` | `common/util/AxisAlignedBB.hpp` | 碰撞箱 |
| `Types` | `common/core/Types.hpp` | 基础类型（EntityId等） |
| `spdlog` | 第三方库 | 日志输出 |

### 使用方法

#### 基本使用

```cpp
#include "common/world/entity/EntityManager.hpp"
#include "common/entity/Entity.hpp"
#include "common/entity/VanillaEntities.hpp"

using namespace mc;
using namespace mc::entity;

// 1. 创建实体管理器
EntityManager entityManager;

// 2. 创建并添加实体
const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
auto pig = pigType->create(nullptr);
pig->setPosition(100.0f, 64.0f, 200.0f);

EntityId pigId = entityManager.addEntity(std::move(pig));

// 3. 获取实体
Entity* entity = entityManager.getEntity(pigId);
if (entity) {
    entity->setPosition(105.0f, 64.0f, 205.0f);
}

// 4. 范围查询
Vector3 center(100.0f, 64.0f, 200.0f);
std::vector<Entity*> nearby = entityManager.getEntitiesInRange(center, 50.0f);

// 5. 碰撞箱查询
AxisAlignedBB box(90.0f, 60.0f, 190.0f, 110.0f, 70.0f, 210.0f);
std::vector<Entity*> inBox = entityManager.getEntitiesInAABB(box);

// 6. 更新实体
entityManager.tick();

// 7. 移除实体
entityManager.removeEntity(pigId);
```

#### 与 ServerWorld 集成

```cpp
class ServerWorld {
    EntityManager m_entityManager;

    void tick() {
        // 更新所有实体
        m_entityManager.tick();

        // 同步实体状态到客户端
        m_entityManager.forEachEntity([this](Entity* entity) {
            if (entity->dataManager().hasDirtyData()) {
                syncEntityData(entity);
            }
            return true; // 继续遍历
        });
    }

    void spawnEntity(std::unique_ptr<Entity> entity) {
        EntityId id = m_entityManager.addEntity(std::move(entity));
        // 广播生成事件到客户端
        broadcastEntitySpawn(id);
    }
};
```

### 容易踩的坑

#### 1. 实体所有权问题

```cpp
// 错误：EntityManager 获得所有权后，原指针不再有效
auto pig = pigType->create(nullptr);
Entity* rawPtr = pig.get();
EntityId id = manager.addEntity(std::move(pig));
// pig 现在是 nullptr！
// rawPtr 仍然有效，但不再拥有对象

// 正确：添加后通过 ID 获取
EntityId id = manager.addEntity(std::move(pig));
Entity* entity = manager.getEntity(id);
```

#### 2. ID 重用问题

```cpp
// ID 可能被重用
EntityId id1 = manager.addEntity(entity1);
manager.removeEntity(id1);
EntityId id2 = manager.addEntity(entity2);
// id2 可能等于 id1！

// 解决方案：移除实体后清除所有对该ID的引用
```

#### 3. 线程安全陷阱

```cpp
// 错误：迭代期间其他线程可能修改
for (auto& [id, entity] : manager) { // 无法这样迭代
    // ...
}

// 正确：使用 forEachEntity 或返回的 vector
manager.forEachEntity([](Entity* entity) {
    // 安全，持有锁
    return true; // 继续迭代
});

// 或获取快照
std::vector<Entity*> entities = manager.getEntitiesInRange(pos, range);
// 快照是线程安全的副本
```

#### 4. 空指针检查

```cpp
// getEntity 可能返回 nullptr
Entity* entity = manager.getEntity(someId);
// 错误：未检查空指针
entity->setPosition(0, 0, 0);

// 正确：始终检查
if (Entity* entity = manager.getEntity(someId)) {
    entity->setPosition(0, 0, 0);
}
```

#### 5. 移除时机问题

```cpp
// tick() 会自动移除 isRemoved() == true 的实体
entity->remove(); // 标记为移除
// 此时实体仍在管理器中
manager.tick();   // 此后实体被移除

// 如果需要立即移除：
manager.removeEntity(entity->id());
```

### 涉及的测试用例

测试文件：`tests/common/world/EntityManagerSpawnTest.cpp`

| 测试用例 | 测试内容 |
|---------|---------|
| `AddEntity` | 添加单个实体，验证ID分配和存在性 |
| `AddMultipleEntities` | 添加多个实体，验证ID唯一性和计数 |
| `AddDifferentEntityTypes` | 添加不同类型实体（猪、牛、羊、鸡） |
| `GetEntity` | 通过ID获取实体，验证属性正确性 |
| `GetEntityNotFound` | 查询不存在的ID，验证返回nullptr |
| `GetEntityByType` | 按类型获取实体 |
| `RemoveEntity` | 移除实体，验证ID释放 |
| `RemoveNonExistentEntity` | 移除不存在的实体，验证不崩溃 |
| `RemoveAndAddAgain` | 移除后重新添加，验证ID可重用 |
| `EntityPositionAfterSpawn` | 验证生成后位置和旋转正确 |
| `SpawnFromSpawnedEntityData` | 从 SpawnedEntityData 创建实体 |
| `BatchSpawnFromSpawnedEntityData` | 批量从 SpawnedEntityData 创建实体 |
| `MobEntityIsLivingEntity` | 验证 MobEntity 类型转换 |
| `AnimalEntityIsMobEntity` | 验证 AnimalEntity 继承链 |
| `RemoveMultipleEntities` | 逐个移除多个实体，验证计数正确 |
| `TickAllowsReentrantRangeQuery` | 验证 `tick()` 内重入 `getEntitiesInRange()` 不会死锁 |
| `ForEachAllowsReentrantQueries` | 验证 `forEachEntity()` 回调内重入查询不会死锁 |

### 性能考虑

1. **实体数量**: 使用 `unordered_map` 实现 O(1) 的ID查询
2. **范围查询**: 当前实现为 O(n) 遍历，大量实体时考虑空间分区优化
3. **锁粒度**: 每个操作都持有全局锁，高并发场景可能成为瓶颈
4. **ID重用**: 避免ID无限增长导致的溢出问题

### 与 MC 1.16.5 的对应关系

| MC Java 1.16.5 | 本项目 | 说明 |
|---------------|--------|------|
| `World.entities` | `EntityManager.m_entities` | 实体存储 |
| `World.getEntityById()` | `EntityManager.getEntity()` | ID查询 |
| `World.addEntity()` | `EntityManager.addEntity()` | 添加实体 |
| `World.removeEntity()` | `EntityManager.removeEntity()` | 移除实体 |
| `World.getEntitiesInAABB()` | `EntityManager.getEntitiesInAABB()` | 碰撞箱查询 |
| `World.getEntitiesWithinAABBExcludingEntity()` | `EntityManager.getEntitiesInAABB(box, except)` | 排除查询 |
| `Entity.entityId` | `Entity.m_id` / `Entity::id()` | 实体ID |
