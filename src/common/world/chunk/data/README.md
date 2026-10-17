# 区块数据

## 目录结构树

```text
data/
├── BiomeContainer.hpp/cpp       # 区块生物群系采样数据
├── ChunkData.hpp/cpp            # 段、权威高度图、方块实体与光照数据
├── ChunkSection.hpp/cpp         # 段内方块调色板与非空气计数
├── Heightmap.hpp/cpp            # 高度图位存储、类型谓词与批量重建
├── IChunk.hpp                   # 区块数据访问接口
├── PalettedContainer.hpp/cpp    # 方块状态调色板与位压缩存储
└── light/                      # 区块光照数组
```

## 内部模块关系

`ChunkData` 实现 `IChunk`，持有 `ChunkSection`、生物群系和高度图。
`Heightmap::rebuild` 在一次列扫描中构建选中的高度图类型，跳过空段。
存档准备与生成阶段共用该算法；存档准备只补齐尚未初始化的类型。

## 上下游外部依赖关系

- 依赖公共坐标、世界尺寸、方块注册表、调色板和光照基础类型。
- 存储层在回传加载结果前准备高度图；生成器通过 `ChunkPrimer` 管理生成期副本。
- 服务端世界、光照与网络同步读取运行时 `ChunkData`，客户端也复用本目录的数据模型。

## 容易踩的坑

- 高度图保存最高方块的 Y+1；空列是 `Heightmap::NO_BLOCK_SENTINEL`，不能用 0 代替。
- 已初始化的存档高度图必须保留，补齐操作不能覆盖外来存档的真实高度值。
- 高度图重建需要独占尚未发布的区块，或持有能阻止方块写入的锁。
- 跳过空段依赖准确的非空气计数；修改调色板时必须同步维护计数。
- `Heightmap::getData()` 按值返回；需要保留结果时使用拥有值的局部变量。
