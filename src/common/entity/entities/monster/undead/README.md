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

骷髅类实体采用 MC 1.16.5 的 `setCombatTask()` 模式来动态选择战斗目标：

#### 战斗目标切换机制

```
AbstractSkeletonEntity
├── m_rangedAttackGoal (RangedBowAttackGoal)
├── m_meleeAttackGoal (MeleeAttackGoal)
└── setCombatTask() -> 根据装备选择战斗目标
    ├── 持弓 -> 添加 RangedBowAttackGoal (优先级 4)
    └── 无弓 -> 添加 MeleeAttackGoal (优先级 4)
```

**设计要点：**
- `registerGoals()` 只注册非战斗目标（移动、看向、目标选择等）
- `setCombatTask()` 在构造函数末尾调用，动态添加战斗目标
- 子类（如 WitherSkeletonEntity）重写 `setCombatTask()` 选择不同的战斗方式

#### SkeletonEntity - 骷髅
| 特性 | 说明 |
|------|------|
| 远程 | 使用弓箭进行远程攻击（RangedBowAttackGoal） |
| 燃烧 | 在阳光下燃烧 |
| 掉落 | 掉落骨头和箭矢 |
| 战斗 | 会拉开距离射击、走位 |

#### StrayEntity - 流浪者
| 特性 | 说明 |
|------|------|
| 雪地 | 在雪地生物群系生成 |
| 减速 | 箭矢造成迟缓效果 |
| 装备 | 生成时穿戴破旧装备 |
| 掉落 | 掉落迟缓之箭 |
| 远程 | 继承父类的远程攻击（RangedBowAttackGoal） |

#### WitherSkeletonEntity - 凋灵骷髅
| 特性 | 说明 |
|------|------|
| 下界 | 在下界要塞生成 |
| 近战 | 使用石剑进行近战攻击（MeleeAttackGoal） |
| 凋零 | 攻击造成凋零效果（200 ticks = 10 秒） |
| 高攻 | 攻击伤害 4.0（普通骷髅 2.0） |
| 免疫 | 免疫凋零效果 |
| 阳光 | 不在阳光下燃烧 |
| 仇恨 | 主动攻击猪灵 |
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

## 溺水转化系统 (ZombieEntity)

僵尸在水中浸泡足够长时间后会转化为溺尸。这是 MC 1.16.5 的核心机制。

### 转化条件

| 条件 | 值 |
|------|-----|
| 开始转化 | 在水中 600 ticks (30秒) |
| 转化时间 | 300 ticks (15秒) |
| 触发条件 | 头部在水中 |

### 转化流程

```cpp
void ZombieEntity::updateDrowning() {
    // 每tick检查
    if (isInWater() && shouldDrown()) {
        m_inWaterTime++;
        if (m_inWaterTime >= 600 && !m_converting) {
            startDrowning(300);  // 开始15秒转化
        }
    } else {
        m_inWaterTime = 0;  // 重置计时
    }

    // 转化倒计时
    if (m_converting && m_conversionTime > 0) {
        m_conversionTime--;
        if (m_conversionTime <= 0) {
            convertToDrowned();  // 完成转化
        }
    }
}
```

### convertToDrowned() 实现

当僵尸完成溺水转化时，会调用 `convertToDrowned()` 方法：

1. **创建溺尸**: 从 EntityRegistry 获取 `DrownedEntity` 类型并创建新实例
2. **复制位置**: 位置和旋转角度
3. **复制生命值**: 按比例保留生命值
4. **复制装备**: 所有 6 个装备槽位（主手、副手、头盔、胸甲、护腿、靴子）
5. **复制婴儿状态**: 婴儿僵尸转化为婴儿溺尸
6. **复制自定义名称**: 名称和可见性
7. **复制持久化状态**: 命名牌命名的实体会保留
8. **清理**: 清空原僵尸装备（防止死亡掉落）
9. **音效**: 播放 `ENTITY_ZOMBIE_CONVERTED_TO_DROWNED`
10. **事件**: 播放世界事件 1040
11. **移除**: 标记原僵尸为移除状态

参考 MC 1.16.5 `ZombieEntity.onDrowned()` 和 `func_234341_c_()`

### 子类覆盖

- **HuskEntity**: 重写 `shouldDrown()` 返回 `true`，先变成普通僵尸再变成溺尸
- **DrownedEntity**: 重写 `shouldDrown()` 返回 `false`，不会再次转化
- **ZombieVillagerEntity**: 重写 `shouldDrown()` 返回 `false`，不会变成溺尸

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
