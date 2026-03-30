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
};
```

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
