# Bedrock LevelDB Reader

基岩版 Minecraft 存档读取器，采用 world / column / chunk / key / palette 分层架构。

## 目录结构

```
reader/bedrock/
├── BedrockLevelDb.hpp/cpp       # LevelDB 只读接口，数据库打开、单键读取、前缀遍历
├── BedrockWorldReader.hpp/cpp   # world 级读取器，列举维度内已有列并调度列读取
├── BedrockColumnReader.hpp/cpp  # column 级读取器，subchunk/biome/heightmap 聚合入口
├── BedrockChunkReader.hpp/cpp   # section 级解码器，subchunk palette、Data2D、BiomeState 解码
├── BedrockBiomeMapper.hpp/cpp   # 生物群系映射器，基岩版 ID → 内部 BiomeId
├── BedrockLevelDatReader.hpp/cpp # level.dat 读取器，解析基岩版世界元数据（8 字节头 + 小端序 NBT）
├── LevelDBKey.hpp/cpp           # Bedrock 键空间语义，统一表达各种键类型
├── PaletteUtil.hpp/cpp          # Bedrock palette 公共解包工具，varuint 和 32-bit packed indices
└── README.md
```

## 内部模块关系

```
BedrockLDBBackend (位于 backend/)
    └── BedrockWorldReader
            ├── BedrockColumnReader
            │       └── BedrockChunkReader
            │               └── BedrockBiomeMapper
            ├── BedrockLevelDb
            ├── LevelDBKey
            └── PaletteUtil
```

- `BedrockWorldReader` 负责”这个维度有哪些列”
- `BedrockColumnReader` 负责”这一列要拼哪些 payload”
- `BedrockChunkReader` 负责”单个 payload 如何解码”
- `LevelDBKey` / `PaletteUtil` 是跨层复用的公共工具

## 上下游外部依赖关系

### 上游（谁依赖了这个目录）

- `backend/BedrockLDBBackend` - 基岩版存储后端，持有并调用本目录所有 reader
- `list/WorldListService` - 通过 backend 间接使用 BedrockLevelDatReader 读取世界摘要

### 下游（这个目录依赖了谁）

- `common/world/chunk/ChunkData` / `ChunkSection` - 区块数据结构
- `common/world/block/BlockState` - 方块状态
- `common/util/nbt` - NBT 解析（contexts::bedrock_disk）
- `common/core/Result` - 错误处理
- `leveldb` - LevelDB 数据库（第三方库）

## 容易踩的坑

- **LevelDB 键格式因维度而异**：主世界键不含 dimension 字段（9 字节），其他维度含 4 字节 dimension（13 字节）；子区块键额外 1 字节 subChunkY
- **基岩版调色板使用 int[] 而非 Java 的 long[]**：位压缩索引为 32 位，索引顺序 `x = (i >> 8) & 0xF, y = i & 0xF, z = (i >> 4) & 0xF`
- **runtimeEncoding 标志决定调色板解析方式**：`true` 直接读取运行时 state id，`false` 需解析小端序 NBT 的 `name`/`states` 再映射到内部 BlockState
- **生物群系 ID 编号与 Java 版不完全一致**：必须通过 BedrockBiomeMapper 转换
- **version 9 subchunk 的 Y 坐标从 header 读取**：不能盲信 key 上的 subChunkY；version 8 的 caves&cliffs `-4` 偏移仍待补齐
- **超出 CHUNK_SECTIONS 范围的基岩子区块会被跳过**：不会扩展现有世界高度模型
- **多 storage layer 目前只处理第 0 层**：额外液体/覆盖层尚未完整合并
- **实体使用 `identifier` 字符串而非 Java 的 `id`**：解析时需注意字段名差异
