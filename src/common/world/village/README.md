# 村庄系统 (Village System)

本目录实现了Minecraft村庄系统的核心功能，包括POI管理、村庄数据、流言系统和袭击事件。

## 目录结构

```
village/
├── Village.hpp/cpp              # 村庄数据管理（边界、村民、声誉、tick更新）
├── VillageManager.hpp/cpp       # 村庄管理器（世界级，生命周期管理）
├── VillageGossip.hpp/cpp        # 流言管理器（声誉系统）
├── VillageGossipType.hpp/cpp    # 流言类型枚举
├── README.md                     # 本文档
│
├── poi/                          # POI子系统
│   ├── PointOfInterest.hpp/cpp   # POI数据（位置、类型、票据）
│   ├── PointOfInterestType.hpp/cpp # POI类型枚举（床位、工作站、钟）
│   ├── PointOfInterestStorage.hpp/cpp # POI存储（区块级索引、空间查询）
│   └── README.md
│
├── raid/                         # 袭击子系统
│   ├── RaiderType.hpp/cpp        # 掠夺者类型枚举
│   ├── Raid.hpp/cpp              # 单次袭击事件（波次、生成、胜负判定）
│   ├── RaidManager.hpp/cpp       # 世界级袭击管理器
│   └── README.md                 # 注：RaidWave是Raid.hpp内定义的结构体
│
└── trade/                        # 交易子系统
    ├── Merchant.hpp              # 商人接口
    ├── MerchantOffer.hpp/cpp     # 单个交易项
    ├── MerchantOffers.hpp/cpp    # 交易列表
    ├── VillagerTrades.hpp/cpp    # 村民交易配方表
    ├── WanderingTraderTrades.hpp/cpp # 流浪商人交易表
    └── README.md
```

**注意**: 僵尸围村系统 (VillageSiege) 位于 `src/server/world/spawn/VillageSiege.hpp/cpp`

## 内部模块关系

```
VillageManager
    ├── Village（多个实例）
    │       ├── VillageGossipManager（声誉系统）
    │       └── 依赖 poi::PointOfInterestStorage（查询床位/工作站）
    ├── poi::PointOfInterestStorage
    └── 与 raid::RaidManager 协作（袭击触发时）

raid::RaidManager
    └── Raid（多个实例）
           ├── RaiderType（掠夺者类型枚举）
           ├── RaidWave（波次运行时数据，定义在Raid.hpp中）
           └── RaidParticipant（参与者贡献记录）

trade::（独立模块，被VillagerEntity使用）
    ├── MerchantOffer
    ├── MerchantOffers
    └── VillagerTrades / WanderingTraderTrades（交易配方表）
```

**核心数据流**：
- `VillageManager` 持有全局唯一的 `PointOfInterestStorage`
- `Village` 通过 POI 存储计算边界、统计床位/工作站
- `VillageGossipManager` 管理 `Village` 内的玩家声誉
- `RaidManager` 独立运行，但通过 `Village` 获取袭击位置和状态

## 上下游外部依赖关系

**上游依赖（本目录依赖）：**
- `common/core/Types.hpp` - 基础类型（u8/u16/u32/u64/i32/i64/f32、EntityId、Uuid）
- `common/world/block/BlockPos.hpp` - 方块位置
- `common/world/chunk/ChunkPos.hpp` - 区块坐标
- `common/world/IWorld.hpp` - 世界接口（实体生成、tick、难度查询）
- `common/util/nbt/` - NBT 序列化
- `common/util/math/random/Random.hpp` - 随机数生成
- `entity/ai/brain/sensor/Sensors.hpp` - 村民传感器

**下游依赖（被依赖）：**
- `server/world/ServerWorld.hpp` - 持有 `VillageManager` 和 `RaidManager` 实例
- `server/world/spawn/VillageSiege.hpp` - 僵尸围村，查询村庄位置
- `entity/entities/villager/VillagerEntity.cpp` - 村民实体，查找床位和工作站
- `entity/ai/goal/goals/villager/VillagerGoals.cpp` - 村民 AI 目标
- `server/advancement/AdvancementEventHandler.hpp` - 英雄成就触发

## 容易踩的坑

### Village ID 管理
`Village::setId()` 仅由 `VillageManager` 调用。创建村庄后必须先设置 ID 才能进行后续操作。

### 村庄指针可能为空
`Raid` 构造时传入的 `Village*` 可能为 `nullptr`，调用方必须在关键操作前检查 `isValid()` 或做防御性判断。村庄被销毁时关联的袭击会自动失效。

### POI 统计更新间隔
`VillageConfig::POI_STAT_UPDATE_INTERVAL = 1200` tick（约 1 分钟），POI 计数不是实时的，刚放置的床位可能需要等待下一个更新周期。

### 村民超时机制
`VillageConfig::VILLAGER_TIMEOUT = 6000` tick（5 分钟），村民离开村庄范围后不会立即被移除，而是等待超时。

### 流言衰减与声誉范围
声誉范围是 `[-1000, +1000]`，流言会随时间衰减。声誉 +1000 对应价格 0.5 倍，声誉 -1000 对应价格 1.5 倍。

### 价格修正因子计算
`getPriceModifier()` 返回 `[0.5, 1.5]` 范围，高声誉 = 低价格。计算公式：`clamp(1.0 - reputation/1000.0, 0.5, 1.5)`

### 区块到村庄映射
`VillageManager::m_chunkToVillages` 用于快速查找区块内的村庄。村民加入/区块加载/卸载时需要正确更新此映射。

### 袭击者实体追踪
`Raid::raiders()` 返回的 `EntityId` 列表只表示追踪 ID，不保证实体仍存在于世界中。使用前需通过 `IWorld::getEntity()` 验证。

### 波次间隔单位
`RaidConfig::WAVE_INTERVAL = 1200` 是 tick 数（约 60 秒），不是毫秒或秒。

### 难度影响袭击行为
和平难度下袭击直接停止。波次数由难度决定：简单 3 波、普通 5 波、困难 7 波。

### 不祥之兆等级与波次
总波次 = 基础波次 + `max(0, badOmenLevel - 1)`。等级 1 不增加波次，等级 2+ 才会增加。
