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
├── AbstractSkeletonEntity.hpp/cpp  # 骷髅基类（远程/近战切换）
├── SkeletonEntity.hpp/cpp          # 骷髅（弓箭远程攻击）
├── StrayEntity.hpp/cpp             # 流浪者（迟缓之箭）
├── WitherSkeletonEntity.hpp/cpp    # 凋灵骷髅（石剑近战、凋零效果）
├── ZombieEntity.hpp/cpp            # 僵尸基类（溺水转化、召唤援军）
├── HuskEntity.hpp/cpp              # 尸壳（沙漠变种、脱水攻击）
├── DrownedEntity.hpp/cpp           # 溺尸（水中生成、三叉戟）
└── ZombieVillagerEntity.hpp/cpp    # 僵尸村民（治愈系统）
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

## 内部模块关系

- **AbstractSkeletonEntity**：骷髅系基类，实现 `IRangedAttackMob` 接口，采用 `setCombatTask()` 模式动态选择远程/近战目标
- **ZombieEntity**：僵尸系基类，实现溺水转化、增援召唤、破门能力、婴儿状态、生成初始化
- **ZombieVillagerEntity**：继承 ZombieEntity，实现治愈系统（铁栏杆/床加速、力量效果加速）

各实体通过重写父类方法实现差异化行为：
- `shouldBurnInDaylight()`：HuskEntity/DrownedEntity 返回 false
- `shouldDrown()`：ZombieVillagerEntity 返回 false，DrownedEntity 返回 false
- `setCombatTask()`：WitherSkeletonEntity 重写使用近战，其他骷髅使用远程
- `finalizeSpawn()`：ZombieEntity 覆写，设置破门能力（概率 = specialMultiplier × 0.1）和万圣节南瓜头
- `populateDefaultEquipmentSlots()`：ZombieEntity 覆写，Hard 难度 5%/其他 1% 概率生成铁剑或铁锹

## 上下游外部依赖关系

### 上游依赖

| 依赖模块 | 用途 |
|---------|------|
| `entity/core` | Entity, LivingEntity, MobEntity, CreatureEntity, MonsterEntity 基类 |
| `entity/interfaces` | IRangedAttackMob 远程攻击接口 |
| `entity/ai/goal` | AI 目标系统（MeleeAttackGoal, RangedBowAttackGoal, BreakDoorGoal 等） |
| `entity/attribute` | Attributes 属性系统 |
| `entity/damage` | DamageSource 伤害系统 |
| `entity/effect` | 药水效果系统（饥饿、凋零、迟缓等） |
| `entity/entities/villager` | VillagerData 村民数据（职业、等级、经验） |
| `world/` | IWorld, BlockPos, BlockState 等世界接口 |

### 下游依赖

| 依赖方 | 用途 |
|-------|------|
| `entity/core/VanillaEntities` | 实体类型注册 |
| `world/spawn` | 怪物生成系统 |
| `client/renderer/entity` | 实体渲染器 |

## 容易踩的坑

### 骷髅类战斗目标切换

1. **setCombatTask() 调用时机**：`registerGoals()` 只注册非战斗目标，`setCombatTask()` 在构造函数末尾调用，根据装备动态添加战斗目标
2. **战斗目标优先级**：战斗目标使用优先级 4（`COMBAT_GOAL_PRIORITY`），确保非战斗目标（游泳、看向等）优先执行
3. **WitherSkeletonEntity 特殊处理**：凋灵骷髅重写 `setCombatTask()` 强制使用近战，因为默认生成时持石剑

### 僵尸类溺水转化

4. **shouldDrown() 重写**：HuskEntity 返回 true（先变僵尸再变溺尸），DrownedEntity 返回 false（已溺尸），ZombieVillagerEntity 返回 false（不会变溺尸）
5. **convertToDrowned() 数据保留**：必须保留位置、生命值比例、装备、婴儿状态、自定义名称、持久化状态，清空原僵尸装备防止死亡掉落
6. **转化时间常量**：水下 600 ticks（30秒）开始转化，转化持续 300 ticks（15秒）

### 僵尸村民治愈系统

7. **治愈时间**：基础时间 3600-6000 ticks（3-5分钟随机），使用 `startConverting()` 时可指定时间或 -1 随机
8. **加速检测范围**：4x4x4 范围内的铁栏杆和床，每个有 30% 概率加速，最多检测 14 个方块
9. **床加速特性**：所有 16 种颜色的床都有效，占用状态不影响，头部和脚部都有效
10. **消失规则**：正在治愈或有经验的僵尸村民不能消失，见 `canDespawn()` 实现

### 属性与尺寸

11. **婴儿僵尸**：通过 `isBaby()` 检查，尺寸和眼睛高度不同，速度有 50% 加成
12. **步进高度**：DrownedEntity 步进高度为 1.0f，可走上完整方块

### 数据同步

13. **ZombieVillagerEntity 数据参数**：`CONVERTING_PARAM`、`VILLAGER_TYPE_PARAM`、`VILLAGER_PROFESSION_PARAM`、`VILLAGER_LEVEL_PARAM` 需要在 `registerData()` 中注册
14. **客户端同步**：`syncMetadataFromDataManager()` 需要从 DataManager 读取数据更新本地状态
