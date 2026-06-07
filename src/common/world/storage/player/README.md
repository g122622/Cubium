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
│  ServerPlayerData│     │  (序列化)            │
└─────────────────┘      └─────────────────────┘
                                  │
                                  ▼
                         ┌─────────────────┐
                         │ PlayerDataManager│
                         └─────────────────┘
                                  │
                                  ▼
                         ┌─────────────────┐
                         │   RocksDB       │
                         │  players 列族   │
                         └─────────────────┘
```

## 上下游外部依赖关系

**上游依赖（本模块被谁使用）**：
- `SingleLevelStorageManager` - 通过 `playerDataManager()` 暴露本模块
- `ServerWorld` - 在玩家加入/退出/保存时调用
- `PlayerManager` - 管理在线玩家时触发保存
- `/save-all` 命令 - 触发 `saveAllDirty()`

**下游依赖（本模块依赖谁）**：
- `RocksDBDatabase` - 底层持久化存储
- `NbtIo` - NBT 序列化/反序列化
- `ItemStack` - 物品序列化
- `EffectInstance` - 药水效果序列化
- `ServerPlayerData`/`ServerPlayer` - 运行时玩家状态转换
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
