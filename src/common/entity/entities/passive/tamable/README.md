# 可驯服动物

可被玩家驯服的动物实体。

## 文件列表

| 文件 | 说明 |
|------|------|
| TameableEntity.hpp/cpp | 可驯服实体基类 |

## 继承

```
MobEntity
└── CreatureEntity
    └── AgeableEntity
        └── AnimalEntity
            └── TameableEntity
                ├── WolfEntity (狼)
                ├── CatEntity (猫)
                ├── OcelotEntity (豹猫)
                └── ParrotEntity (鹦鹉)
```

## 特性

### 驯服系统
- `isTamed()` - 检查是否驯服
- `setTamed(bool)` - 设置驯服状态
- `getOwnerId()` - 获取主人ID
- `setOwnerId(u64)` - 设置主人

### 坐下/站起
- `isSitting()` - 检查是否坐下
- `setSitting(bool)` - 设置坐下状态
- `toggleSitting()` - 切换坐下状态

### 愤怒系统
- 实现 `IAngerable` 接口
- `setAttackTarget()` - 设置攻击目标
- `isAngry()` - 检查是否愤怒

## 使用示例

```cpp
// 驯服狼
if (!wolf->isTamed() && player->hasItem(Items::BONE)) {
    if (random.nextFloat() < 0.33f) {
        wolf->setTamed(true);
        wolf->setOwnerId(player->entityId());
        // 显示爱心效果
    }
}

// 命令坐下
if (wolf->isOwner(player->entityId())) {
    wolf->toggleSitting();
}

// 设置攻击目标
wolf->setAttackTarget(enemy);
```

## AI目标

| Goal | 优先级 | 说明 |
|------|--------|------|
| SitGoal | 1 | 坐下时保持不动 |
| FollowOwnerGoal | 3 | 跟随主人 |
| BegGoal | 7 | 乞求食物 |
| OwnerHurtByTargetGoal | - | 主人被攻击时反击 |
| OwnerHurtTargetGoal | - | 攻击主人攻击的目标 |
