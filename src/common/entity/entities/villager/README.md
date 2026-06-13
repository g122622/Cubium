# 村民实体 (Villager Entities)

本目录包含村民和流浪商人实体的实现。

## 目录结构树

```
villager/
├── AbstractVillagerEntity.hpp/cpp  # 抽象村民基类，交易系统和繁殖意愿
├── VillagerEntity.hpp/cpp          # 村民实体 + VillagerData + WanderingTraderEntity
├── ProfessionMapping.hpp/cpp       # 职业-工作站POI映射工具类
└── README.md                        # 本文档
```

## 内部模块关系

```
AbstractVillagerEntity (抽象村民基类)
├── 继承自 AgeableEntity (可成长实体)
├── 实现 INamedContainerProvider (交易界面)
├── 持有 MerchantOffers (交易列表)
├── 持有 IInventory (库存)
│
├── VillagerEntity (村民)
│   ├── 持有 VillagerData (职业/类型/等级/经验)
│   ├── 持有 Brain<VillagerEntity> (AI大脑)
│   ├── 持有 ProfessionMapping (职业-工作站映射)
│   └── 功能: 职业系统、工作站点、睡眠系统、流言传播
│
└── WanderingTraderEntity (流浪商人)
    └── 功能: 随机生成、固定交易、消失倒计时、贸易羊驼
```

## 上下游外部依赖关系

**上游依赖（本目录依赖的模块）**：
- `entity/core/` - Entity, LivingEntity, MobEntity, AgeableEntity 基类
- `entity/ai/brain/` - Brain 系统、Memory、Sensor、Task、Schedule
- `entity/inventory/INamedContainerProvider.hpp` - 容器界面接口
- `world/village/trade/` - MerchantOffers, MerchantOffer 交易系统
- `world/village/poi/` - PointOfInterestType 工作站 POI 类型
- `world/blockentity/core/SimpleInventory.hpp` - 简单库存实现

**下游依赖（依赖本目录的模块）**：
- `server/world/ServerWorld.hpp` - 村民生成的世界管理
- `server/network/` - 村民交易界面同步
- `client/renderer/entity/` - 村民渲染器
- `entity/entities/monster/undead/ZombieVillagerEntity.hpp` - 僵尸村民治愈后转换

## 容易踩的坑

### 职业与工作站映射

**Workstation 枚举已废弃**：`VillagerEntity::Workstation` 枚举不应使用，统一使用 `ProfessionMapping` 类通过 `PointOfInterestType` 进行职业-工作站映射：

```cpp
// 正确做法
auto poiType = ProfessionMapping::getWorkstationPOI(VillagerProfession::Farmer);
auto profession = ProfessionMapping::getProfessionFromPOI(poiType);
```

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

## 参考

- MC 1.16.5 VillagerEntity
- MC 1.16.5 AbstractVillagerEntity
- MC 1.16.5 VillagerData
- MC 1.16.5 VillagerProfession
- MC 1.16.5 WanderingTraderEntity
