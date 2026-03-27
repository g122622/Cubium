# 存档系统 (Save System)

## 概述

存档系统负责 Minecraft 世界的持久化存储，包括区块数据、玩家数据、世界元数据等。该系统与 Java 版 Minecraft 1.16.5 的存档格式完全兼容。

## 目录结构

```
save/
├── core/                  # 核心组件
│   ├── SaveFormat         # 存档格式管理器
│   ├── LevelSave          # 单个世界的存档操作
│   ├── SessionLock        # 会话锁（防止多进程访问）
│   └── DataVersion        # 数据版本常量
├── region/                # Region 文件系统
│   ├── RegionFile         # 单个 .mca 文件操作
│   ├── RegionFileCache    # Region 文件缓存
│   ├── RegionBitmap       # 扇区分配位图
│   └── CompressionType    # 压缩类型枚举
├── serializer/            # 序列化器
│   ├── ChunkSerializer    # 区块序列化
│   ├── PlayerSerializer   # 玩家数据序列化
│   ├── LevelDataSerializer # 世界元数据序列化
│   ├── EntitySerializer   # 实体序列化
│   └── BlockEntitySerializer # 方块实体序列化
├── io/                    # I/O 系统
│   ├── IOWorker           # 异步 I/O 工作线程
│   ├── FileUtil           # 文件工具函数
│   └── CompressionUtil    # GZIP/Zlib 压缩工具
├── data/                  # 数据结构
│   ├── LevelData          # 世界元数据结构
│   ├── PlayerData         # 玩家存储数据
│   ├── WorldSettings      # 世界设置
│   └── GameRules          # 游戏规则
└── manager/               # 管理器
    ├── SaveManager        # 存档管理器（主入口）
    ├── PlayerDataManager  # 玩家数据管理
    └── DimensionDataManager # 维度数据管理
```

## 模块职责

### core/ - 核心组件
提供存档系统的核心功能：存档格式管理、世界存档操作、会话锁、数据版本。

### region/ - Region 文件系统
实现 Minecraft 的 Region 文件格式（.mca），每个文件存储 32x32 个区块。

### serializer/ - 序列化器
负责将游戏对象（区块、玩家、实体等）与 NBT 格式互转。

### io/ - I/O 系统
提供异步 I/O 能力，避免阻塞主线程；提供文件操作和压缩工具。

### data/ - 数据结构
定义存档相关的数据结构，如世界元数据、玩家数据、游戏规则等。

### manager/ - 管理器
提供统一的存档 API，整合各个子系统。

## 文件格式

### level.dat
世界元数据文件，使用 GZIP 压缩的 NBT 格式。

### playerdata/<uuid>.dat
玩家数据文件，使用 GZIP 压缩的 NBT 格式。

### region/r.<x>.<z>.mca
区块数据文件，每个文件存储 32x32 个区块。

## 与 Java 版兼容性

本系统与 Minecraft Java 1.16.5 存档格式完全兼容：
- 可以读取 Java 版创建的世界
- 保存的存档可以被 Java 版读取
- 数据版本：2586

## 使用示例

```cpp
#include "world/save/manager/SaveManager.hpp"

// 创建新世界
auto saveResult = mc::world::save::SaveManager::createNew(
    "saves/", "MyWorld", settings);
if (saveResult.success()) {
    auto& saveManager = saveResult.value();

    // 保存区块
    saveManager->saveChunkAsync(*chunkData);

    // 加载区块
    auto chunkFuture = saveManager->loadChunkAsync(x, z);

    // 保存玩家
    saveManager->savePlayer(*playerData);

    // 关闭存档
    saveManager->close();
}
```

## 参考资料

- MC Java 1.16.5 源码：`net/minecraft/world/storage/`
- 文档：`docs/MC_SAVE_SYSTEM_ANALYSIS.md`
