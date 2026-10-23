# 交易系统 (Trade System)

本目录实现村民和流浪商人的交易系统。

## 目录结构

```
trade/
├── Merchant.hpp               # 商人接口 IMerchant 及交易列表管理类 MerchantOffers
├── ItemCost.hpp/cpp           # 支付代价（物品模板 + 数量 + potion_contents/custom_data 逐组件谓词）
├── MerchantOffer.hpp/cpp      # 单个交易项（双支付代价、卖出物品、使用次数、需求/特殊价格计价、NBT序列化）
├── MerchantOffers.cpp         # MerchantOffers 类实现（批量补货、需求更新、物品匹配）
├── TradeContext.hpp/cpp       # 交易生成上下文（世界 / 卖家村民 / 离线种子）
├── TradeRandom.hpp/cpp        # 可派生 / 可复现的交易随机源
├── VillagerTrades.hpp/cpp     # 村民交易表（职业 → 等级 → 条目），条目类型与全量交易数据
├── WanderingTraderTrades.hpp/cpp # 流浪商人交易表（两个独立抽取池）
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
    ├── getOfferFor(buyA, buyB, hint) — 按物品匹配交易（数量满足优先，其次只匹配物品种类）
    ├── restockAll() — 批量补货
    ├── updateDemandAll() — 批量更新所有交易需求值
    ├── needsRestockAny() — 检查是否有交易需要补货（任意uses > 0）
    ├── resetDailyRestockAll() — 重置所有交易的每日补货计数
    └── resetSpecialPrices() — 清除声誉/英雄效果带来的特殊价格偏移

ItemCost（支付代价）
    ├── test(stack) — 物品类型 + 数量 + 逐组件谓词
    ├── testComponentwise(stack) — 忽略数量，只看类型与组件
    └── withPotion() / withCustomData() — 追加上下文敏感的组件谓词

TradeListing（条目基类）
    └── getOffer(ctx, random) — 每次生成交易时重新抽取（附魔、染料、寻宝地图目标等）

VillagerTrades（村民交易表）
    ├── TRADES — 职业 → 等级 → 条目列表（收录原版全部条目）
    └── generateOffers(profession, level, ctx) — 生成单个等级的条目

WanderingTraderTrades（流浪商人交易表）
    ├── s_pools — 两个交易池，每池独立抽取 2 次（允许重复）
    └── generateOffers(ctx) — 逐池抽取并生成交易

MerchantOffer（单个交易项）
    ├── take(buyA, buyB) — 扣除支付物品（交易执行核心）
    ├── assemble() — 复制结果物品
    ├── isOutOfStock() — 是否售罄（uses >= maxUses）
    ├── needsRestock() — 是否需要补货（uses > 0）
    ├── updateDemand() — 更新需求值（demand += 2*uses - maxUses）
    ├── restock() — 补货（重置uses=0，restocksToday++）
    ├── resetDailyRestock() — 重置每日补货计数
    ├── getXp() / shouldRewardExp() — 经验奖励信息
    ├── swapCosts() — 支付槽反序命中后互换两条代价
    └── getCostA() / getCostB() / getBuyA() / getBuyB() / getSell() — 代价与物品获取
```

## 上下游外部依赖关系

**上游依赖（本目录依赖）：**
- `common/core/Types.hpp` - 基础类型（i32、u64、f32 等）
- `common/item/core/ItemStack.hpp` - 物品堆
- `common/util/nbt/Nbt.hpp` - NBT 序列化
- `common/entity/entities/villager/AbstractVillagerEntity.hpp` - 村民实体基类（村民类型）
- `common/item/` - 物品、附魔、药水、染料标签（条目生成时读取）
- `common/world/IWorld.hpp` - 世界随机源与最近结构查询（寻宝地图）

**下游依赖（被依赖）：**
- `entity/entities/villager/VillagerEntity.cpp` - 村民交易
- `entity/entities/villager/AbstractVillagerEntity.cpp` - 抽象村民基类（实现 IMerchant 接口）
- `entity/inventory/container/MerchantContainer.hpp/cpp` - 交易容器（3格支付/结果槽）
- `entity/inventory/container/MerchantContainerMenu.hpp/cpp` - 交易容器菜单
- `entity/inventory/container/MerchantResultSlot.hpp/cpp` - 交易结果槽
- `server/network/` - 交易数据包

