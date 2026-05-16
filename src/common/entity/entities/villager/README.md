# 村民实体 (Villager Entities)

本目录包含村民和流浪商人实体的实现。

## 目录结构

```
villager/
├── AbstractVillagerEntity.hpp/cpp  # 抽象村民基类
├── VillagerEntity.hpp/cpp          # 村民实体 + VillagerData
└── README.md                        # 本文档
```

## 继承层次

```
Entity
└── LivingEntity
    └── MobEntity
        └── CreatureEntity
            └── AgeableEntity
                └── AbstractVillagerEntity  # 抽象村民基类
                    ├── VillagerEntity       # 村民
                    └── WanderingTraderEntity # 流浪商人
```

## 实体列表

| 实体 | 描述 |
|------|------|
| VillagerEntity | 村民，可交易，有职业系统 |
| WanderingTraderEntity | 流浪商人，随机生成，交易固定 |

## 核心类设计

### VillagerData

村民数据类，存储：
- **VillagerType**: 村民类型（沙漠、丛林、平原等）
- **VillagerProfession**: 职业（农民、图书管理员等）
- **Level**: 等级（1-5）
- **Experience**: 经验值

```cpp
class VillagerData {
public:
    VillagerType type() const;
    VillagerProfession profession() const;
    i32 level() const;
    i32 experience() const;

    void setProfession(VillagerProfession profession);
    void addExperience(i32 amount);

    static i32 getExperienceForLevel(i32 level);
    static constexpr i32 getMaxLevel() { return 5; }
};
```

### AbstractVillagerEntity

抽象村民基类，提供：
- 交易系统（MerchantOffers）
- 库存管理
- 繁殖意愿

```cpp
class AbstractVillagerEntity : public AgeableEntity {
public:
    // 交易
    MerchantOffers* getOffers();
    void startTrading(Player* player);
    void stopTrading();

    // 经验
    i32 experience() const;
    void addExperience(i32 amount);

    // 库存
    Inventory& inventory();

    // 繁殖
    bool isWillingToBreed() const;
};
```

### VillagerEntity

村民实体，具有：
- 职业系统（15种职业）
- 工作站点绑定
- 交易升级系统
- 日程系统（工作、睡觉、社交）
- 物品拾取能力
- **睡眠系统**

```cpp
class VillagerEntity : public AbstractVillagerEntity {
public:
    // 职业
    VillagerProfession profession() const;
    void setProfession(VillagerProfession profession);

    // 等级
    i32 level() const;
    void addVillagerExperience(i32 amount);

    // 工作
    BlockCoord workStation() const;
    bool canWork() const;
    void work();
    void rest();

    // 物品拾取
    bool canPickUpItem(const ItemStack& itemStack) const;
    bool isBreedingItem(const ItemStack& itemStack) const;

    // 睡眠系统
    bool isSleeping() const;
    std::optional<BlockPos> getSleepingPosition() const;
    void startSleeping(BlockPos pos);
    void stopSleeping();
    bool isNightTime() const;
};
```

#### 物品拾取系统

村民可以拾取以下物品：

| 物品 | 用途 |
|------|------|
| 面包 (BREAD) | 繁殖食物、拾取 |
| 土豆 (POTATO) | 繁殖食物、拾取 |
| 胡萝卜 (CARROT) | 繁殖食物、拾取 |
| 甜菜根 (BEETROOT) | 繁殖食物、拾取 |
| 小麦 (WHEAT) | 拾取 |
| 小麦种子 (WHEAT_SEEDS) | 拾取 |
| 甜菜根种子 (BEETROOT_SEEDS) | 拾取 |

参考 MC 1.16.5 `VillagerEntity.func_230293_i_()`

## 职业系统

### VillagerProfession 枚举

| 职业 | 工作站点 |
|------|----------|
| None | 无 |
| Armorer | 高炉 |
| Butcher | 烟熏炉 |
| Cartographer | 制图台 |
| Cleric | 酿造台 |
| Farmer | 堆肥桶 |
| Fisherman | 木桶 |
| Fletcher | 制箭台 |
| Leatherworker | 炼药锅 |
| Librarian | 讲台 |
| Mason | 切石机 |
| Nitwit | 无（傻子村民） |
| Shepherd | 织布机 |
| Toolsmith | 锻造台 |
| Weaponsmith | 锻造台 |

### 等级系统

| 等级 | 名称 | 升级所需经验 |
|------|------|--------------|
| 1 | 新手 | - |
| 2 | 学徒 | 10 |
| 3 | 老手 | 70 |
| 4 | 专家 | 150 |
| 5 | 大师 | 250 |

### VillagerType 枚举

| 类型 | 生物群系 |
|------|----------|
| Desert | 沙漠 |
| Jungle | 丛林 |
| Plains | 平原 |
| Savanna | 热带草原 |
| Snow | 雪 |
| Swamp | 沼泽 |
| Taiga | 针叶林 |

## 交易系统

