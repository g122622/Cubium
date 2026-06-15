# 村民实体 (Villager Entities)

本目录包含村民和流浪商人实体的实现。

## 目录结构树

```
villager/
├── AbstractVillagerEntity.hpp/cpp  # 抽象村民基类，交易系统和繁殖意愿
├── VillagerEntity.hpp/cpp          # 村民实体 + VillagerData + WanderingTraderEntity
├── ProfessionMapping.hpp/cpp       # 职业-工作站POI映射工具类
└── README.md                       # 本文档
```

## 内部模块关系

```
AbstractVillagerEntity (抽象村民基类)
├── 继承自 AgeableEntity (可成长实体)
├── 实现 INamedContainerProvider (交易界面)
├── 实现 IMerchant (商人接口)
│   ├── getOffers() / setOffers() / overrideOffers()
│   ├── startTrading() / stopTrading() / isTrading()
│   ├── notifyTrade() / notifyTradeUpdated()
│   ├── getVillagerXp() / overrideXp() / addExperience()
│   ├── showProgressBar() / canRestock() / isClientSide()
│   ├── stillValid() / asEntity()
│   └── getTradingPlayer() — 当前交易玩家
├── 持有 MerchantOffers (交易列表)
├── 持有 IInventory (库存)
├── 纯虚方法: rewardTradeXp() — 子类实现经验奖励逻辑
│
├── VillagerEntity (村民)
│   ├── 持有 VillagerData (职业/类型/等级/经验)
│   ├── 持有 Brain<VillagerEntity> (AI大脑)
│   ├── 持有 ProfessionMapping (职业-工作站映射)
│   ├── rewardTradeXp() — 增加经验，检测升级，生成经验球（3+random(4)，升级+5）
│   ├── _handleTradeReputation() — 交易完成后更新村庄声望 + 开心村民粒子
│   ├── _increaseMerchantCareer() — 为当前等级追加新交易（不再次升级）
│   ├── shouldRestock() — 补货判定（跨天检测 + 每日2次限制 + needsRestock检查）
│   ├── restock() — 执行补货（updateDemandAll + restockAll + 记录时间 + 增加计数）
│   ├── needsToRestock() — 检查是否有交易被使用过（uses > 0）
│   ├── allowedToRestock() — 每日补货许可（首次允许，第二次需间隔2400tick）
│   ├── _catchUpDemand() — 跨天需求补偿（昨日少于2次补货时补偿）
│   ├── 功能: 职业系统、工作站点、睡眠系统、流言传播
│   ├── tick()中处理: 升级计时器（40 tick后升级+再生效果）、交易声望更新
│   ├── setLastHurtBy()覆写: 被玩家攻击时广播VillagerAngry粒子(status 13)+添加MinorNegative流言
│   └── tick()中处理: 袭击期间RaidManager检测，广播VillagerSplash粒子(status 42, 1/100概率/tick)
│
└── WanderingTraderEntity (流浪商人)
    ├── rewardTradeXp() — 生成经验球给玩家（3+random(4)，无升级加成）
    └── 功能: 随机生成、固定交易、消失倒计时、贸易羊驼
```

## 上下游外部依赖关系

**上游依赖（本目录依赖的模块）**：
- `entity/core/` - Entity, LivingEntity, MobEntity, AgeableEntity 基类
- `entity/ai/brain/` - Brain 系统、Memory、Sensor、Task、Schedule
- `entity/inventory/INamedContainerProvider.hpp` - 容器界面接口
- `entity/inventory/container/MerchantContainerMenu.hpp` - 交易容器菜单
- `entity/experience/ExperienceDropHandler.hpp` - 经验球生成工具类
- `world/village/trade/` - MerchantOffers, MerchantOffer, IMerchant 交易系统
- `world/village/poi/` - PointOfInterestType 工作站 POI 类型
- `world/village/raid/` - RaidManager 袭击管理器（村民tick()中检测袭击并广播恐慌粒子）
- `world/blockentity/core/SimpleInventory.hpp` - 简单库存实现
- `world/IWorld.hpp` - 世界接口（onVillagerTrade 事件桥接）

**下游依赖（依赖本目录的模块）**：
- `server/world/ServerWorld.hpp` - 村民生成的世界管理
- `server/network/` - 村民交易界面同步
- `client/renderer/entity/` - 村民渲染器
- `entity/entities/monster/undead/ZombieVillagerEntity.hpp` - 僵尸村民治愈后转换

## 交易系统交互流程

1. 玩家与村民交互 → `AbstractVillagerEntity::createMenu()` 创建 `MerchantContainerMenu`
2. `createMenu()` 内部创建 `MerchantContainer`（3格交易容器）并添加到菜单
3. 玩家在支付槽放入物品 → `MerchantContainer::updateSellItem()` 自动匹配交易
4. 玩家从结果槽取出物品 → `MerchantResultSlot::onTake()` 执行交易
5. 交易执行：`offer.take(buyA, buyB)` 扣除物品 → `merchant.notifyTrade()` 增加使用次数和经验
6. 关闭界面 → `MerchantContainerMenu::removed()` 返还支付槽物品并调用 `merchant.stopTrading()`

