# 亡灵生物模块

包含所有亡灵类敌对生物的实现。亡灵生物共享以下特性：
- 在阳光下燃烧（部分变种除外）
- 受到治疗药水伤害
- 受到伤害药水治疗
- 免疫中毒和凋零效果
- 被亡灵杀手附魔造成额外伤害

## 目录结构

```
undead/
├── AbstractSkeletonEntity.hpp/cpp  # 骷髅基类
├── SkeletonEntity.hpp/cpp           # 骷髅
├── StrayEntity.hpp/cpp              # 流浪者
├── WitherSkeletonEntity.hpp/cpp     # 凋灵骷髅
├── ZombieEntity.hpp/cpp             # 僵尸基类
├── HuskEntity.hpp/cpp               # 尸壳
├── DrownedEntity.hpp/cpp            # 溺尸
└── ZombieVillagerEntity.hpp/cpp     # 僵尸村民
```

## 继承层次

```
Entity
└── LivingEntity
    └── MobEntity
        └── CreatureEntity
            └── MonsterEntity
                ├── AbstractSkeletonEntity (骷髅基类)
                │   ├── SkeletonEntity
                │   ├── StrayEntity
                │   └── WitherSkeletonEntity
                └── ZombieEntity (僵尸基类)
                    ├── HuskEntity
                    ├── DrownedEntity
                    └── ZombieVillagerEntity
```

## 实体详细说明

### 僵尸类 (ZombieEntity)

#### ZombieEntity - 僵尸
| 特性 | 说明 |
|------|------|
| 燃烧 | 在阳光下燃烧 |
| 破门 | 可以破坏木门（困难难度） |
| 增援 | 被攻击时有概率召唤增援 |
| 感染 | 杀死村民会将其转化为僵尸村民 |
| 转化 | 在水中会转化为溺尸 |

#### HuskEntity - 尸壳
| 特性 | 说明 |
|------|------|
| 沙漠 | 在沙漠生物群系生成 |
| 免疫 | 免疫阳光下燃烧 |
| 脱水 | 攻击造成脱水效果 |
| 转化 | 在水中会先变成普通僵尸，再变成溺尸 |

#### DrownedEntity - 溺尸
| 特性 | 说明 |
|------|------|
| 水生 | 在水中生成，可在水中呼吸 |
| 武器 | 可以生成时手持三叉戟或钓鱼竿 |
| 攻击 | 远程投掷三叉戟或近战攻击 |
| 掉落 | 掉落铜锭和三叉戟 |

#### ZombieVillagerEntity - 僵尸村民
| 特性 | 说明 |
|------|------|
| 来源 | 村民被僵尸杀死后转化 |
| 职业 | 保留村民的职业和等级信息 |
| 治愈 | 可通过虚弱药水 + 金苹果治愈回村民 |
| 掉落 | 治愈后保留绑定诅咒装备 |

### 骷髅类 (AbstractSkeletonEntity)

#### SkeletonEntity - 骷髅
| 特性 | 说明 |
|------|------|
| 远程 | 使用弓箭进行远程攻击 |
| 燃烧 | 在阳光下燃烧 |
| 掉落 | 掉落骨头和箭矢 |
| 战斗 | 会拉开距离射击 |

#### StrayEntity - 流浪者
| 特性 | 说明 |
|------|------|
| 雪地 | 在雪地生物群系生成 |
| 减速 | 箭矢造成迟缓效果 |
| 装备 | 生成时穿戴破旧装备 |
| 掉落 | 掉落迟缓之箭 |

#### WitherSkeletonEntity - 凋灵骷髅
| 特性 | 说明 |
|------|------|
| 下界 | 在下界要塞生成 |
| 凋零 | 攻击造成凋零效果 |
| 高攻 | 攻击伤害极高 |
| 掉落 | 掉落煤炭、骨头，稀有掉落凋灵骷髅头颅 |

## 治愈系统 (ZombieVillagerEntity)

僵尸村民可以通过以下步骤治愈：

### 治愈流程

1. **施加虚弱效果**: 玩家对僵尸村民使用喷溅型虚弱药水
2. **喂食金苹果**: 玩家对有虚弱效果的僵尸村民使用金苹果
3. **等待治愈**: 治愈时间 3600-6000 ticks (3-5 分钟)

