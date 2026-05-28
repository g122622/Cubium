# Bedrock LevelDB Reader

基岩版 Minecraft 存档读取器，当前正在按 `Chunker` 的 world / column / chunk / key / palette 分层重建。

## 文件说明

- **BedrockLevelDb** (.hpp/.cpp) - LevelDB 只读接口，只负责数据库打开、单键读取、前缀遍历
- **LevelDBKey** (.hpp/.cpp) - Bedrock 键空间语义，统一表达 chunk / subchunk / local player / actorprefix / portals 等键
- **PaletteUtil** (.hpp/.cpp) - Bedrock palette 公共解包工具，负责 varuint 和 32-bit packed indices
- **BedrockWorldReader** (.hpp/.cpp) - world 级读取器，负责列举维度内已有列并调度列读取
- **BedrockColumnReader** (.hpp/.cpp) - column 级读取器，负责 subchunk / biome / heightmap 聚合入口
- **BedrockChunkReader** (.hpp/.cpp) - section 级解码器，只负责 subchunk palette、`Data2D`、`BiomeState` 等局部 payload
- **BedrockBiomeMapper** (.hpp/.cpp) - 生物群系映射器，基岩版 ID → 内部 BiomeId
- **BedrockLevelDatReader** (.hpp/.cpp) - level.dat 读取器，解析基岩版世界元数据（8 字节头 + 小端序 NBT）

## 分层关系

```text
BedrockLDBBackend
  -> BedrockWorldReader
      -> BedrockColumnReader
          -> BedrockChunkReader
      -> BedrockLevelDb
      -> LevelDBKey
      -> PaletteUtil
```

- `BedrockWorldReader` 负责“这个维度有哪些列”
- `BedrockColumnReader` 负责“这一列要拼哪些 payload”
- `BedrockChunkReader` 负责“单个 payload 如何解码”
- `LevelDBKey` / `PaletteUtil` 是跨层复用的公共工具，不再把键语义和 palette 位操作散落在 backend / chunk reader 里

## 关键技术细节

### LevelDB 键格式

主世界：
```
[chunkX:4B LE][chunkZ:4B LE][type:1B]
[chunkX:4B LE][chunkZ:4B LE][type:1B][subChunkY:1B]  (SubChunkPrefix)
```

其他维度：
```
[chunkX:4B LE][chunkZ:4B LE][dimensionId:4B LE][type:1B]
[chunkX:4B LE][chunkZ:4B LE][dimensionId:4B LE][type:1B][subChunkY:1B]
```

### 键类型
| 类型 | ID | 说明 |
|------|-----|------|
| Data3D | 43 | 新版高度图 + 3D 生物群系 |
| Version | 44 | 区块版本号（1 字节） |
| Data2D | 45 | 旧版高度图 + 生物群系 |
| SubChunkPrefix | 47 | 子区块方块数据（调色板） |
| BlockEntity | 49 | 方块实体 NBT |
| Entity | 50 | 实体 NBT |
| BiomeState | 53 | 新版生物群系（1.18+） |

### 基岩版调色板格式
- 使用 `int[]`（32 位）存储位压缩索引，不同于 Java 的 `long[]`
- `paletteData = (bitsPerEntry << 1) | runtimeEncoding`
- `bitsPerEntry == 127` 表示空调色板
- 索引顺序：`x = (i >> 8) & 0xF, y = i & 0xF, z = (i >> 4) & 0xF`
- 当前实现同时处理两种调色板条目：
  - `runtimeEncoding = true`：直接读取运行时 state id
  - `runtimeEncoding = false`：读取小端序 Bedrock NBT 条目中的 `name` / `states`，再映射到内部 `BlockState`

### 与 Java 版的区别
- LevelDB vs 区域文件
- 小端序 vs 大端序 NBT
- 32 位 int 压缩 vs 64 位 long 压缩
- 生物群系 ID 编号不完全一致
- 实体使用 `identifier` 字符串而非 `id`

## 当前边界

- 只读读取已接入区块、玩家和 `level.dat`
- `~local_player` 与 `actorprefix*` 玩家键已支持读取
- `P2-1` 已完成：world / column / chunk / key / palette 分层骨架已就位，backend 调用链已切到新 reader
- `P2-2` 已部分完成：version 9 subchunk 现在会从 header 读取真实 Y，而不是继续盲信 key 上的 subChunkY
- version 8 的旧 caves&cliffs `-4` 偏移仍待继续补齐，因为它依赖更上层的 level / generator 语义接线
- `P2-3` / `P2-4` 仍待继续精确补齐：`Data3D` 主路径、liquid / waterlogged merge
- 超出当前项目 `CHUNK_SECTIONS` 范围的基岩子区块仍会跳过，不会扩展现有世界高度模型
- 多 storage layer 目前仍只把第 0 层当作主方块层处理，额外液体/覆盖层尚未完整合并
