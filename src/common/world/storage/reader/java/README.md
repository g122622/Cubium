# Java Anvil Reader

Java 版 Minecraft 存档读取器，支持 Java 1.16.5+ 的 Anvil 区域文件格式。

## 目录结构

```
java/
├── RegionFile.hpp/.cpp              # .mca 区域文件读取，偏移表解析、区块定位、数据解压
├── JavaWorldReader.hpp/.cpp         # world 级读取器，region 目录定位、region 缓存、1.17+ entities 合并
├── JavaColumnReader.hpp/.cpp        # column 级读取器，Status 过滤、xPos/zPos 校验、biome/heightmap/entities 分派
├── JavaChunkReader.hpp/.cpp         # section 级读取器，方块 palette、方块状态、光照、1.18 biome palette 解包
├── JavaBlockStateMapper.hpp/.cpp    # 方块状态映射器，Java 版方块状态字符串→内部 stateId
├── JavaBiomeMapper.hpp/.cpp         # 生物群系映射器，Java 版生物群系名称/ID→内部 BiomeId
└── JavaLevelDatReader.hpp/.cpp      # level.dat 读取器，gzip+大端序 NBT、本地玩家解析
```

## 内部模块关系

```
JavaWorldReader
    ├── RegionFile          （.mca 文件读取）
    └── JavaColumnReader    （列级 NBT 解析）
            └── JavaChunkReader    （section 解码）
                    ├── JavaBlockStateMapper  （方块状态映射）
                    └── JavaBiomeMapper       （生物群系映射）
```

调用链：`JavaWorldReader` 打开 region 文件，读取原始 NBT 字节流 → `JavaColumnReader` 解析列级字段并分发 → `JavaChunkReader` 解码单个 section。

## 上下游依赖关系

### 上游依赖（本目录依赖的外部模块）

- `common/core/Result.hpp` - 错误处理
- `common/core/Types.hpp` - 基础类型（ChunkCoord, DimensionId 等）
- `common/util/nbt/Nbt.hpp` - NBT 解析（大端序 Java 格式）
- `common/util/NibbleArray.hpp` - 光照数据（4 位紧凑存储）
- `common/world/chunk/ChunkData.hpp` - 区块数据结构
- `common/world/biome/Biome.hpp` - 生物群系定义
- `common/world/blockentity/BlockEntityRegistry.hpp` - 方块实体注册表
- `common/entity/serialization/EntityDeserializer.hpp` - 实体反序列化
- `common/world/storage/core/SaveFormat.hpp` - 存档格式信息

### 下游依赖（谁使用了本目录）

- `SingleLevelStorageManager` - 通过 `JavaAnvilBackend` 间接调用本目录的读取器
- 外来存档读取流程：`SaveFormatDetector` 检测格式 → `SingleLevelStorageManager::open()` 创建 backend → backend 调用本目录读取器

## 容易踩的坑

### 区域文件格式 (.mca)

- 8192 字节头部：1024 条偏移记录 + 1024 条时间戳记录
- 偏移记录 4 字节：3 字节扇区偏移（大端序）+ 1 字节扇区数
- 区块数据格式：4 字节长度 + 1 字节压缩类型 + 压缩数据
- 压缩类型：1=GZip, 2=ZLib, 3=Uncompressed，**ZLib 最常见**

### Java 位压缩格式

- 使用 `long[]` 数组（64 位有符号）存储位压缩索引
- `bitsPerEntry = max(4, ceil(log2(palette.size())))`
- 解包公式：`index[i] = (data[i / valuesPerLong] >> ((i % valuesPerLong) * bitsPerEntry)) & mask`
- **注意**：Java 1.13+ 使用 padded 格式（每个 long 独立），旧版使用 compact 格式

### Java 生物群系路径分支

`JavaColumnReader` 需要处理多版本路径：
- 旧版 `ByteArray Biomes`（256 字节，2D）
- 1.15+ `Biomes[1024]`（int 数组，3D 采样）
- 1.18+ `sections[].biomes.palette + data`（section 级 palette）

### 1.17+ 实体分离

- `DATA_VERSION_ENTITIES_SEPARATED = 2724`（21w43a）起实体数据独立存储于 `entities/` 目录
- `JavaWorldReader` 负责在 world 层合并 `region/` 与 `entities/` 数据
- 合并逻辑：同时存在时合并 Entities 列表；仅 entities region 时构造最小列 NBT

### 区块坐标校验

- `JavaColumnReader` 会校验 NBT 中的 `xPos/zPos` 与请求坐标是否一致
- 不一致时仅警告不拒绝，因为某些工具导出的存档可能存在偏移

### Status 过滤

- 未完成的区块（Status 为 `empty`、`structure_starts` 等低级状态）会被跳过
- 返回空 `optional` 表示该区块不应加载

### 高度图格式

- Java 版使用 9 位编码每个高度值（支持 -512 到 2047 范围）
- 解包后需要加上维度感知的高度偏移（主世界 -64，下界 0，末地 -64）
- Java 存储值语义为 `Y+1-minY`（相对维度最低 Y，且为最高方块 Y+1），加偏移后得到绝对 `Y+1`
- 高度图类型：`WORLD_SURFACE`、`OCEAN_FLOOR`、`MOTION_BLOCKING`、`MOTION_BLOCKING_NO_LEAVES`、`LIGHT_BLOCKING`、`WORLD_SURFACE_WG`、`OCEAN_FLOOR_WG`
- 加载路径：`JavaColumnReader::_readHeightmaps` → `applyHeightmapArray`（long array）或旧版 `HeightMap` int[256] 数组
- 写入 `ChunkData` 使用 `setHeightmapFromStorage`（绕过 `_isOpaque` 判定，整列写入；`updateHeightmap` + nullptr state 是 no-op）

### 维度特定处理

- `JavaColumnReader` 接收 `DimensionId` 参数，通过 `DimensionType::fromId()` 获取维度属性
- Section Y 范围校验：根据维度的 `minHeight/maxHeight` 过滤越界 section（主世界 -4~19，下界 0~7）
- 天空光照门控：非天空光照维度（下界、末地）跳过 `SkyLight` 数据加载
- 高度图偏移：使用维度感知的 `minHeight` 而非全局 `MIN_BUILD_HEIGHT`
- 生物群系 section 基准：使用维度感知的 `minHeight` 计算旧版 3D 生物群系的 baseSectionY
