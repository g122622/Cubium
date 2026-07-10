# World Storage 模块

## 概述

本模块实现世界存档的持久化层，提供以下核心功能：

1. **世界元数据管理**：level.dat NBT 编解码、世界列表管理
2. **RocksDB 存储层**：高性能 Section 级区块存储
3. **会话锁和命名规范化**：防止多进程访问冲突
4. **双层门面接口**：`GlobalStorageManager` 负责跨存档能力，`SingleLevelStorageManager` 负责单存档运行时
5. **保存协调**：`flushAllDirty()` 仅用于 Section/玩家增量落盘，`saveAll()` 用于全量落盘
6. **外来存档只读接入**：自动识别 Java Anvil / Bedrock LevelDB，统一门面暴露区块、玩家与 level.dat 读取能力

遵循 Minecraft Java 1.16.5 的 level.dat 格式规范。

## 目录结构

```
storage/
├── GlobalStorageManager.hpp/cpp      # 跨存档全局门面（世界列表、打开存档）
├── SingleLevelStorageManager.hpp/cpp # 单存档运行时门面（区块/玩家/实体读写）
├── backend/                          # 外来存档只读后端
│   ├── IStorageBackend.hpp           # 存储后端接口
│   ├── JavaAnvilBackend.hpp/cpp      # Java Anvil 格式后端
│   ├── BedrockLDBBackend.hpp/cpp     # 基岩版 LevelDB 格式后端
│   └── README.md
├── core/                             # 核心存储基础设施
│   ├── LevelDatCodec.hpp/cpp         # level.dat NBT 编解码器
│   ├── WorldStoragePaths.hpp/cpp     # 存档路径配置
│   ├── WorldSessionLock.hpp/cpp      # 会话锁（RAII）
│   └── SaveFormat.hpp/cpp            # 存档格式检测
├── list/                             # 世界列表管理
│   ├── WorldListEntry.hpp/cpp        # 世界列表条目数据模型
│   ├── WorldListService.hpp/cpp      # 世界列表服务
│   └── WorldNameSanitizer.hpp/cpp    # 名称规范化
├── request/                          # 请求/响应结构体
│   └── WorldRequests.hpp/cpp         # 世界操作请求
├── db/                               # RocksDB 存储层
│   ├── RocksDBConfig.hpp             # RocksDB 配置
│   ├── RocksDBDatabase.hpp/cpp       # 数据库封装
│   ├── ColumnFamilies.hpp            # 列族定义
│   ├── SectionKey.hpp                # Section 键结构（13字节）
│   ├── SectionCodec.hpp/cpp          # Section 序列化（ZSTD 压缩）
│   └── ConsistencyMode.hpp           # 一致性模式枚举
├── section/                          # Section 数据管理
│   ├── SectionCache.hpp/cpp          # LRU 缓存
│   ├── SectionManager.hpp/cpp        # Section 加载/保存/缓存
│   └── README.md
├── snapshot/                         # 快照系统
│   └── BackupManager.hpp/cpp         # 快照管理
├── save/                             # 保存管理
│   ├── DirtyTracker.hpp/cpp          # 脏 Section 追踪
│   └── AutoSave.hpp/cpp              # 自动保存
├── player/                           # 玩家数据存储
│   ├── PlayerSaveData.hpp/cpp        # 玩家数据结构和 NBT 序列化
│   ├── PlayerDataManager.hpp/cpp     # 玩家数据管理器（缓存+持久化）
│   └── README.md
├── entity/                           # 实体存储
│   ├── EntityKey.hpp                 # 实体存储键格式
│   ├── EntityStorageManager.hpp/cpp  # 实体存储管理器
│   └── README.md
├── blockentity/                      # 方块实体存储
│   └── BlockEntityStorageManager.hpp/cpp
├── task/                             # 存储异步任务
│   ├── StorageTask.hpp/cpp           # 存储任务封装
│   └── StorageTaskManager.hpp/cpp    # 任务调度门面
└── reader/                           # 外来存档读取器
    ├── java/                         # Java Anvil 读取器链
    │   ├── JavaWorldReader.hpp/cpp   # region 目录定位
    │   ├── JavaColumnReader.hpp/cpp  # 列级数据聚合
    │   ├── JavaChunkReader.hpp/cpp   # section 级解码
    │   ├── JavaBlockStateMapper.hpp/cpp
    │   ├── JavaBiomeMapper.hpp/cpp
    │   ├── JavaLevelDatReader.hpp/cpp
    │   ├── RegionFile.hpp/cpp        # region 文件解析
    │   └── README.md
    └── bedrock/                      # 基岩版 LevelDB 读取器链
        ├── BedrockWorldReader.hpp/cpp
        ├── BedrockColumnReader.hpp/cpp
        ├── BedrockChunkReader.hpp/cpp
        ├── BedrockLevelDb.hpp/cpp    # LevelDB 只读接口
        ├── BedrockBiomeMapper.hpp/cpp
        ├── BedrockLevelDatReader.hpp/cpp
        ├── LevelDBKey.hpp/cpp        # LevelDB 键格式
        ├── PaletteUtil.hpp/cpp       # palette 解码工具
        └── README.md
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           Game Layer                                     │
│  ServerWorld / ServerChunkManager / PlayerManager / MinecraftServer      │
└───────────────────────────────────┬─────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      GlobalStorageManager                                │
│  世界列表、存档创建、打开存档 → 返回 SingleLevelStorageManager            │
└───────────────────────────────────┬─────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                    SingleLevelStorageManager                             │
│  单存档运行时门面：区块/玩家/实体/方块实体读写、保存协调                   │
│                                                                         │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐    │
│  │SectionManager│ │PlayerDataManager│ │EntityStorage│ │BackupManager│   │
│  │  (每维度)    │ │              │ │  Manager     │ │              │    │
│  └──────┬───────┘ └──────────────┘ └──────────────┘ └──────────────┘    │
│         │                                                               │
│         ▼                                                               │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐                     │
│  │SectionCache  │ │SectionCodec  │ │StorageTaskMgr│                     │
│  └──────────────┘ └──────────────┘ └──────────────┘                     │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
            ┌───────────────────────┼───────────────────────┐
            ▼                       ▼                       ▼
    ┌──────────────┐       ┌──────────────┐       ┌──────────────┐
    │RocksDBDatabase│      │IStorageBackend│      │LevelDatCodec │
    │  (Native)    │       │  (外来格式)   │       │              │
    └──────────────┘       └──────┬───────┘       └──────────────┘
                                   │
                   ┌───────────────┼───────────────┐
                   ▼                               ▼
           ┌──────────────┐               ┌──────────────┐
           │JavaAnvilBackend│             │BedrockLDBBackend│
           └──────────────┘               └──────────────┘
```

