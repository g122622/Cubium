# 交易系统 (Trade System)

本目录实现村民和流浪商人的交易系统。

## 目录结构

```
trade/
├── Merchant.hpp               # 商人接口 IMerchant 及交易列表管理类 MerchantOffers
├── MerchantOffer.hpp/cpp      # 单个交易项（买入/卖出物品、使用次数、价格调整、NBT序列化）
├── MerchantOffers.cpp         # MerchantOffers 类实现（批量补货、价格更新）
├── VillagerTrades.hpp/cpp     # 村民交易配方表（按职业和等级分组，静态工厂方法生成交易）
├── WanderingTraderTrades.hpp/cpp # 流浪商人交易表（普通/稀有交易池，随机选取）
└── README.md                  # 本文档
```

## 内部模块关系

```
IMerchant（商人接口）
    └── 持有 MerchantOffers（交易列表管理）
           └── 持有多个 MerchantOffer（单个交易项）

VillagerTrades / WanderingTraderTrades
    └── 静态工厂方法，生成 MerchantOffers 实例
```

`MerchantOffer` 是核心数据结构，封装单笔交易；`MerchantOffers` 管理交易列表；`VillagerTrades` 和 `WanderingTraderTrades` 提供各职业/商人的交易配方工厂。

## 上下游外部依赖关系

**上游依赖（本目录依赖）：**
- `common/core/Types.hpp` - 基础类型（i32、u64、f32 等）
- `common/item/core/ItemStack.hpp` - 物品堆
- `common/util/nbt/Nbt.hpp` - NBT 序列化
- `common/entity/entities/villager/AbstractVillagerEntity.hpp` - 村民实体基类（VillagerTrades 依赖）

**下游依赖（被依赖）：**
- `entity/entities/villager/VillagerEntity.cpp` - 村民交易
- `entity/entities/wandering_trader/WanderingTraderEntity.cpp` - 流浪商人交易
- `server/network/` - 交易数据包

## 容易踩的坑

### 价格调整公式
最终价格 = 基础价格 + 特殊价格修正（来自流言/需求）。`getAdjustedBuyPrice()` 返回调整后的买入价格。

### 每日补货限制
每日最多补货 2 次。补货时机由村民 AI 控制，不是自动的。

### 需求系统
需求会动态影响价格。`applyDemand()` 应用需求调整，需求值来自交易次数统计。

### NBT 字段
MerchantOffer 的 NBT 字段包括：`buy`、`buyB`（可选第二买入）、`sell`、`uses`、`maxUses`、`xp`、`priceMultiplier`、`specialPrice`、`demand`、`restocksToday`、`lastRestock`。

### 交易工厂模式
`VillagerTrades` 和 `WanderingTraderTrades` 使用工厂函数而非静态配方，以支持动态价格调整。工厂函数签名不同：村民交易需要 `demand` 参数，流浪商人不需要。
