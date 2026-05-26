# Storage Backend

外来存档格式的只读存储后端抽象层。

## 架构

```
SingleLevelStorageManager (门面)
    ├── Native: RocksDB 管线 (现有实现)
    └── Foreign: IStorageBackend (新增接口)
            ├── JavaAnvilBackend  → Java 版 .mca 区域文件
            └── BedrockLDBBackend → 基岩版 LevelDB
```

## 文件说明

- **IStorageBackend.hpp** - 存储后端接口，定义 open/close/loadChunk/listChunks 等方法
- **JavaAnvilBackend** (.hpp/.cpp) - Java Anvil 格式后端，委托 RegionFile + JavaChunkReader
- **BedrockLDBBackend** (.hpp/.cpp) - 基岩版 LevelDB 格式后端，委托 BedrockLevelDb + BedrockChunkReader

## 设计原则

1. **只读**：外来格式后端始终返回 `isReadonly() == true`，所有写入操作在 SingleLevelStorageManager 层被拦截
2. **透明集成**：上层（ServerWorld、ServerChunkManager）通过 `loadChunk()` 读取数据，无需关心底层格式
3. **格式检测**：`SaveFormatDetector` 在 `open()` 时自动识别存档格式，选择对应后端
4. **轻量抽象**：Native 格式仍由现有 RocksDB 管线处理，不经过后端接口，避免破坏 ScoreboardDataManager 等依赖

## 使用方式

```cpp
SingleLevelStorageConfig config;
auto manager = std::make_unique<SingleLevelStorageManager>();
auto result = manager->open("C:/saves/MyJavaWorld", config);
// SaveFormatDetector 自动检测为 JavaAnvil，创建 JavaAnvilBackend
// 强制设置 config.readonly = true

auto chunkResult = manager->loadChunk(0, 0, 0);
// 委托给 JavaAnvilBackend::loadChunk() → RegionFile → JavaChunkReader

auto chunks = manager->listChunks(0);
// 列举主世界所有区块坐标（仅外来格式可用）
```