## 上下游外部依赖关系

### 上游（谁依赖了这个模块）

- `ServerWorld` - 世界运行时通过 `SingleLevelStorageManager` 进行区块/玩家/实体持久化
- `ServerChunkManager` - 区块加载/保存通过 `loadChunk()` / `saveChunk()`
- `PlayerManager` - 玩家加入/退出/保存通过 `PlayerDataManager`
- `MinecraftServer` - 服务器启动时通过 `GlobalStorageManager` 打开存档，关闭时调用 `saveAll()`
- `/save-all` 命令 - 触发全量保存
- 世界选择界面 - 通过 `GlobalStorageManager::listWorlds()` 枚举存档

### 下游（这个模块依赖了谁）

- `common/core/Result.hpp` - 错误处理
- `common/util/nbt/Nbt.hpp` - NBT 解析
- `common/core/Types.hpp` - 基础类型（ChunkCoord, DimensionId 等）
- `common/profiler/TraceEvents.hpp` - 性能追踪
- `rocksdb` - 键值存储（Native 格式）
- `zstd` - Section 压缩
- `zlib` - gzip 压缩/解压
- `LibArchive` - zip 备份
- `spdlog` - 日志
- `reader/java/` - Java Anvil 读取器链
- `reader/bedrock/` - 基岩版 LevelDB 读取器链

## 访问控制原则

**重要**：存储模块对外分成两层门面。

- **跨存档调用方**（世界选择、存档创建、路径解析）只能访问 `GlobalStorageManager`
- **单存档运行时调用方**（如 ServerWorld、MinecraftServer）只能访问 `SingleLevelStorageManager`
- **内部模块**（如 SectionManager、RocksDBDatabase、SectionCache、backend/）不允许被外部直接访问
- `SingleLevelStorageManager` 通过 getter 方法暴露单存档子服务（`playerDataManager()`、`entityStorage()` 等）
- 区块运行时不应再直接依赖 `SectionCodec`、`SectionKey`、`RocksDBDatabase`、`WorldStoragePaths`
- 区块持久化细节统一收口到 `SingleLevelStorageManager::saveChunk()` / `loadChunk()`

## 容易踩的坑

1. **Section 索引计算**：`index = y * 256 + z * 16 + x`，注意 Y 是高位
2. **生物群系采样**：4x4x4 采样，共 64 个值
3. **光照数据**：NibbleArray，每方块 4 位
4. **列族必须预先创建**：打开数据库时会自动创建缺失的列族
5. **RocksDB 快照**：内存中的 sequence number，不持久化
6. **全量保存与增量保存不同**：`flushAllDirty()` 不会写入干净缓存，且当前不覆盖运行时实体/方块实体；`saveAll()` 才会在世界层配合下完整落盘
7. **`close()` 不负责保存**：关闭存储前必须由上层显式调用 `flushAllDirty()` 或 `saveAll()`，析构/close 只做资源释放
8. **外来格式 detect 不要下沉到 backend**：backend 只负责按门面层已确认的格式打开和读取
9. **外来存档强制只读**：`saveChunk()`、`flushAllDirty()`、`saveAll()`、`saveLevelData()` 在外来格式下静默成功但不落盘
10. **`StorageTaskManager` 不拥有线程池**：必须由外部注入 `ServerWorkerPool`
11. **缓存命中和批量读取必须分开处理**：先查缓存，只把 miss 交给数据库，不要把所有 key 都无脑送进 `MultiGet`
12. **批量读取返回顺序必须稳定**：上层 `loadChunk()` 依赖返回顺序与 `sectionY` 顺序一致
13. **`loadPlayer("~local_player")` 是约定**：本地玩家通过这个特殊字符串读取，Java 从 `Data.Player`，Bedrock 从 `~local_player` 键
