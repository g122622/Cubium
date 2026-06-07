# 交互类 AI 目标 (Interact Goals)

提供可驯服动物（狼、猫、鹦鹉等）特有的 AI 行为目标。

## 目录结构

```
interact/
├── TameableGoals.hpp/cpp       # 可驯服动物目标（FollowOwnerGoal, SitGoal, BegGoal）
└── LandOnOwnersShoulderGoal.hpp/cpp  # 肩膀乘坐目标（鹦鹉落到主人肩膀）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                    TameableGoals.hpp                        │
│  ┌─────────────┐  ┌─────────┐  ┌──────────┐                │
│  │FollowOwnerGoal│  │SitGoal  │  │ BegGoal  │                │
│  │  (Move)      │  │ (Target)│  │  (Look)  │                │
│  └──────┬───────┘  └────┬────┘  └────┬─────┘                │
│         │               │            │                       │
│         └───────────────┴────────────┘                       │
│                         │                                    │
│                   TameableEntity                             │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│              LandOnOwnersShoulderGoal.hpp                   │
│                      (Move)                                  │
│                         │                                    │
│               ShoulderRidingEntity                           │
│                   (继承自 TameableEntity)                    │
└─────────────────────────────────────────────────────────────┘
```

## 外部依赖关系

### 上游依赖（本目录依赖的模块）

| 模块 | 用途 |
|------|------|
| `entity/ai/goal/Goal.hpp` | Goal 基类 |
| `entity/ai/goal/GoalFlag.hpp` | 互斥标志枚举 |
| `entity/entities/passive/tamable/TameableEntity` | 可驯服实体基类 |
| `entity/entities/passive/tamable/ShoulderRidingEntity` | 肩膀乘坐实体基类 |
| `entity/entities/player/Player` | 玩家实体 |
| `item/core/ItemStack` | 物品堆 |
| `world/IWorld` | 世界接口 |

### 下游依赖（使用本目录的模块）

| 模块 | 使用的目标 |
|------|------------|
| `WolfEntity` | FollowOwnerGoal, SitGoal, BegGoal |
| `CatEntity` | FollowOwnerGoal, SitGoal, BegGoal |
| `ParrotEntity` | FollowOwnerGoal, SitGoal, BegGoal, LandOnOwnersShoulderGoal |

---

## 容易踩的坑

### 1. FollowOwnerGoal 传送距离检查

传送功能使用 `distanceTo()` 返回的距离与 `m_teleportDistance` 比较，注意 `distanceTo()` 返回的是 **欧几里得距离**（带 sqrt），不是距离平方。如果性能敏感，应改用距离平方比较。

### 2. SitGoal 互斥标志

`SitGoal` 使用 `GoalFlag::Target` 而非 `GoalFlag::Move`。这是因为坐下状态需要阻止实体选择攻击目标，而非仅仅阻止移动。如果错误使用 `Move` 标志，可能导致坐下时仍然攻击附近敌人。

### 3. BegGoal 驯服/繁殖物品区分

`BegGoal` 的 `_isPlayerHoldingFood()` 检查逻辑：
- **已驯服动物**：对驯服物品（如骨头）**和**繁殖物品都乞求
- **未驯服动物**：仅对繁殖物品乞求

这可能与直觉相悖（未驯服动物不对驯服物品乞求），需特别注意。

### 4. LandOnOwnersShoulderGoal 抢占条件

`isPreemptible()` 在 `m_isSittingOnShoulder == true` 时返回 `false`。这意味着一旦鹦鹉成功坐到肩膀上，其他 AI 目标无法打断它。但如果只是飞向主人过程中（`m_isSittingOnShoulder == false`），是可以被其他目标打断的。

### 5. BegGoal 距离检查使用平方距离

`BegGoal::shouldExecute()` 中使用 `distanceSqTo()` 而非 `distanceTo()` 来比较距离，这是正确的性能优化方式。但 `shouldContinueExecuting()` 中使用 `distanceTo()` 来比较 `m_maxDistance`，存在不一致。建议统一使用平方距离比较。

### 6. 与 TemptGoal 的区别

| 特性 | BegGoal | TemptGoal |
|------|---------|-----------|
| 行为 | 只看向玩家，不移动 | 跟随玩家移动 |
| 适用动物 | 狼、猫、鹦鹉 | 牛、猪、羊等 |
| 互斥标志 | `Look` | `Move`, `Look` |
| 驯服物品支持 | 已驯服动物对驯服物品乞求 | 不检查驯服状态 |

不要混淆这两个目标的使用场景。