## 交易匹配逻辑

`MerchantOffers::getOfferFor(buyA, buyB, hint)` 匹配规则：
1. 有 `hint`（玩家点击的交易索引）时优先该索引，只校验物品种类与组件（数量不足也返回，供界面展示）
2. 否则先找数量完全满足的交易（`canAccept`），再退回只匹配物品种类的交易
3. 物品匹配使用 `ItemCost::test`（类型 + 组件谓词）；数量由需求修正后的支付数量门控

`MerchantContainer::updateSellItem()` 更新逻辑：
1. 优先按常量槽顺序（槽 A → 代价 A、槽 B → 代价 B）匹配
2. 无匹配、售罄或支付不足时尝试交换；命中交换交易时调用 `offer.swapCosts()` 把代价与常量槽重新绑定
3. 只有支付物品足额（`canAccept`）时才把 `offer.getSell().copy()` 放入结果槽

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
支付数量 = clamp(基础数量 + floor(基础数量 × 需求 × 价格乘数) + 特殊价格修正, 1, 物品最大堆叠数)。
- **需求**只由使用次数决定（`demand += 2*uses - maxUses`），不写入特殊价格；
- **特殊价格**来自声誉与村庄英雄效果，由 `AbstractVillagerEntity::startTrading` 计算，`stopTrading` 时清零；
- 修正为负需求时需求加成的下限为 0，不会低于基础数量。

### 条目与生成时机
交易表保存的是"条目"而非成品交易：`TradeContext` 携带世界与卖家村民，每次生成时按随机流与村民类型
重新抽取（附魔等级、随机染料、寻宝地图目标、按生物群系分支）。离线场景（单元测试）用 `traderSeed` 构造
可复现的随机树。

### 每日补货限制
每日最多补货 2 次。补货时机由 `VillagerEntity::shouldRestock()` 控制，该逻辑包含：
- 跨天检测（距上次补货超过12000tick 或新游戏日开始）→ 重置每日补货次数并补偿需求
- 允许补货检查：首次补货总是允许，第二次需间隔2400tick
- 需要补货检查：任意交易被使用过（uses > 0）即触发补货
- 需求补偿：跨天时，若昨日补货少于2次，对每次未补货执行额外的 restockAll + updateDemandAll

### 需求系统
需求会动态影响价格。`updateDemand()` 计算公式：`demand = demand + uses - (maxUses - uses)`，即 `demand += 2*uses - maxUses`。
使用次数超过一半时需求增加（价格上涨），反之需求减少（价格下降）。需求加成在计价时按
`floor(基础数量 × demand × priceMultiplier)` 计算，与特殊价格互不影响。

### NBT 字段
MerchantOffer 的 NBT 字段包括：`buyA`、`buyB`（可选第二支付，物品堆的 Count 即代价数量，组件谓词随物品堆一起保存）、`sell`、`uses`、`maxUses`、`xp`、`priceMultiplier`、`specialPrice`、`demand`、`restocksToday`、`lastRestock`。

### 交易条目与随机数
交易表保存条目（`TradeListing`），生成时用 `TradeContext` + 随机流现场构造交易。有世界时随机数取自世界
随机流；离线（单元测试）时按 `traderSeed` 构造可复现的随机树。流浪商人的两个交易池各自独立抽取 2 次，
同一池内可能抽到同一条目两次。

### 村民类型分支
渔夫的船只、制图师的寻宝地图与旗帜按 `ctx.trader` 的村民类型分支；上游传入非村民实体（流浪商人）时
`ctx.trader` 为空，这些条目直接返回空交易。

### 寻宝地图
`TreasureMapListing` 已接入 `IWorld::findNearestMapStructure` 搜索目标结构，但地图创建与装饰标记接口尚在
重建，当前返回空交易（并记录警告），TODO 标志见类注释。

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
