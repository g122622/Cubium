# Java Anvil Reader

Java 版 Minecraft 存档读取器，支持 Java 1.16.5+ 的 Anvil 区域文件格式。

## 文件说明

- **RegionFile** (.hpp/.cpp) - .mca 区域文件读取器，负责偏移表解析、区块定位、数据解压
- **JavaChunkReader** (.hpp/.cpp) - 区块 NBT 解析器，将 Java 区块数据转换为项目的 ChunkData 结构
- **JavaBlockStateMapper** (.hpp/.cpp) - 方块状态映射器，将 Java 版方块状态字符串映射到内部 stateId
- **JavaBiomeMapper** (.hpp/.cpp) - 生物群系映射器，将 Java 版生物群系名称/ID 映射到内部 BiomeId
- **JavaLevelDatReader** (.hpp/.cpp) - level.dat 读取器，解析 Java 版世界元数据（gzip + 大端序 NBT）

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

### Java 1.16.5 生物群系
- 存储为 `int[1024]`（4x4x64 列式），需降采样到项目的 4x4x4/section 格式
