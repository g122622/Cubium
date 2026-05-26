# Bedrock LevelDB Reader

基岩版 Minecraft 存档读取器，支持基岩版 1.18+ 的 LevelDB 存储格式。

## 文件说明

- **BedrockLevelDb** (.hpp/.cpp) - LevelDB 只读接口，键构建、前缀遍历
- **BedrockChunkReader** (.hpp/.cpp) - 区块调色板解析器，解析子区块数据、生物群系
- **BedrockBiomeMapper** (.hpp/.cpp) - 生物群系映射器，基岩版 ID → 内部 BiomeId
- **BedrockLevelDatReader** (.hpp/.cpp) - level.dat 读取器，解析基岩版世界元数据（8 字节头 + 小端序 NBT）

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
- 超出当前项目 `CHUNK_SECTIONS` 范围的基岩子区块仍会跳过，不会扩展现有世界高度模型
- 多 storage layer 目前仍只把第 0 层当作主方块层处理，额外液体/覆盖层尚未完整合并
