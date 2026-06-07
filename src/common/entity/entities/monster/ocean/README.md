# 海洋怪物模块 (Ocean Monsters)

包含海洋生物群系中的敌对生物实现。

## 目录结构

```
ocean/
├── GuardianEntity.hpp/cpp       # 守卫者实体，激光攻击、目标选择（玩家+鱿鱼）
└── ElderGuardianEntity.hpp/cpp  # 远古守卫者实体，Boss级、挖掘疲劳效果
```

## 内部模块关系

```
ElderGuardianEntity 继承 GuardianEntity
       │
       └── 复用 GuardianEntity 的激光攻击、AI 目标配置
```

**GuardianEntity 核心组件**：
- `GuardianAttackGoal`：激光攻击目标（充能 60 tick + 冷却 20 tick）
- `NearestAttackableTargetGoal`：目标选择（玩家 + 鱿鱼，距离 > 3 格）
- 尖刺动画：40 tick 周期切换

## 上下游外部依赖关系

**上游依赖（本模块依赖）**：
- `MonsterEntity`：敌对生物基类
- `GuardianAttackGoal`：激光攻击 AI 目标（`entity/ai/goal/goals/special/`）
- `NearestAttackableTargetGoal`：目标选择 AI 目标（`entity/ai/goal/goals/target/`）
- `Attributes`：属性系统（攻击伤害、生命值等）
- `SoundEvents`：音效事件（水中/陆地不同音效）

**下游依赖（依赖本模块）**：
- 实体注册系统：实体类型注册
- 海底神殿结构生成：守卫者生成

## 继承关系

```
Entity
└── LivingEntity
    └── MobEntity
        └── MonsterEntity
            └── GuardianEntity
                └── ElderGuardianEntity (远古守卫者)
```

## 容易踩的坑

1. **目标选择距离限制**：守卫者的目标选择谓词要求距离 > 3 格才会攻击，太近的目标不会被选中。这是 MC 1.16.5 的设计。

2. **水中/陆地音效不同**：守卫者在水中和陆地上使用不同的音效（`ENTITY_GUARDIAN_AMBIENT` vs `ENTITY_GUARDIAN_AMBIENT_LAND`），`getAmbientSound()` 需要根据 `isInWater()` 返回正确音效。

3. **远古守卫者属性覆盖**：`ElderGuardianEntity::registerAttributes()` 会覆盖父类的属性值，必须先调用 `GuardianEntity::registerAttributes()`。

4. **尖刺动画在 tick 中更新**：尖刺状态是 40 tick 周期自动切换，不是由外部事件触发。

5. **挖掘疲劳效果范围**：远古守卫者的 `MINING_FATIGUE_RANGE = 50.0f` 是半径，每 `FATIGUE_INTERVAL = 600` tick（30秒）应用一次效果。

## 参考

- MC 1.16.5 `net.minecraft.entity.monster.GuardianEntity`