**经验奖励链**：`onTake()` → `notifyTrade()` → `rewardTradeXp()` → `addVillagerExperience()`

> ⚠️ **注意**：`notifyTrade()` 内部已调用 `rewardTradeXp()`，`onTake()` 不应再次添加经验，否则会导致经验翻倍。

### 经验球生成逻辑

- **VillagerEntity::rewardTradeXp()**：
  1. 记录升级前等级 `prevLevel`
  2. 调用 `addVillagerExperience(offer.getXp())` 增加村民经验
  3. 若 `offer.shouldRewardExp()` 且世界存在，生成经验球：基础值 `3 + random(0~3)`（3~6）
  4. 若 `m_villagerData.level() > prevLevel`（本次交易导致升级），经验球值额外 +5（8~11）
  5. 升级时设置 `m_updateMerchantTimer = 40` 和 `m_increaseProfessionLevelOnUpdate = true`
  6. 通过 `ExperienceDropHandler::spawnExperienceOrbs()` 在村民位置上方0.5格生成经验球

  升级计时器在 `tick()` 中消费：
  - 仅在非交易状态（`!isTrading()`）时递减
  - 计时器到期时，若 `m_increaseProfessionLevelOnUpdate` 为 true，调用 `_increaseMerchantCareer()` 升级并追加新等级交易
  - 无论是否升级，计时器到期时给予村民再生效果 I（200 tick = 10秒）

- **WanderingTraderEntity::rewardTradeXp()**：
  1. 若 `offer.shouldRewardExp()` 且世界存在，生成经验球：`3 + random(0~3)`（3~6）
  2. 没有升级系统，无额外+5加成
  3. 通过 `ExperienceDropHandler::spawnExperienceOrbs()` 在商人位置上方0.5格生成经验球

## 容易踩的坑

### 职业与工作站映射

**Workstation 枚举已废弃**：`VillagerEntity::Workstation` 枚举不应使用，统一使用 `ProfessionMapping` 类通过 `PointOfInterestType` 进行职业-工作站映射：

```cpp
// 正确做法
auto poiType = ProfessionMapping::getWorkstationPOI(VillagerProfession::Farmer);
auto profession = ProfessionMapping::getProfessionFromPOI(poiType);
// 获取所有可获取的工作站POI类型（无职业村民搜索工作站时使用）
const auto& workstations = ProfessionMapping::getAcquirableWorkstations();
```

**WorkStationSensor 使用职业映射**：`WorkStationSensor::update()` 不再硬编码搜索 Smoker，而是根据村民职业动态确定 POI 类型。有职业村民搜索对应工作站，无职业村民搜索所有可获取工作站类型，傻子村民不搜索。

**LookForJobSiteGoal 已实现 POI 搜索**：无职业村民通过 `ProfessionMapping::getAcquirableWorkstations()` 搜索最近空闲工作站，到达工作站后根据 POI 类型自动分配职业（参考 MC 原版 `AssignProfessionFromJobSite`）。

### Brain 系统管理村民行为

村民使用 Brain 系统而非传统 Goal 系统控制行为：
- **日程系统**：通过 `Schedule::VILLAGER_DEFAULT` 定义每日活动安排
- **活动切换**：`Brain::updateActivity()` 每 20 tick 检查一次活动
- **睡眠行为**：由 `SleepAtNightGoal` 在 REST 活动期间自动执行
- **工作行为**：由 `WorkAtJobSiteGoal` 在 WORK 活动期间执行

不要尝试手动触发村民的睡眠或工作，这些由 Brain 系统自动管理。

### 夜间时间判断

夜间时间范围：**12542 - 23459 tick**（MC 1.16.5），不要使用其他值。

### Brain 记忆模块

村民睡眠相关记忆：
- `HOME`: 床位位置（GlobalPos，包含维度信息）
- `LAST_SLEPT`: 上次睡眠时间
- `LAST_WOKEN`: 上次醒来时间

### 物品拾取限制

村民可拾取物品列表固定（参考 MC 1.16.5 `VillagerEntity.func_230293_i_()`）：
- 繁殖食物：面包、土豆、胡萝卜、甜菜根
- 农民额外拾取：小麦、小麦种子、甜菜根种子、骨粉

### 食物点数系统

