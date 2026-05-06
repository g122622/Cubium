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

## NBT 序列化

MerchantOffer 支持 NBT 序列化，格式参考 MC 1.16.5：

| NBT 键 | 类型 | 说明 |
|--------|------|------|
| `buy` | Compound | 第一买入物品 |
| `buyB` | Compound | 第二买入物品（可选） |
| `sell` | Compound | 卖出物品 |
| `uses` | Int | 已使用次数 |
| `maxUses` | Int | 最大使用次数 |
| `xp` | Int | 交易经验 |
| `priceMultiplier` | Float | 价格乘数 |
| `specialPrice` | Int | 特殊价格修正 |
| `demand` | Int | 需求修正 |
| `restocksToday` | Int | 今日补货次数 |
| `lastRestock` | Long | 上次补货时间 |

```cpp
// 序列化
nbt::tags::compound_tag tag;
offer.serialize(tag);

// 反序列化
MerchantOffer offer = MerchantOffer::deserialize(tag);
```

## 交易价格计算

最终价格 = 基础价格 + 特殊价格修正

- `getAdjustedBuyPrice()` - 获取调整后的买入价格
- `setSpecialPrice(price)` - 设置特殊价格修正（来自流言）
- `applyDemand(demandBonus)` - 应用需求调整

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