### 治愈加速

| 加速因素 | 效果 |
|----------|------|
| 铁栏杆 | 4x4x4 范围内，每个有 30% 概率加速 |
| 床 | 4x4x4 范围内，每个有 30% 概率加速 |
| 力量效果 | 每级减少 10% 治愈时间 |

### 数据同步参数

| 参数名 | 类型 | 说明 |
|--------|------|------|
| CONVERTING_PARAM | bool | 是否正在治愈 |
| VILLAGER_TYPE_PARAM | i32 | 村民类型 |
| VILLAGER_PROFESSION_PARAM | i32 | 村民职业 |
| VILLAGER_LEVEL_PARAM | i32 | 村民交易等级 |

### 关键方法

```cpp
// 开始治愈
void startConverting(const std::string& starterUuid, i32 time = -1);

// 取消治愈
void stopConverting();

// 完成治愈，变为村民
void finishConverting();

// 获取治愈进度（每tick减少的时间）
i32 getConversionProgress() const;

// 设置村民数据
void setVillagerData(const entity::VillagerData& data);
```

### 治愈后效果

- 僵尸村民变成对应职业的村民
- 保留职业、等级和经验
- 治愈后有 10 秒恶心效果
- 绑定诅咒装备转移到村民身上
- 触发 CURED_ZOMBIE_VILLAGER 成就

## 属性值

| 实体 | 生命值 | 攻击伤害 | 移动速度 |
|------|--------|----------|----------|
| ZombieEntity | 20.0 | 3.0 | 0.23 |
| HuskEntity | 20.0 | 3.0 | 0.23 |
| DrownedEntity | 20.0 | 3.0 | 0.23 |
| ZombieVillagerEntity | 20.0 | 3.0 | 0.23 |
| SkeletonEntity | 20.0 | 2.0 | 0.25 |
| StrayEntity | 20.0 | 2.0 | 0.25 |
| WitherSkeletonEntity | 20.0 | 4.0 | 0.25 |

## 声音事件

| 事件 | 描述 |
|------|------|
| ENTITY_ZOMBIE_AMBIENT | 僵尸环境音 |
| ENTITY_ZOMBIE_HURT | 僵尸受伤 |
| ENTITY_ZOMBIE_DEATH | 僵尸死亡 |
| ENTITY_ZOMBIE_STEP | 僵尸脚步声 |
| ENTITY_ZOMBIE_INFECT | 村民感染 |
| ENTITY_ZOMBIE_VILLAGER_CURE | 僵尸村民治愈 |
| ENTITY_SKELETON_AMBIENT | 骷髅环境音 |
| ENTITY_SKELETON_HURT | 骷髅受伤 |
| ENTITY_SKELETON_DEATH | 骷髅死亡 |
| ENTITY_SKELETON_STEP | 骷髅脚步声 |
| ENTITY_SKELETON_SHOOT | 骷髅射箭 |

## 实现状态

| 实体 | 基础框架 | AI 目标 | 特殊行为 | 治愈系统 |
|------|----------|---------|----------|----------|
| ZombieEntity | ✅ | ✅ | ✅ | N/A |
| HuskEntity | ✅ | ✅ | ✅ | N/A |
| DrownedEntity | ✅ | ⏳ | ⏳ | N/A |
| ZombieVillagerEntity | ✅ | ✅ | ✅ | ✅ |
| SkeletonEntity | ✅ | ✅ | ✅ | N/A |
| StrayEntity | ✅ | ✅ | ✅ | N/A |
| WitherSkeletonEntity | ✅ | ✅ | ✅ | N/A |

## 测试用例

- `tests/common/entity/entities/monster/ZombieVillagerEntityTest.cpp`
  - 基础属性测试（眼睛高度）
  - 治愈状态测试（开始/停止/进度）
  - 村民数据测试（类型/职业/等级/经验）
  - 消失规则测试（治愈中/有经验）

## 参考

- MC 1.16.5 `net.minecraft.entity.monster.ZombieEntity`
- MC 1.16.5 `net.minecraft.entity.monster.ZombieVillagerEntity`
- MC 1.16.5 `net.minecraft.entity.monster.AbstractSkeletonEntity`