VillagerEntity 提供基于库存食物点数的分享和繁殖判断：
- `foodPoints()` 映射：面包=4点、土豆=1点、胡萝卜=1点、甜菜根=1点
- `countFoodPointsInInventory()` — 计算库存食物点数总和
- `hasExcessFood()` — 食物点数 >= 24 时返回 true（村民愿意分享食物）
- `wantsMoreFood()` — 食物点数 < 12 时返回 true（村民需要更多食物）
- `EXCESS_FOOD_THRESHOLD = 24` / `WANTS_MORE_FOOD_THRESHOLD = 12`

食物点数仅统计繁殖物品（面包、土豆、胡萝卜、甜菜根），不包含小麦和种子。

### 职业等级经验

| 等级 | 名称 | 升级所需累计经验 |
|------|------|------------------|
| 1 | 新手 | - |
| 2 | 学徒 | 10 |
| 3 | 老手 | 70 |
| 4 | 专家 | 150 |
| 5 | 大师 | 250 |

### 流浪商人与村民的区别

- 流浪商人没有职业系统，交易列表固定
- 流浪商人不繁殖，有消失倒计时
- `WanderingTraderEntity::getTradingLevel()` 始终返回 0
- 流浪商人的 `rewardTradeXp()` 生成经验球（3~6），但没有升级加成
- 流浪商人的 `showProgressBar()` 返回 true（但等级始终为0）

### 交易经验奖励

- `VillagerEntity::rewardTradeXp()` 调用 `addVillagerExperience(offer.getXp())` 增加村民数据中的经验，并通过 `ExperienceDropHandler::spawnExperienceOrbs()` 生成经验球
- `AbstractVillagerEntity::notifyTrade()` 调用 `rewardTradeXp()` 并增加交易使用次数，同时通过 `IWorld::onVillagerTrade()` 发布 `VillagerTradeEvent` 触发成就/进度和统计更新
- **不要在交易流程中重复调用经验奖励方法**，经验已在 `notifyTrade()` 链中统一处理

### 交易声望更新

交易完成后，`m_lastTradedPlayer` 在 `rewardTradeXp()` 中设置，在下一个 `tick()` 中消费：
- `_handleTradeReputation()` 更新村庄声望：`VillageGossipType::Trading`，每次交易 +1（最大累积100次，声望影响 +2）
- 处理完成后 `m_lastTradedPlayer` 置空，确保每笔交易只触发一次
- 交易成功后通过 `broadcastEntityStatus(VillagerHappy)` 广播开心村民粒子效果

### 升级计时器

- `m_updateMerchantTimer`：40 tick（2秒）倒计时，仅在非交易状态递减
- `m_increaseProfessionLevelOnUpdate`：计时器到期时是否需要补充交易
- `m_prevLevelBeforeTrade`：记录升级前等级，用于多级升级时生成所有中间等级交易
- 计时器到期时：为所有新等级追加交易（`_increaseMerchantCareer()`），给予再生效果 I
- `_increaseMerchantCareer()` **不再次升级**（等级已由 `addExperience()` 正确递增）
- 多级升级处理：若一次交易从1级升到3级，会为2级和3级都生成交易并追加（循环 `prevLevel+1` 到 `currentLevel`）
- `_increaseMerchantCareer()` 仅追加新等级的交易，不替换现有交易（保留使用次数和需求状态）

### stillValid 距离检测

- `AbstractVillagerEntity::stillValid()` 使用 `distanceSqTo()` 与平方阈值 `64.0f` 比较（等价于 8 格距离）
- 避免使用 `distanceTo()` 进行距离检测，`distanceSqTo()` 无需计算平方根，性能更优

### setLastHurtBy 覆写（被攻击时的愤怒反应）

- `VillagerEntity` 覆写了 `LivingEntity::setLastHurtBy()`，当村民被玩家攻击时触发两个效果：
  1. 广播 `VillagerAngry` 粒子（status 13），客户端渲染愤怒粒子效果
  2. 向攻击者添加 `MinorNegative` 流言（`VillageGossipType::MinorNegative`），降低交易价格
- 覆写方法中**必须调用基类实现** `LivingEntity::setLastHurtBy()`，否则 `lastHurtByPlayer` 等基类字段不会更新
- 仅在攻击者为玩家时触发流言和粒子，非玩家攻击者只走基类逻辑

### 袭击恐慌粒子（Raid Panic）

- `VillagerEntity::tick()` 中检查 `RaidManager` 是否存在活跃袭击
- 当村民所在区域有活跃袭击时，每 tick 有 1/100 概率广播 `VillagerSplash` 粒子（status 42）
- 依赖 `IWorld::raidManager()` 获取袭击管理器，客户端世界返回 `nullptr`（不触发）
- 对应 MC 1.16.5 中村民在袭击期间显示汗滴/飞溅粒子的行为

## 参考

- MC 1.16.5 VillagerEntity
- MC 1.16.5 AbstractVillagerEntity
- MC 1.16.5 VillagerData
- MC 1.16.5 VillagerProfession
- MC 1.16.5 WanderingTraderEntity
