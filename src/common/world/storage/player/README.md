# 玩家数据存储模块

## 概述

本模块实现玩家数据的持久化存储，包括：

1. **PlayerSaveData** - 玩家数据结构（NBT 序列化）
2. **PlayerDataManager** - 玩家数据管理器（缓存 + 持久化）

玩家数据存储在 RocksDB 的 `players` 列族中，使用 NBT 格式序列化。

## 目录结构

```
player/
├── PlayerSaveData.hpp      # 玩家数据结构定义
├── PlayerSaveData.cpp      # 序列化/反序列化实现
├── PlayerDataManager.hpp   # 玩家数据管理器接口
├── PlayerDataManager.cpp   # 管理器实现
└── README.md               # 本文件
```

## 内部模块关系

```
┌─────────────────┐      ┌─────────────────────┐
│  ServerPlayer   │─────▶│  PlayerSaveData     │
│  ServerPlayerData│     │  (序列化)            │◀─────┐
└─────────────────┘      └─────────────────────┘      │
                                  │                   │
                                  ▼                   │
                         ┌─────────────────┐          │
                         │ PlayerDataManager│          │
                         └─────────────────┘          │
                                  │                   │
                                  ▼                   │
                         ┌─────────────────┐          │
                         │   RocksDB       │          │
                         │  players 列族   │          │
                         └─────────────────┘          │
                                                       │
┌─────────────────┐      ┌─────────────────────┐      │
│  Player 实体     │◀─────│  applyToPlayer()    │──────┘
│  (登录恢复)      │      │  (反序列化恢复)      │
└─────────────────┘      └─────────────────────┘
```

## 上下游外部依赖关系

**上游依赖（本模块被谁使用）**：
- `SingleLevelStorageManager` - 通过 `playerDataManager()` 暴露本模块
- `ServerWorld` - 在玩家加入/退出/保存时调用
- `PlayerManager` - 管理在线玩家时触发保存
- `IntegratedServer` / `StandaloneServer` - 关服时通过 `savePlayerRuntimeState()` 钩子回写在线玩家运行时状态（`fromPlayer()` + `savePlayer()`），登录时通过 `applyToPlayer()` 恢复
- `/save-all` 命令 - 触发 `saveAllDirty()`

**下游依赖（本模块依赖谁）**：
- `RocksDBDatabase` - 底层持久化存储
- `NbtIo` - NBT 序列化/反序列化
- `ItemStack` - 物品序列化
- `EffectInstance` - 药水效果序列化
- `Player` / `ServerPlayerData` - 运行时玩家状态转换（`fromPlayer` 接受 `const Player&`，`fromServerPlayerData` 接受 `const ServerPlayerData&`）
- `zlib` - gzip 压缩

## 与 MC 1.16.5 的对应

| MC 1.16.5 | 本项目 |
|-----------|--------|
| PlayerData | PlayerDataManager |
| player/*.dat | players 列族 |
| CompoundNBT | nbt::tags::compound_tag |
| CompressedStreamTools | PlayerSaveData::serialize/deserialize |

## 容易踩的坑

1. **玩家 UUID**: 使用基于用户名的离线模式 UUID（MC 1.16.5 标准算法：`UUID.nameUUIDFromBytes(("OfflinePlayer:" + username).getBytes(UTF_8))`）

2. **物品序列化**: 背包物品使用 `ItemStack::toNbt()` 和 `ItemStack::fromNbt()`，格式遵循 MC 1.16.5 ItemStack NBT 格式（id, Count, tag）

3. **效果序列化**: 药水效果使用 `EffectInstance::toNbt()` 和 `EffectInstance::fromNbt()`，格式遵循 MC 1.16.5（Id, Amplifier, Duration, Ambient, ShowParticles, ShowIcon）

4. **自动保存时机**: 通过 `SingleLevelStorageManager` 内部的 `AutoSave` 机制，玩家数据会在世界保存时一起保存；析构函数不负责保存，上层必须显式调用

5. **缓存与脏标记**: `savePlayer()` 只标记脏数据，`savePlayerImmediate()` 才同步写入；`saveAllDirty()` 批量保存脏数据，`saveAll()` 保存所有缓存

6. **applyToPlayer()**: `PlayerDataManager::applyToPlayer(Player&, const PlayerSaveData&)` 将保存数据恢复到 Player 实体，在玩家登录时由 `StandaloneServer`/`IntegratedServer` 调用。恢复的字段包括：位置/旋转、维度、游戏模式、生命值、饥饿值、经验、玩家能力、重生点、下界入口位置、最后死亡位置、睡眠状态、空气供应、疾跑/潜行状态、冲量上下文、背包物品、鼠标持有物品、药水效果

7. **两条序列化路径**:
   - **Entity NBT 路径**: `Player::addAdditionalSaveData()` / `Player::readAdditionalSaveData()` — 实体序列化链，由 `EntityDeserializer` 在加载非玩家实体时使用
   - **PlayerSaveData 路径**: `PlayerSaveData::toNbt()` / `PlayerSaveData::fromNbt()` + `PlayerDataManager::fromPlayer()` / `applyToPlayer()` — 独立的玩家存储系统，用于登录保存/恢复

8. **冲量上下文持久化**: 冲量上下文字段（`currentImpulseImpactPos`、`ignoreFallDamageFromCurrentImpulse`、`currentImpulseContextResetGraceTime`）同时通过两条路径持久化。`m_currentExplosionCause` 不持久化（MC Java 同样不序列化此运行时瞬时字段）

9. **关服时玩家运行时状态回写（fromPlayer + savePlayerRuntimeState 钩子）**:
   `PlayerDataManager::fromPlayer(const Player&)` 提取 Player 实体的运行时状态（位置、生命、饥饿、经验、背包、效果等）为 `PlayerSaveData`。该方法在关服时由 `IntegratedServer::savePlayerRuntimeState()` 和 `StandaloneServer::savePlayerRuntimeState()` 调用——遍历所有维度的在线 Player 实体，调用 `fromPlayer()` 提取状态，再用 `savePlayer()` 灌入缓存并标记脏。后续 `stopCore()` → `shutdownManagers()` → `saveAllWorldData()` 会通过 `PlayerDataManager::saveAll()` 把缓存落盘到 RocksDB。
   - **签名说明**：`fromPlayer()` 的参数类型为 `const Player&`，可接受 `ServerPlayer`（`ServerPlayerEntityManager::createPlayerEntity` 创建的就是 `ServerPlayer` 实例，通过基类引用访问运行时状态）。
   - **调用时机约束**：必须在主循环线程 join 之后、玩家实体被 `clearAll()` 移除之前调用，否则会与 `tick()` 产生数据竞争或拿到空指针。详见 `src/server/application/README.md` 第 9、10 节。
   - **UUID 来源覆盖**：`Player` 实体的 `m_uuid` 由登录流程（`handleLoginRequestPacket`）计算离线 UUID 后存入 `ServerPlayerData`，但**未回写到实体本身**。若直接用 `fromPlayer()` 提取的 `uuid` 字段落盘，会以空字符串作为 RocksDB key，导致下次登录无法读回。因此 `savePlayerRuntimeState()` 在调用 `fromPlayer()` 后，会用 `PlayerManager` 中的权威 UUID（`playerData->uuid`）覆盖 `saveData.uuid`，确保落盘 key 与登录时 `loadPlayer()` 查询的 key 一致。
