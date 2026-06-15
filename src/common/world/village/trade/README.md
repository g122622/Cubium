# 交易系统 (Trade System)

本目录实现村民和流浪商人的交易系统。

## 目录结构

```
trade/
├── Merchant.hpp               # 商人接口 IMerchant 及交易列表管理类 MerchantOffers
├── MerchantOffer.hpp/cpp      # 单个交易项（买入/卖出物品、使用次数、价格调整、NBT序列化）
├── MerchantOffers.cpp         # MerchantOffers 类实现（批量补货、价格更新、物品匹配）
├── VillagerTrades.hpp/cpp     # 村民交易配方表（按职业和等级分组，静态工厂方法生成交易）
├── WanderingTraderTrades.hpp/cpp # 流浪商人交易表（普通/稀有交易池，随机选取）
└── README.md                  # 本文档
```

## 内部模块关系

```
IMerchant（商人接口）
    ├── getOffers() / setOffers() / overrideOffers() — 交易列表管理
    ├── startTrading() / stopTrading() / isTrading() — 交易生命周期
    ├── getTradingPlayer() — 当前交易玩家
    ├── notifyTrade(offer) — 交易执行通知（增加使用次数、奖励经验）
    ├── notifyTradeUpdated(resultStack) — 交易输入变化通知（播放音效）
    ├── getVillagerXp() / overrideXp() / addExperience() — 经验管理
    ├── showProgressBar() / canRestock() — 界面属性
    ├── isClientSide() — 客户端/服务端判断
    ├── stillValid(player) — 交易有效性检查
    └── asEntity() — 获取商人对应实体（用于播放音效等）

MerchantOffers（交易列表管理）
    ├── addOffer() / removeOffer() / getOffer() — 列表操作
    ├── getOfferFor(buyA, buyB, hint) — 按物品匹配交易（支持双支付槽顺序交换）
    ├── restockAll() — 批量补货
    ├── updateDemandAll() — 批量更新所有交易需求值
    ├── needsRestockAny() — 检查是否有交易需要补货（任意uses > 0）
    ├── resetDailyRestockAll() — 重置所有交易的每日补货计数
    └── updatePrices(modifier) — 基于声誉更新价格

MerchantOffer（单个交易项）
    ├── take(buyA, buyB) — 扣除支付物品（交易执行核心）
    ├── assemble() — 复制结果物品
    ├── isOutOfStock() — 是否售罄（uses >= maxUses）
    ├── needsRestock() — 是否需要补货（uses > 0）
    ├── updateDemand() — 更新需求值（demand += 2*uses - maxUses）
    ├── restock() — 补货（重置uses=0，restocksToday++）
    ├── resetDailyRestock() — 重置每日补货计数
    ├── getXp() / shouldRewardExp() — 经验奖励信息
    └── getBuyA() / getBuyB() / getSell() — 物品获取

VillagerTrades / WanderingTraderTrades
    └── 静态工厂方法，生成 MerchantOffers 实例
```

## 上下游外部依赖关系

**上游依赖（本目录依赖）：**
- `common/core/Types.hpp` - 基础类型（i32、u64、f32 等）
- `common/item/core/ItemStack.hpp` - 物品堆
- `common/util/nbt/Nbt.hpp` - NBT 序列化
- `common/entity/entities/villager/AbstractVillagerEntity.hpp` - 村民实体基类（VillagerTrades 依赖）

**下游依赖（被依赖）：**
- `entity/entities/villager/VillagerEntity.cpp` - 村民交易
- `entity/entities/villager/AbstractVillagerEntity.cpp` - 抽象村民基类（实现 IMerchant 接口）
- `entity/inventory/container/MerchantContainer.hpp/cpp` - 交易容器（3格支付/结果槽）
- `entity/inventory/container/MerchantContainerMenu.hpp/cpp` - 交易容器菜单
- `entity/inventory/container/MerchantResultSlot.hpp/cpp` - 交易结果槽
- `server/network/` - 交易数据包