交易通过 `MerchantOffers` 和 `MerchantOffer` 类管理：

```cpp
// 创建交易
auto offer = std::make_unique<MerchantOffer>();
offer->setBuyItem1(ItemStack(Items::EMERALD, 1));
offer->setSellItem(ItemStack(Items::BREAD, 6));
offer->setMaxUses(16);

// 添加到村民
villager->getOffers()->add(std::move(offer));
```

## 睡眠系统

村民具有完整的睡眠系统，在夜间自动寻找床位并睡眠。

### 睡眠机制架构

村民的睡眠行为通过 Brain 系统自动管理，无需手动触发：

1. **Schedule 时间表**：
   - `Schedule::VILLAGER_DEFAULT` 定义村民的日常活动安排
   - 游戏时间 12000 ticks 时自动切换到 `Activity::REST` 活动

2. **Brain 活动切换**：
   - `Brain::updateActivity()` 每 20 tick 检查一次活动
   - 根据当前游戏时间自动切换活动

3. **AI 目标执行**：
   - `SleepAtNightGoal` 在 REST 活动期间检查睡眠条件
   - 自动查找床位、移动到床、开始睡眠

### 日程系统

村民的日程安排（MC 1.16.5）：

| 游戏时间 | 活动类型 | 行为 |
|----------|----------|------|
| 10 tick | IDLE | 空闲 |
| 2000 tick | WORK | 工作 |
| 9000 tick | MEET | 聚会 |
| 11000 tick | IDLE | 空闲 |
| 12000 tick | REST | 休息/睡眠 |

### 工作系统

村民的工作行为同样通过 Brain 系统和 AI Goal 自动管理：

1. **WorkAtJobSiteGoal**：
   - `shouldExecute()`: 检查是否是工作时间 (2000-9000 tick) 和是否有工作站点
   - `tick()`: 检查是否在工作站点附近，执行工作逻辑
   - 工作时增加村民经验值

2. **LookForJobSiteGoal**：
   - 自动寻找可用的工作站点
   - 通过 POI 系统查找职业对应的工作方块

### 睡眠状态管理

| 方法 | 描述 |
|------|------|
| `isSleeping()` | 检查村民是否正在睡眠 |
| `getSleepingPosition()` | 获取睡眠位置（床位坐标） |
| `startSleeping(BlockPos pos)` | 开始睡眠 |
| `stopSleeping()` | 停止睡眠 |
| `isNightTime()` | 检查是否是夜间时间 |

### 睡眠时间

夜间时间范围：12542 - 23459 tick（MC 1.16.5）

### 睡眠行为

1. **寻找床位**: 通过 POI 系统查找最近的可用床位
2. **绑定床位**: 将床位位置存储到 Brain 的 HOME 记忆
3. **移动到床**: 导航到床位位置
4. **开始睡眠**:
   - 设置睡眠姿态 (EntityPose::Sleeping)
   - 记录睡眠位置
   - 占用 POI 床位
5. **停止睡眠**:
   - 恢复站立姿态
   - 清除睡眠位置
   - 记录醒来时间

### POI 集成

睡眠系统与 POI 系统集成：
- 使用 `PointOfInterestStorage::findNearestFree()` 查找可用床位
- 使用 `PointOfInterestStorage::acquirePOI()` 占用床位
- 支持所有 16 种颜色的床

### Brain 记忆

睡眠相关记忆模块：
- `HOME`: 床位位置（GlobalPos，包含维度信息）
- `LAST_SLEPT`: 上次睡眠时间
- `LAST_WOKEN`: 上次醒来时间

参考 MC 1.16.5 VillagerEntity, SleepAtNightGoal

## 使用示例

### 创建村民

```cpp
auto villager = std::make_unique<VillagerEntity>(LegacyEntityType::Unknown, id);
villager->setVillagerType(VillagerType::Plains);
villager->setProfession(VillagerProfession::Farmer);
villager->setPosition(x, y, z);
world->spawnEntity(std::move(villager));
```

### 设置职业

```cpp
villager->setProfession(VillagerProfession::Librarian);
// 职业改变会重置等级和交易列表
```

### 增加经验

```cpp
// 玩家完成交易后
villager->addVillagerExperience(10);
// 经验达到阈值会自动升级
```

## 流浪商人

流浪商人是特殊的村民变体：

- 随机在世界生成
- 交易列表固定
- 不会繁殖
- 有两只贸易羊驼
- 一定时间后消失

```cpp
auto trader = std::make_unique<WanderingTraderEntity>(LegacyEntityType::Unknown, id);
trader->setDespawnDelay(48000);  // 40分钟后消失
trader->spawnLlamas();           // 生成羊驼
world->spawnEntity(std::move(trader));
```

## 参考

- MC 1.16.5 VillagerEntity
- MC 1.16.5 AbstractVillagerEntity
- MC 1.16.5 VillagerData
- MC 1.16.5 VillagerProfession
- MC 1.16.5 WanderingTraderEntity
