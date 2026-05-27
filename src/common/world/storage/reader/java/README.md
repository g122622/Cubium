# Java Anvil Reader

Java 版 Minecraft 存档读取器，支持 Java 1.16.5+ 的 Anvil 区域文件格式。

## 文件说明

- **RegionFile** (.hpp/.cpp) - `.mca` 区域文件读取器，负责偏移表解析、区块定位、数据解压
- **JavaWorldReader** (.hpp/.cpp) - world 级读取器，负责 region 目录定位、region 缓存、列数据入口
- **JavaColumnReader** (.hpp/.cpp) - column 级读取器，负责 `Status` 过滤、`xPos/zPos` 校验、biome / heightmap / entities / block entities 路径分派
- **JavaChunkReader** (.hpp/.cpp) - section 级读取器，只负责方块 palette、方块状态、光照，以及 1.18 biome palette 的局部解包辅助
- **JavaBlockStateMapper** (.hpp/.cpp) - 方块状态映射器，将 Java 版方块状态字符串映射到内部 `stateId`
- **JavaBiomeMapper** (.hpp/.cpp) - 生物群系映射器，将 Java 版生物群系名称/ID 映射到内部 `BiomeId`
- **JavaLevelDatReader** (.hpp/.cpp) - `level.dat` 读取器，解析 Java 版世界元数据（gzip + 大端序 NBT）

## 分层关系

当前 Java 读取链已按 `Chunker` 的 world / column / chunk 三层职责拆开：

```text
JavaAnvilBackend
  -> JavaWorldReader
      -> RegionFile
      -> JavaColumnReader
          -> JavaChunkReader
```

- `JavaWorldReader` 不再解析 chunk NBT 内容，只负责“到哪里找列数据”
- `JavaColumnReader` 负责列级字段与版本分支，包括旧版 `Biomes` 数组和 1.18 `sections[].biomes`
- `JavaChunkReader` 不再负责 root / Level / sections 遍历，只负责“单个 section 如何解码”

## 当前阶段边界

- 已完成 world / column / chunk 分层，以及 1.17+ `entities/` world 级合并
- 已补 Java biome 主路径，并把 heightmap / block entity 恢复接到现有 `ChunkData` / `BlockEntityRegistry`
- 运行时 `Entity` 导入链当前仍未接上：`entities/` 现在只保证列输入 NBT 被正确合并，不代表项目已经能把 Java 实体 NBT 还原为运行时实体实例
- 因此这里的边界必须说清楚，不能把“world 层合并成功”误写成“实体系统已完整恢复”

## 1.17+ entities 区域合并

当前已补上 `P1-2` 的 world 级合并入口：

- `JavaWorldReader` 在 `SaveFormatInfo.dataVersion >= 2724` 时，同时扫描 `region/` 与 `entities/`
- 如果主 region 和 entities region 同时存在，会把 entities 文件中的 `Entities` 列表合并回主列 NBT
- 如果只存在 entities region，会按 `Position` 构造最小列 NBT，并把 `Entities` 挂回列根

这和 `Chunker` 的分层保持一致：`entities/` 合并职责属于 world 层，而不是 `JavaColumnReader` / `JavaChunkReader`

## 关键技术细节

### 区域文件格式 (.mca)
- 8192 字节头部：1024 条偏移记录 + 1024 条时间戳记录
- 每条偏移记录 4 字节：3 字节扇区偏移 + 1 字节扇区数
- 区块数据格式：4 字节长度 + 1 字节压缩类型 + 压缩数据
- 压缩类型：1=GZip, 2=ZLib, 3=Uncompressed

### Java 位压缩格式
- 使用 `long[]` 数组（64 位）存储位压缩索引
- `bitsPerEntry = max(4, ceil(log2(palette.size())))`
- 每个 long 存储 `64 / bitsPerEntry` 个索引
- 解包：`index[i] = (data[i / valuesPerLong] >> ((i % valuesPerLong) * bitsPerEntry)) & mask`

### Java 生物群系路径
- 旧版路径支持 `ByteArray Biomes`
- 1.15+ 支持 `Biomes[1024]`
- 兼容 `Biomes[256]`
- 1.18+ 主路径为 `sections[].biomes.palette + data`
- 这些列级 biome 分支统一由 `JavaColumnReader` 选择，`JavaChunkReader` 仅保留 1.18 section biome palette 的解包辅助