## 交易匹配逻辑

`MerchantOffers::getOfferFor(buyA, buyB, hint)` 匹配规则：
1. 遍历所有交易报价，检查 `buyA` 和 `buyB` 是否与报价的买入物品匹配
2. 如果提供了 `hint`（玩家点击的交易索引），优先匹配该索引
3. 物品匹配使用 `ItemStack::isSameItem()` 方法，只比较物品类型，忽略数量
4. 如果 `buyB` 为空，则只匹配单物品交易

`MerchantContainer::updateSellItem()` 更新逻辑：
1. 优先使用原始顺序（buyA→报价buyA, buyB→报价buyB）匹配
2. 如果无匹配或售罄，尝试交换顺序（buyA→报价buyB, buyB→报价buyA）
3. 匹配成功且未售罄时，将 `offer.getSell().copy()` 放入结果槽
4. 匹配失败或售罄时，清空结果槽

## 交易执行流程

1. 玩家从结果槽取出物品 → `MerchantResultSlot::onTake()`
2. `offer.take(buyA, buyB)` — 扣除支付槽物品（尝试两种顺序）
3. `merchant.notifyTrade(offer)` — 通知商人交易完成
   - `offer.increaseUses()` — 增加交易使用次数
   - `rewardTradeXp(offer)` — 子类实现经验奖励
4. 更新支付槽物品为扣除后的剩余数量
5. **注意**：经验已在 `notifyTrade()` → `rewardTradeXp()` 链中统一处理，不应在 `onTake()` 中重复添加

## 容易踩的坑

### 价格调整公式
最终价格 = 基础价格 + 特殊价格修正（来自流言/需求）。`getAdjustedBuyPrice()` 返回调整后的买入价格。

### 每日补货限制
每日最多补货 2 次。补货时机由 `VillagerEntity::shouldRestock()` 控制，该逻辑包含：
- 跨天检测（距上次补货超过12000tick 或新游戏日开始）→ 重置每日补货次数并补偿需求
- 允许补货检查：首次补货总是允许，第二次需间隔2400tick
- 需要补货检查：任意交易被使用过（uses > 0）即触发补货
- 需求补偿：跨天时，若昨日补货少于2次，对每次未补货执行额外的 restockAll + updateDemandAll

### 需求系统
需求会动态影响价格。`updateDemand()` 计算公式：`demand = demand + uses - (maxUses - uses)`，即 `demand += 2*uses - maxUses`。
使用次数超过一半时需求增加（价格上涨），反之需求减少（价格下降）。更新后重新计算特殊价格：`specialPrice = demand * priceMultiplier`。

### NBT 字段
MerchantOffer 的 NBT 字段包括：`buy`、`buyB`（可选第二买入）、`sell`、`uses`、`maxUses`、`xp`、`priceMultiplier`、`specialPrice`、`demand`、`restocksToday`、`lastRestock`。

### 交易工厂模式
`VillagerTrades` 和 `WanderingTraderTrades` 使用工厂函数而非静态配方，以支持动态价格调整。工厂函数签名不同：村民交易需要 `demand` 参数，流浪商人不需要。

### 经验奖励注意事项
- `AbstractVillagerEntity::notifyTrade()` 内部已调用 `rewardTradeXp()`
- `VillagerEntity::rewardTradeXp()` 调用 `addVillagerExperience()`
- `WanderingTraderEntity::rewardTradeXp()` 为空实现
- **不要在交易结果槽的 `onTake()` 中重复调用经验奖励方法**，否则会导致经验翻倍

### take() 方法注意事项
`MerchantOffer::take(buyA, buyB)` 会尝试从传入的 ItemStack 中扣除物品：
- 返回 `true` 表示成功扣除（物品数量已减少）
- 返回 `false` 表示扣除失败（物品不匹配或数量不足）
- `MerchantResultSlot::onTake()` 中会尝试两种顺序：`take(buyA, buyB)` 和 `take(buyB, buyA)`
