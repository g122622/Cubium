# 序列化器 (Serializer)

负责将游戏对象与 NBT 格式互转。

## 文件说明

| 文件 | 职责 |
|------|------|
| `ChunkSerializer.hpp/cpp` | 区块数据序列化，处理方块、生物群系、高度图等 |
| `PlayerSerializer.hpp/cpp` | 玩家数据序列化，处理位置、物品栏、效果等 |
| `LevelDataSerializer.hpp/cpp` | 世界元数据序列化，处理 level.dat |
| `EntitySerializer.hpp/cpp` | 通用实体序列化，处理实体基础数据 |
| `BlockEntitySerializer.hpp/cpp` | 方块实体序列化，处理箱子、熔炉等 |

## NBT 结构参考

### 区块 NBT 结构
```
CompoundTag (root)
├── DataVersion: Int
└── Level: CompoundTag
    ├── xPos: Int
    ├── zPos: Int
    ├── LastUpdate: Long
    ├── InhabitedTime: Long
    ├── Status: String
    ├── Sections: ListTag
    │   └── CompoundTag (每个段)
    │       ├── Y: Byte
    │       ├── Palette: ListTag
    │       ├── BlockStates: LongArrayTag
    │       ├── BlockLight: ByteArrayTag
    │       └── SkyLight: ByteArrayTag
    ├── Biomes: IntArrayTag
    ├── Heightmaps: CompoundTag
    ├── Entities: ListTag
    ├── TileEntities: ListTag
    ├── TileTicks: ListTag
    ├── LiquidTicks: ListTag
    └── Structures: CompoundTag
```

### 玩家 NBT 结构
```
CompoundTag (root)
├── DataVersion: Int
├── UUID: IntArrayTag (2个int)
├── Dimension: Int
├── Pos: ListTag (3个double)
├── Rotation: ListTag (2个float)
├── Motion: ListTag (3个double)
├── Health: Float
├── foodLevel: Int
├── XpLevel: Int
├── XpTotal: Int
├── abilities: CompoundTag
├── Inventory: ListTag
├── EnderItems: ListTag
└── Attributes: ListTag
```

## 容易踩的坑

1. **数据版本**：所有序列化数据必须包含 DataVersion 字段
2. **调色板压缩**：区块方块数据使用调色板压缩，不是直接存储 ID
3. **字节序**：NBT 使用大端序
4. **列表类型**：NBT ListTag 中所有元素必须是相同类型
