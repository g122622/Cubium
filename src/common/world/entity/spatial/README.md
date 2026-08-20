# 实体空间索引模块

3D section(16³)空间索引，把实体按 AABB 中心所在 section 分桶，所有空间/类型查询从 O(全服实体) 降到 O(覆盖 section 数 × section 内实体数)。索引实时准确：实体跨 section 移动立即迁移，同一 tick 内移动后查询读到新位置。

## 目录结构

```
src/common/world/entity/spatial/
├── EntitySectionBucket.hpp     # 单个 16³ section 内的实体集合（全列表为真源 + 按类型懒加载子列表）
├── EntitySectionBucket.cpp     # add/remove swap-remove、entitiesOfType 懒加载
├── EntitySpatialIndex.hpp      # 空间索引主类（section 哈希表 + 实体→section 反查 + 玩家专表）
└── EntitySpatialIndex.cpp      # addEntity/removeEntity/onEntityPositionChanged + 4 类查询
```

## 内部模块关系

- `EntitySectionBucket`：单 section 容器，对齐 Java `ClassInstanceMultiMap`。`m_all` 全列表为真源，`m_byType` 按类型懒加载子列表（首次 `entitiesOfType(type)` 时构建，此后 `add`/`remove` 同步维护已建缓存）。
- `EntitySpatialIndex`：持有 `m_sections`（`SectionPos::toLong()` → bucket）、`m_entitySection`（实体 ID → 当前 section key，迁移对比用）、`m_players`（玩家专表）。查询算法：AABB 三重循环枚举覆盖的 section + 精筛；球转外接盒 AABB + 距离平方精筛；类型查询遍历各 section 子列表；玩家走专表 O(1)。

## 上下游外部依赖关系

### 依赖项

| 依赖 | 路径 | 用途 |
|------|------|------|
| `Entity` | `common/entity/core/Entity.hpp` | 实体基类（id/boundingBox/entityType/isRemoved） |
| `EntityType` | `common/entity/core/EntityType.hpp` | 类型分桶 key（`const EntityType*` 指针稳定） |
| `VanillaEntityTypeKeys` | `common/entity/registry/VanillaEntityTypeKeys.hpp` | 玩家专表判定（`PLAYER`） |
| `SectionPos` | `common/world/chunk/base/SectionPos.hpp` | section 坐标打包（`toLong/fromLong`） |
| `BlockPos` | `common/world/block/BlockPos.hpp` | AABB 中心 → section 坐标（`BlockPos(Vector3)` floor） |
| `AxisAlignedBB` | `common/util/AxisAlignedBB.hpp` | 精筛（`intersects/distanceToSqr/center`） |
| `WorldConstants` | `common/world/WorldConstants.hpp` | `MIN_SECTION_Y/MAX_SECTION_Y`（chunk 列 24 section 遍历） |

### 被依赖项

| 模块 | 路径 | 用途 |
|------|------|------|
| `EntityManager` | `common/world/entity/EntityManager.hpp` | 持有 `m_spatialIndex`，4 类查询转发至此；`addEntity/removeEntity/_removeDeadEntitiesInternal` 维护索引 |
| `Entity` | `common/entity/core/Entity.cpp` | `reapplyPosition()` 经 EntityManager 反向指针触发 `onEntityPositionChanged` |
| `ServerWorld` | `server/world/ServerWorld.cpp` | `onChunkUnloading/shutdown` 调 `getEntityIdsInChunkColumn` 取区块卸载/关机保存的实体（替代已删除的 `EntityChunkTracker`） |

## 容易踩的坑

### 1. 查询遍历期间实体 move 的迭代器安全

查询回调内实体 `move`→`reapplyPosition`→`onEntityPositionChanged` 可能触发当前 bucket 的 swap-remove。所有 bucket 遍历一律用**下标 + `size()` 重取**（非 range-for/迭代器），swap-remove 不失效 vector、不崩溃。接受近似语义（对齐 Java：查询期间实体 move 不保证一致性）。

### 2. `m_world=nullptr` 测试场景的索引更新

测试用裸 `EntityManager`（无 IWorld）。索引更新走 `Entity`→`EntityManager` 反向指针（非 `IWorld` 钩子），`m_world=nullptr` 时索引仍正常工作。`addEntity` 前调 `setPosition` 不通知（反向指针未设），但 `addEntity` 时按当前位置一次性登记——正确；`addEntity` 后调 `setPosition` 通知迁移——正确。

### 3. section key 用 AABB 中心而非脚底

用 `boundingBox().center()` 算 section，大体积实体跨 section 时归属稳定，避免边界抖动反复迁移。若改用脚底坐标，大体积实体（如末影龙）会在边界处频繁触发 `onEntityPositionChanged` 迁移。

### 4. 空桶立即回收

`removeEntity` 末尾 bucket `isEmpty()` 则 `m_sections.erase`，`sectionCount()` 只反映非空 section。不要假设某 section key 永久存在——查询时 `m_sections.find(key)` miss 是常态（O(1) 跳过）。

### 5. 类型子列表懒加载一致性

`m_byType` 仅在 `entitiesOfType(type)` 首次请求时构建。`add`/`remove` 只同步维护**已存在**的键——未查询过的类型不建缓存，避免预分配。若直接读写 `m_byType` 绕过 `add/remove` 会破坏与 `m_all` 的一致性。

### 6. 索引自身不加锁

本类线程安全由持有方 `EntityManager::m_mutex`（recursive_mutex）保证。`m_sections`/`m_players` 声明 `mutable`：const 查询方法内回调经 Entity→EntityManager→索引触发 section 迁移（逻辑 const）。不要在本类内加独立锁——会与 EntityManager 锁形成死锁或破坏重入语义。

### 7. `getEntityIdsInChunkColumn` 遍历 24 个 section

区块卸载取实体遍历该 chunk 列 y 从 `MIN_SECTION_Y(-4)` 到 `MAX_SECTION_Y(19)` 共 24 个 section。区块卸载是低频操作（受 `CHUNK_UNLOAD_RADIUS` 与 tick 节流），24 次哈希查找可忽略。不要把它用于热路径查询。
