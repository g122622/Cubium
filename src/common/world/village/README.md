# 村庄系统 (Village System)

本目录实现了Minecraft村庄系统的核心功能，包括POI管理、村庄数据、流言系统和袭击事件。

## 目录结构

```
village/
├── Village.hpp/cpp              # 村庄数据管理
├── VillageManager.hpp/cpp       # 村庄管理器（世界级）
├── VillageGossip.hpp/cpp        # 流言系统
├── VillageGossipType.hpp/cpp    # 流言类型枚举
├── README.md                     # 本文档
│
├── poi/                          # POI子系统
│   ├── PointOfInterest.hpp/cpp   # POI数据
│   ├── PointOfInterestType.hpp/cpp # POI类型
│   ├── PointOfInterestStorage.hpp/cpp # POI存储
│   └── README.md
│
├── raid/                         # 袭击子系统
│   ├── Raid.hpp/cpp
│   ├── RaidManager.hpp/cpp
│   ├── RaidWave.hpp/cpp
│   ├── RaiderType.hpp
│   └── README.md
│
└── trade/                        # 交易子系统
    ├── Merchant.hpp
    ├── MerchantOffer.hpp/cpp     # ✅ NBT序列化已实现
    ├── MerchantOffers.hpp/cpp
    ├── VillagerTrades.hpp/cpp    # ✅ 所有职业交易配方已实现
    └── README.md
```

**注意**: 僵尸围村系统 (VillageSiege) 位于 `src/server/world/spawn/VillageSiege.hpp/cpp`

## 核心类

### Village

村庄数据管理类，负责：
- 村庄边界计算（基于床位和工作站分布）
- 村民列表管理
- 床位和工作站计数
- 流言/声誉系统
- 铃铛（聚集点）管理
- **tick更新**：村民范围检查、POI统计更新、袭击状态检查

```cpp
Village village(centerPos);
village.addVillager(villagerId);
village.recalculateBounds(poiStorage);
bool canBreed = village.canBreed();  // 是否有足够床位繁殖

// tick 更新（每游戏刻调用）
village.tick(world, gameTime, &poiStorage);
```

#### Village::tick() 实现细节

村庄的 `tick()` 方法每游戏刻执行以下操作：

1. **流言衰减** - 通过 `VillageGossipManager::tick()` 更新流言系统
2. **村民范围检查** - 移除离开村庄范围超过5分钟的村民
3. **POI统计更新** - 每分钟更新床位、工作站计数和聚集点（钟）
4. **袭击状态检查** - 检查袭击是否结束（当村庄处于袭击状态时）

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
- 村民超时：6000 tick (5分钟) 离开村庄范围后移除
- POI统计更新：每1200 tick (1分钟)

## TODO

- [x] 实现 Village::tick() 村民范围检查
- [x] 实现 Village::tick() POI统计更新
- [x] 实现 Village::tick() 袭击状态检查
- [x] 实现袭击系统 (raid/)
- [x] 实现交易系统 (trade/) - MerchantOffer NBT序列化已完成
- [x] 实现僵尸围村 (VillageSiege) - 位于 `src/server/world/spawn/`
- [ ] 集成 VillageSiege 到 ServerWorld
- [ ] 编写单元测试
