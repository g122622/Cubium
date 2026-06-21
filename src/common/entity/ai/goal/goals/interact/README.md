# 交互类 AI 目标 (Interact Goals)

提供实体与门交互和可驯服动物特有的 AI 行为目标。

## 目录结构

```
interact/
├── DoorInteractGoal.hpp/cpp    # 门交互目标基类（检测门位置、追踪穿过状态）
├── BreakDoorGoal.hpp/cpp       # 破门目标（僵尸、卫道士破坏木门）
├── TameableGoals.hpp/cpp       # 可驯服动物目标（FollowOwnerGoal, SitGoal, BegGoal）
└── LandOnOwnersShoulderGoal.hpp/cpp  # 肩膀乘坐目标（鹦鹉落到主人肩膀）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                   DoorInteractGoal                           │
│  检测路径上的木门，追踪实体是否穿过门                            │
│  提供 _isDoorOpen() 和 _setDoorOpen() 接口给子类              │
│                         │                                    │
│              ┌──────────┴──────────┐                        │
│         BreakDoorGoal          OpenDoorGoal (TODO)          │
│  破坏木门：难度检查、         打开/关门（卫道士袭击时使用）      │
│  破坏动画、音效、                                        │
│  最终移除门方块                                         │
│  依赖：DoorBlock::isWooden(), WorldEvents, GameRules         │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    TameableGoals.hpp                         │
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
│              LandOnOwnersShoulderGoal.hpp                    │
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
| `entity/core/MobEntity` | 生物实体基类（破门目标） |
| `entity/entities/passive/tamable/TameableEntity` | 可驯服实体基类 |
| `entity/entities/passive/tamable/ShoulderRidingEntity` | 肩膀乘坐实体基类 |
| `entity/entities/player/Player` | 玩家实体 |
| `item/core/ItemStack` | 物品堆 |
| `world/IWorld` | 世界接口 |
| `world/block/blocks/DoorBlock` | 门方块（isOpen, isWooden, toggleDoor） |
| `world/gamerule/GameRules` | mobGriefing 游戏规则 |

### 下游依赖（使用本目录的模块）

| 模块 | 使用的目标 |
|------|------------|
| `ZombieEntity` | BreakDoorGoal（破门能力，概率 = specialMultiplier × 0.1） |
| `VindicatorEntity` | BreakDoorGoal（袭击时破门） |
| `WolfEntity` | FollowOwnerGoal, SitGoal, BegGoal |
| `CatEntity` | FollowOwnerGoal, SitGoal, BegGoal |
| `ParrotEntity` | FollowOwnerGoal, SitGoal, BegGoal, LandOnOwnersShoulderGoal |

---

## 容易踩的坑

### 1. BreakDoorGoal 难度判断

BreakDoorGoal 使用 `DifficultyPredicate` 判断是否允许破门。默认谓词 `defaultDoorBreakDifficultyPredicate()` 仅在 Normal 和 Hard 难度允许破门，这与 MC Java 行为一致。Peaceful 和 Easy 难度下亡灵生物不会破门。

### 2. BreakDoorGoal 需要导航器设置 canOpenDoors

DoorInteractGoal::shouldExecute() 会检查 `navigator->canOpenDoors()`。如果实体的导航器没有启用开门能力，破门目标不会激活。僵尸和卫道士需要确保导航器的 `setCanOpenDoors(true)` 和 `setCanEnterDoors(true)` 被调用。

### 3. DoorInteractGoal 的 passed 检测

DoorInteractGoal 使用方向向量点积来检测实体是否穿过门。当生物从门的一侧走到另一侧时，方向向量的点积从正变负，判定为穿过。这个机制对开门和破门都适用。

### 4. FollowOwnerGoal 传送距离检查

传送功能使用 `distanceTo()` 返回的距离与 `m_teleportDistance` 比较，注意 `distanceTo()` 返回的是 **欧几里得距离**（带 sqrt），不是距离平方。如果性能敏感，应改用距离平方比较。

### 5. SitGoal 互斥标志

`SitGoal` 使用 `GoalFlag::Target` 而非 `GoalFlag::Move`。这是因为坐下状态需要阻止实体选择攻击目标，而非仅仅阻止移动。如果错误使用 `Move` 标志，可能导致坐下时仍然攻击附近敌人。

### 6. BegGoal 驯服/繁殖物品区分

`BegGoal` 的 `_isPlayerHoldingFood()` 检查逻辑：
- **已驯服动物**：对驯服物品（如骨头）**和**繁殖物品都乞求
- **未驯服动物**：仅对繁殖物品乞求

这可能与直觉相悖（未驯服动物不对驯服物品乞求），需特别注意。

### 7. LandOnOwnersShoulderGoal 抢占条件

`isPreemptible()` 在 `m_isSittingOnShoulder == true` 时返回 `false`。这意味着一旦鹦鹉成功坐到肩膀上，其他 AI 目标无法打断它。但如果只是飞向主人过程中（`m_isSittingOnShoulder == false`），是可以被其他目标打断的。

### 8. BegGoal 距离检查使用平方距离

`BegGoal::shouldExecute()` 中使用 `distanceSqTo()` 而非 `distanceTo()` 来比较距离，这是正确的性能优化方式。但 `shouldContinueExecuting()` 中使用 `distanceTo()` 来比较 `m_maxDistance`，存在不一致。建议统一使用平方距离比较。

### 9. 与 TemptGoal 的区别

| 特性 | BegGoal | TemptGoal |
|------|---------|-----------|
| 行为 | 只看向玩家，不移动 | 跟随玩家移动 |
| 适用动物 | 狼、猫、鹦鹉 | 牛、猪、羊等 |
| 互斥标志 | `Look` | `Move`, `Look` |
| 驯服物品支持 | 已驯服动物对驯服物品乞求 | 不检查驯服状态 |

不要混淆这两个目标的使用场景。
