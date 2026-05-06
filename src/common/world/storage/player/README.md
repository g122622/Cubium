# 玩家数据存储模块

## 概述

本模块实现玩家数据的持久化存储，包括：

1. **PlayerSaveData** - 玩家数据结构
2. **PlayerDataManager** - 玩家数据管理器

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

## 文件介绍

### PlayerSaveData.hpp

玩家持久化数据结构，包含：

| 字段 | 类型 | 描述 |
|------|------|------|
| uuid | String | 玩家唯一标识符 |
| username | String | 用户名 |
| posX/Y/Z | f64 | 世界坐标 |
| yaw/pitch | f32 | 旋转角度 |
| dimension | DimensionId | 当前维度 |
| gameMode | GameMode | 游戏模式 |
| health | f32 | 当前生命值 |
| foodLevel | i32 | 饥饿值 (0-20) |
| experienceLevel | i32 | 经验等级 |
| experienceProgress | f32 | 经验进度 |
| inventorySlots | vector<Slot> | 背包物品 |
| effects | vector<EffectInstance> | 药水效果 |
| spawnPoint | optional<GlobalPos> | 重生点 |
| abilities | PlayerAbilities | 玩家能力 |

### PlayerDataManager.hpp

玩家数据管理器，提供：

- **loadPlayer(uuid)** - 从数据库加载玩家数据
- **savePlayer(data)** - 标记玩家数据为脏
- **savePlayerImmediate(data)** - 立即保存玩家数据
- **saveAllDirty()** - 保存所有脏数据
- **saveAll()** - 保存所有缓存数据
- **markDirty(uuid)** - 标记玩家数据为脏

## 使用示例

```cpp
// 通过 WorldStorageService 获取玩家数据管理器
auto& storage = world.storage();
auto* playerMgr = storage.playerDataManager();

// 保存在线玩家
server.playerManager().forEachPlayer([&](ServerPlayerData& playerData) {
    PlayerSaveData saveData = PlayerDataManager::fromServerPlayerData(playerData);
    playerMgr->savePlayerImmediate(saveData);
});

// 加载玩家数据
auto result = playerMgr->loadPlayer("player-uuid");
if (result.success() && result.value()) {
    PlayerSaveData& data = *result.value();
    // 使用数据恢复玩家状态
}
```

## 与 MC 1.16.5 的对应

| MC 1.16.5 | 本项目 |
|-----------|--------|
| PlayerData | PlayerDataManager |
| player/*.dat | players 列族 |
| CompoundNBT | nbt::tags::compound_tag |
| CompressedStreamTools | PlayerSaveData::serialize/deserialize |

## 数据流向

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

## 依赖项

- **WorldStorageService** - 提供数据库访问
- **RocksDBDatabase** - 底层存储
- **NbtIo** - NBT 序列化
- **zlib** - gzip 压缩

## 注意事项

1. **玩家 UUID**: 目前使用 `playerId` 转换为字符串，未来需要实现真正的 UUID
2. **物品序列化**: 背包物品序列化已实现，使用 `ItemStack::toNbt()` 和 `ItemStack::fromNbt()` 方法
3. **效果序列化**: 药水效果序列化已实现，使用 `EffectInstance::toNbt()` 和 `EffectInstance::fromNbt()` 方法
4. **自动保存**: 通过 SaveManager 的自动保存机制，玩家数据会在世界保存时一起保存
5. **物品格式**: 遵循 MC 1.16.5 ItemStack NBT 格式（id, Count, tag）
6. **效果格式**: 遵循 MC 1.16.5 EffectInstance NBT 格式（Id, Amplifier, Duration, Ambient, ShowParticles, ShowIcon）
