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

- **IStorageBackend.hpp** - 存储后端接口，定义 open/close/loadChunk/loadPlayer/loadLevelData 等读取方法
- **JavaAnvilBackend** (.hpp/.cpp) - Java Anvil 格式后端，委托 `JavaWorldReader -> JavaColumnReader -> JavaChunkReader`
- **BedrockLDBBackend** (.hpp/.cpp) - 基岩版 LevelDB 格式后端，委托 BedrockLevelDb + BedrockChunkReader

## 设计原则

1. **只读**：外来格式后端始终返回 `isReadonly() == true`，所有写入操作在 SingleLevelStorageManager 层被拦截
2. **透明集成**：上层（ServerWorld、ServerChunkManager）通过 `loadChunk()` 读取数据，无需关心底层格式
3. **格式检测单点化**：`SaveFormatDetector` 只在 `SingleLevelStorageManager::open()` 执行一次，由门面层选择对应后端并把 `SaveFormatInfo` 下发给 backend；backend 不重复 detect
4. **轻量抽象**：Native 格式仍由现有 RocksDB 管线处理，不经过后端接口，避免破坏 ScoreboardDataManager 等依赖

## 使用方式

```cpp
SingleLevelStorageConfig config;
auto manager = std::make_unique<SingleLevelStorageManager>();
auto result = manager->open("C:/saves/MyJavaWorld", config);
// SaveFormatDetector 在 SingleLevelStorageManager 内只执行一次
// 识别为 JavaAnvil 后创建 JavaAnvilBackend，并把已确定的 SaveFormatInfo 传给 backend
// 强制设置 config.readonly = true

auto chunkResult = manager->loadChunk(0, 0, 0);
// 委托给 JavaAnvilBackend::loadChunk() → RegionFile → JavaChunkReader

auto playerResult = manager->loadPlayer("~local_player");
auto levelDataResult = manager->loadLevelData();
// 统一门面只保留已完整接入主流程的读取能力
```
