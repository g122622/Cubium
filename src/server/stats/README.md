# 统计系统

本目录实现 Minecraft 1.16.5 风格的玩家统计系统。

## 目录结构

```
src/server/stats/
├── StatType.hpp/cpp           # 统计类型枚举和工具函数
├── Stat.hpp/cpp               # 统计项类（类型+ID+值）
├── StatRegistry.hpp/cpp       # 统计注册表（单例，注册所有内置统计）
├── StatisticsManager.hpp/cpp  # 玩家统计管理器（NBT序列化、增删改查）
└── README.md                  # 本文档
```

## 内部模块关系

```
StatType ──→ Stat ──→ StatisticsManager
    │                      ↑
    └──→ StatRegistry ─────┘
```

- `StatType` 定义统计类型枚举，被所有模块依赖
- `Stat` 表示单个统计项，依赖 `StatType`
- `StatRegistry` 注册所有内置统计，依赖 `StatType`
- `StatisticsManager` 管理玩家统计数据，依赖 `Stat` 和 `StatType`

## 上下游外部依赖关系

**上游依赖（本目录使用的）：**
- `common/core/Types.hpp` - 基本类型
- `common/resource/ResourceLocation.hpp` - 资源位置
- `common/util/nbt/Nbt.hpp` - NBT 序列化
- `common/stats/Stats.hpp` - 自定义统计常量（与 StatRegistry 注册名对应）

**下游依赖（使用本目录的）：**
- `server/player/ServerPlayer.hpp` - 每个玩家持有 `StatisticsManager` 实例
- `common/entity/entities/player/Player.hpp` - `awardCustomStat()` 虚方法（基类空实现）
- 各种方块（BarrelBlock、BrewingStandBlock 等）- 调用 `player.awardCustomStat()`
- 记分板系统 - 统计判据 `minecraft.{type}:{id}` 可用于记分板目标

## 容易踩的坑

- **统计ID格式**：完整统计ID格式为 `minecraft.{type}:{id}`，例如 `minecraft.mined:minecraft.stone`。注意冒号分隔类型和ID，而非点号。
- **零增量不创建条目**：`StatisticsManager::increment()` 对零增量不会创建新条目，避免无意义的存储开销。
- **脏数据标记**：修改统计后会自动标记 `dirty`，增量保存时需检查 `isDirty()` 以避免全量写入。
- **线程安全**：`StatisticsManager` 非线程安全，多线程访问需外部同步。`StatRegistry` 是单例，初始化时需确保单线程。
- **常量与注册名对应**：`common/stats/Stats.hpp` 中的常量字符串必须与 `StatRegistry._registerAllCustomStats()` 中注册的完全一致，否则统计不会生效。
- **awardCustomStat 客户端安全**：`Player::awardCustomStat()` 基类为空实现，客户端调用不会崩溃也不会更新统计；仅 `ServerPlayer` 重写版本实际更新 `StatisticsManager`。
