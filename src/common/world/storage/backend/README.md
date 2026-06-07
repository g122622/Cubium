# Storage Backend

外来存档格式的只读存储后端抽象层。

## 目录结构

```
backend/
├── IStorageBackend.hpp       # 存储后端接口，定义 open/close/loadChunk/loadPlayer/loadLevelData 等读取方法
├── JavaAnvilBackend.hpp/cpp  # Java Anvil 格式后端，委托 JavaWorldReader -> JavaColumnReader -> JavaChunkReader
└── BedrockLDBBackend.hpp/cpp # 基岩版 LevelDB 格式后端，委托 BedrockWorldReader -> BedrockColumnReader -> BedrockChunkReader
```

## 内部模块关系

```
IStorageBackend (接口)
    ├── JavaAnvilBackend
    │       ├── JavaWorldReader      → region 目录定位、region 缓存
    │       ├── JavaColumnReader     → Status 过滤、biome/heightmap 分派
    │       ├── JavaChunkReader      → section 级方块/光照解码
    │       ├── JavaBlockStateMapper → Java 方块状态字符串 → 内部 stateId
    │       └── JavaBiomeMapper      → Java 生物群系名称/ID → 内部 BiomeId
    └── BedrockLDBBackend
            ├── BedrockLevelDb       → LevelDB 只读接口
            ├── BedrockWorldReader   → 列举维度内已有列
            ├── BedrockColumnReader  → subchunk/biome/heightmap 聚合
            ├── BedrockChunkReader   → subchunk palette 解码
            └── BedrockBiomeMapper   → 基岩版 ID → 内部 BiomeId
```

两个 backend 都已对齐统一本地玩家约定：`loadPlayer("~local_player")`。

## 上下游依赖关系

### 上游（谁依赖了这个目录）

- `SingleLevelStorageManager` - 在 `open()` 时根据 `SaveFormatDetector` 结果创建对应 backend 实例，并委托读取操作

### 下游（这个目录依赖了谁）

- `SaveFormat` - 格式枚举和 `SaveFormatInfo`
- `ChunkData` - 区块数据结构
- `PlayerSaveData` - 玩家数据结构
- `LevelRuntimeData` / `LevelDatCodec` - 世界元数据
- `reader/java/` - Java Anvil 读取器链
- `reader/bedrock/` - 基岩版 LevelDB 读取器链

## 设计原则

1. **只读**：外来格式后端始终返回 `isReadonly() == true`，所有写入操作在 `SingleLevelStorageManager` 层被拦截
2. **透明集成**：上层（ServerWorld、ServerChunkManager）通过 `loadChunk()` 读取数据，无需关心底层格式
3. **格式检测单点化**：`SaveFormatDetector` 只在 `SingleLevelStorageManager::open()` 执行一次，backend 不重复 detect
4. **轻量抽象**：Native 格式仍由现有 RocksDB 管线处理，不经过后端接口，避免破坏 ScoreboardDataManager 等依赖

## 容易踩的坑

- **外来格式 detect 不要下沉到 backend**：backend 只负责按门面层已确认的格式打开和读取，避免 detector 规则在两层分叉
- **`loadPlayer("~local_player")` 是约定**：本地玩家通过这个特殊字符串读取，Java 从 `Data.Player`，Bedrock 从 `~local_player` 键
- **Backend 不负责实体注入世界运行时**：Java/Bedrock 实体 NBT 会解析为运行时实体实例挂在 `ChunkData` 上，但何时进入 `EntityManager` 属于服务器集成职责
- **超出 `CHUNK_SECTIONS` 范围的基岩子区块会被跳过**：不会扩展现有世界高度模型
