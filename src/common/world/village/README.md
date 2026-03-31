# 村庄系统 (Village System)

本目录实现了Minecraft村庄系统的核心功能，包括POI管理、村庄数据、流言系统和袭击事件。

## 目录结构

```
village/
├── Village.hpp/cpp              # 村庄数据管理
├── VillageManager.hpp/cpp       # 村庄管理器（世界级）
├── VillageGossip.hpp/cpp        # 流言系统
├── VillageGossipType.hpp/cpp    # 流言类型枚举
├── VillageSiege.hpp/cpp         # 僵尸围村（TODO）
├── README.md                     # 本文档
│
├── poi/                          # POI子系统
│   ├── PointOfInterest.hpp/cpp   # POI数据
│   ├── PointOfInterestType.hpp/cpp # POI类型
│   ├── PointOfInterestStorage.hpp/cpp # POI存储
│   └── README.md
│
├── raid/                         # 袭击子系统（TODO）
│   ├── Raid.hpp/cpp
│   ├── RaidManager.hpp/cpp
│   ├── RaidWave.hpp/cpp
│   ├── RaiderType.hpp
│   └── README.md
│
└── trade/                        # 交易子系统（TODO）
    ├── Merchant.hpp
    ├── MerchantOffer.hpp/cpp
    ├── MerchantOffers.hpp/cpp
    ├── VillagerTrades.hpp/cpp
    └── README.md
```

## 核心类

### Village

村庄数据管理类，负责：
- 村庄边界计算（基于床位和工作站分布）
- 村民列表管理
- 床位和工作站计数
- 流言/声誉系统
- 铃铛（聚集点）管理

```cpp
Village village(centerPos);
village.addVillager(villagerId);
village.recalculateBounds(poiStorage);
bool canBreed = village.canBreed();  // 是否有足够床位繁殖
```

### VillageManager

世界级村庄管理器，负责：
- 村庄生命周期管理
- 村民与村庄的关联
- POI注册和管理
- 区块加载/卸载回调

```cpp
VillageManager manager(serverWorld);

// 区块加载时
manager.onChunkLoaded(x, z);

// 村民加入时
manager.onVillagerJoin(villagerId, pos);

// 方块放置时（可能创建新POI）
manager.onBlockPlaced(pos, blockId);
```

### VillageGossipManager

流言管理器，影响交易价格：
- 正面流言（交易、治愈）降低价格
- 负面流言（攻击村民）提高价格
- 流言随时间衰减

```cpp
// 治愈僵尸村民
village.addGossip(playerId, VillageGossipType::MajorPositive, 1);

// 获取价格修正（声誉+1000 → 价格0.5倍）
f32 modifier = village.getPriceModifier(playerId);
```

## 与MC Java对齐

- 参考 MC 1.16.5 `Village`, `VillageManager`, `GossipContainer`
- 声誉范围：[-1000, +1000]
- 价格修正：[0.5, 1.5]
- 流言衰减：每日衰减一定比例

## TODO

- [ ] 实现袭击系统 (raid/)
- [ ] 实现交易系统 (trade/)
- [ ] 实现僵尸围村 (VillageSiege)
- [ ] 集成到ServerWorld
- [ ] 编写单元测试
