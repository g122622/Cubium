# 交易系统 (Trade System)

本目录实现村民和流浪商人的交易系统。

## 目录结构

```
trade/
├── Merchant.hpp              # 商人接口
├── MerchantOffer.hpp/cpp     # 单个交易优惠
├── MerchantOffers.hpp/cpp    # 交易列表（在Merchant.hpp中定义）
├── VillagerTrades.hpp/cpp    # 村民交易配方表（TODO）
├── WanderingTraderTrades.hpp/cpp # 流浪商人交易表（TODO）
└── README.md                 # 本文档
```

## 核心类

### MerchantOffer

单个交易项：

```cpp
// 创建交易：1个绿宝石 → 1个面包
MerchantOffer offer(
    ItemStack(Items::EMERALD, 1),
    ItemStack(Items::BREAD, 1),
    12,   // 最大使用次数
    2,    // 经验
    1.0f  // 价格乘数
);

// 检查是否可交易
if (offer.canAccept(playerItem)) {
    offer.apply(player, villager);
}

// 补货
offer.restock();
```

### MerchantOffers

交易列表：

```cpp
MerchantOffers offers;
offers.addOffer(std::make_unique<MerchantOffer>(...));

// 获取交易
MerchantOffer* offer = offers.getOffer(0);

// 批量补货
offers.restockAll();

// 基于声誉调整价格
offers.updatePrices(0.8f);  // 20%折扣
```

## 交易价格计算

最终价格 = 基础价格 × 声誉修正 × 价格乘数 + 需求调整

- 声誉修正：0.5（完全信任）~ 1.5（完全不信任）
- 需求调整：频繁交易同一物品会提高价格

## 与MC Java对齐

- 参考 MC 1.16.5 `MerchantOffer`, `MerchantOffers`
- 每日补货限制：2次
- 需求系统：动态价格调整
- 经验系统：交易给村民增加经验

## TODO

- [ ] 实现VillagerTrades（所有职业的交易配方）
- [ ] 实现WanderingTraderTrades
- [ ] 集成到VillagerEntity
- [ ] 实现交易UI
