# POI（兴趣点）系统

本目录实现了村庄系统中的兴趣点(Point of Interest)管理，包括床位、工作站、钟等特殊方块。

## 目录结构

```
poi/
├── PointOfInterestType.hpp/cpp   # POI类型枚举和辅助函数（床位、工作站、其他POI）
├── PointOfInterest.hpp/cpp       # POI数据结构（位置、类型、票据/占用状态）
├── PointOfInterestStorage.hpp/cpp # POI存储和查询（区块级索引、空间查询、线程安全）
└── README.md                      # 本文档
```

## 内部模块关系

```
PointOfInterestType  ←──  PointOfInterest  ←──  PointOfInterestStorage
     (枚举基础)              (数据结构)                (存储管理)
```

- **PointOfInterestType**：定义所有 POI 类型的枚举（床位16种、工作站12种、其他4种）
- **PointOfInterest**：单个 POI 实例，包含位置、类型、票据列表（占用状态）
- **PointOfInterestStorage**：管理所有 POI 的注册/注销/查询，使用区块级索引实现高效空间查询

## 上下游外部依赖关系

**本目录依赖**：
- `common/core/Types.hpp` - 基础类型（u8/u16/u32/u64/i32/i64/f32 等）
- `common/world/block/BlockPos.hpp` - 方块位置
- `common/world/chunk/ChunkPos.hpp` - 区块坐标
- `common/util/nbt/` - NBT 序列化

**被依赖方**：
- `world/village/Village.hpp/cpp` - 村庄类，使用 POI 计算边界和统计
- `world/village/VillageManager.hpp/cpp` - 村庄管理器，管理 POI 注册
- `entity/entities/villager/VillagerEntity.cpp` - 村民实体，查找床位和工作站
- `entity/entities/villager/ProfessionMapping.hpp` - 职业与工作站映射
- `entity/ai/goal/goals/villager/VillagerGoals.cpp` - 村民 AI 目标
- `entity/ai/brain/sensor/Sensors.cpp` - 传感器

## 容易踩的坑

1. **线程安全**：`PointOfInterestStorage` 所有公共方法都是线程安全的（内部使用 `std::mutex`），可在多线程环境下直接调用。

2. **票据系统**：POI 使用票据(tickets)管理占用，而非简单的布尔标志。一个 POI 可被多个实体占用（取决于 `maxTickets`），例如钟可以多个村民共享。调用 `acquire()` 前应先检查 `canAcquire()`。

3. **区块级索引**：POI 按区块分组存储，空间查询会优先按区块过滤。使用 `onChunkLoaded()`/`onChunkUnloaded()` 通知 Storage 区块状态变化。

4. **POI 类型与职业映射**：`PointOfInterestType` 中的工作站类型与村民职业的映射关系由 `ProfessionMapping` 类统一管理，不要在此处添加新的映射逻辑。

5. **NBT 序列化**：`PointOfInterest` 和 `PointOfInterestStorage` 都支持 NBT 序列化，用于存档保存。区块卸载时 POI 数据不会自动保存，需由上层 `VillageManager` 处理。
