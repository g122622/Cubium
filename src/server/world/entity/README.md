# Server World Entity 模块

本模块负责服务端实体的网络同步和物品拾取管理，是服务端世界系统的重要组成部分。

## 目录结构

```
src/server/world/entity/
├── EntityTracker.hpp       # 实体追踪器（管理实体客户端可见性，同步位置/旋转/元数据）
├── EntityTracker.cpp       # 实体追踪器实现
├── ItemPickupManager.hpp   # 物品拾取管理器（检测拾取、物品合并、背包更新）
└── ItemPickupManager.cpp   # 物品拾取管理器实现
```

## 内部模块关系

- **EntityTracker** 管理实体的客户端可见性，基于距离和视距计算追踪范围，发送实体生成/销毁/更新包；tick 中检测 `isHurtMarked()` 并在受伤时发送速度同步包
- **ItemPickupManager** 处理玩家拾取掉落物逻辑，包括拾取检测、背包更新。物品合并由 `ItemEntity::_updateMerge` 在每实体 tick 中统一处理（移动时每 2 tick、静止时每 40 tick 检测，对应原版 `ItemEntity.mergeWithNeighbours` 单一路径），ItemPickupManager 不再重复扫描合并

注：实体空间归属（含区块卸载取实体）已由 common 层 `EntitySpatialIndex`（3D section 索引）统一承担，本模块不再维护独立的区块归属追踪器（原 `EntityChunkTracker` 已删除）。区块卸载/关机保存时 `ServerWorld` 经 `EntityManager::spatialIndex().getEntityIdsInChunkColumn(cx, cz)` 取该 chunk 列内全部实体。

## 上下游外部依赖关系

**被依赖方（上游）**：
- `server/application/IServer`, `MinecraftServer` - 服务器接口和主类
- `server/core/PlayerManager`, `ConnectionManager`, `ServerPlayerData` - 玩家/连接管理
- `common/entity/core/Entity`, `LivingEntity`, `MobEntity`, `ItemEntity`, `Player` - 实体类型
- `common/world/entity/EntityManager` - 实体管理器
- `common/network/ir/packets/play/*` - IR 网络包（`AddEntity` 生成、`SetEntityData` 元数据、`SetEntityMotion` 速度、`RemoveEntities` 销毁等；旧 `EntityPackets`/`InventoryPackets` 已删除）
- `common/entity/core/Entity`（hurtMarked 标记） - 实体受伤标记驱动速度同步

**依赖方（下游）**：
- `MinecraftServer` - 在 tick 中调用 `EntityTracker::tick()` 和 `ItemPickupManager::tick()`

---

## 容易踩的坑

### 1. 线程安全问题
`EntityTracker` 使用 `std::mutex` 保护内部状态，但在持有锁时不应调用外部回调，否则可能死锁。正确做法是先收集需要发送的数据，释放锁后再发送。

### 2. 实体 ID 类型转换
协议使用 `u32` 作为实体 ID，但内部使用 `EntityId`，网络同步时需要 `static_cast<u32>(entity->id())`。

### 3. 物品拾取延迟
刚丢弃的物品有 10 tick 的拾取延迟（`DEFAULT_THROWER_PICKUP_DELAY`），防止玩家立即拾取自己丢弃的物品。

### 4. 实体类型判断
`sendSpawnPacket` 在 1.21.11 IR 下统一发送 `ir::play::AddEntity`（不再区分 `SpawnMobPacket`/`SpawnEntityPacket`）。`MobEntity` 的 headYaw 通过 `AddEntity.yHeadRot` 透传；元数据（含 `ItemEntity` 的 `ItemStack`）走独立的 `ir::play::SetEntityData`——客户端收 `AddEntity` 后必须再收一次 `SetEntityData` 才能正确渲染。

### 5. UUID 处理
Entity 内部以 `std::string` 存储 UUID（32字符十六进制），网络包需要 `std::array<u8, 16>` 格式，使用 `util::uuidFromString()` 进行转换。

### 6. 物品合并单一入口
物品合并仅由 `ItemEntity::_updateMerge` 处理（搜索 AABB 内邻居，数量较少的合并到较多的，受 `ItemEntity::MERGE_RANGE` 控制）。`ItemEntity::tick` 起始处检查空物品立即移除（`getItem().isEmpty()`），`pickupDelay` 仅当 `>0 且 != FAKE_PICKUP_DELAY(32767)` 时递减（创造假物品永不递减、不可拾取）。ItemPickupManager::tick 不再调用任何合并扫描，避免与 `_updateMerge` 重复执行导致 CPU 翻倍。

### 7. hurtMarked 速度同步机制
- EntityTracker::tick() 遍历追踪实体时检查 `entity->isHurtMarked()`
- 当 hurtMarked 为 true 时，调用 `_sendVelocityPacket()` 向所有追踪玩家发送 `ir::play::SetEntityMotion`（旧 `EntityVelocityPacket` 已删除，统一走 IR），然后调用 `clearHurtMarked()` 清除标记
- 该机制确保实体受伤/击退后客户端速度立即同步，避免客户端预测与服务器不一致
- `_sendVelocityPacket()` 是新增的私有方法，封装速度包的构建和广播逻辑

### 8. "AndSelf" 速度同步与 causeExtraKnockback 修复
- MC Java 的 `sendToTrackingPlayersAndSelf` 中，当实体本身是 Player 时也需要向其自身发送速度包
- EntityTracker::tick() 在 hurtMarked 为 true 时，除了向追踪玩家发送速度包，还会通过 `Player::sendVelocityPacket()` 虚方法向自身发送
- ServerPlayer 不会出现在自己的追踪列表中，因此需要单独发送
- `Player::causeExtraKnockback()` 在疾跑击退时，会先为 ServerPlayer 目标立即发送速度包并清除 hurtMarked，此分支不会执行，从而避免速度重复应用
