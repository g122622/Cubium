# 被动/中立生物模块

包含所有被动生物和中立生物的实现。

## 目录结构

```
passive/
├── basic/          # 普通动物（猪、牛、羊、鸡等）
├── tamable/        # 可驯服动物（狼、猫、鹦鹉等）
├── special/        # 特殊动物（狐狸、熊猫、北极熊等）
├── horse/          # 马类（马、驴、骡、羊驼等）
├── fish/           # 鱼类（鳕鱼、鲑鱼、河豚、热带鱼）
├── water/          # 水生生物（鱿鱼、海豚）
├── ambient/        # 环境生物（蝙蝠）
└── golem/          # 傀儡（铁傀儡、雪傀儡）
```

## 继承层次

```
MobEntity
└── CreatureEntity
    └── AgeableEntity
        └── AnimalEntity
            ├── TameableEntity (可驯服)
            │   ├── WolfEntity (狼)
            │   ├── CatEntity (猫)
            │   ├── OcelotEntity (豹猫)
            │   └── ParrotEntity (鹦鹉)
            ├── PigEntity (猪)
            ├── CowEntity (牛)
            ├── SheepEntity (羊)
            ├── ChickenEntity (鸡)
            └── ...
```

## 动物行为

### 基础行为优先级
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | PanicGoal | 受伤/着火时逃跑 |
| 2 | BreedGoal | 繁殖 |
| 3 | TemptGoal | 被食物诱惑 |
| 4 | FollowParentGoal | 幼体跟随父母 |
| 5 | WaterAvoidingRandomWalkingGoal | 避开水随机行走 |
| 6 | LookAtGoal | 看向玩家 |
| 7 | LookRandomlyGoal | 随机看向 |

### 可驯服动物额外行为
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | SitGoal | 坐下（当被命令时） |
| 3 | FollowOwnerGoal | 跟随主人 |

## 繁殖系统

```cpp
// 检查是否可以繁殖
if (animal->canBreedWith(partner)) {
    // 生成幼体
    auto baby = animal->spawnBaby(partner);
    // 重置爱心状态
    animal->resetLove();
    partner->resetLove();
}
```

## 驯服系统

```cpp
// 驯服动物
if (!wolf->isTamed() && player->hasItem(Items::BONE)) {
    if (random.nextFloat() < 0.33f) {
        wolf->setTamed(true);
        wolf->setOwnerId(player->entityId());
    }
}

// 命令坐下
if (wolf->isOwner(player->entityId())) {
    wolf->toggleSitting();
}
```
