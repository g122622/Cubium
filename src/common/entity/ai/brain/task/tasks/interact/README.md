# Brain 交互任务 (interact/)

Brain 系统的交互类任务，控制实体与门、玩家、物品等的交互行为。

## 目录结构

```
interact/
├── InteractTasks.hpp    # 7个交互任务模板类
└── README.md            # 本文件
```

## 任务类概览

所有任务均为模板类 `Task<E>`，E 为实体类型，通过 Brain 的记忆模块通信。

| 任务类 | 记忆依赖 | 功能说明 |
|--------|---------|---------|
| `VillagerInteractTask<E>` | INTERACTION_TARGET(present), WALK_TARGET(registered) | 村民导航到互动目标附近，持续看向目标，在范围内停止移动 |
| `InteractWithDoorTask<E>` | INTERACTABLE_DOORS(present), OPENED_DOORS(registered), WALK_TARGET(present) | 沿路径行走时检测木门并自动开关，任务结束时关闭已打开的门 |
| `FollowOwnerTask<E>` | 无（直接检查 TameableEntity 状态） | 驯服动物跟随主人移动，距离过远时传送到主人身边 |
| `ProtectOwnerTask<E>` | OWNER_HURT_BY(present), ATTACK_TARGET(registered) | 驯服动物保护主人，设置攻击目标到攻击者并追击 |
| `PickupItemTask<E>` | NEAREST_VISIBLE_WANTED_ITEM(present) | 导航到物品位置并拾取，到达拾取范围后设置 pickupDelay=0 |
| `FollowParentTask<E>` | NEAREST_VISIBLE_ADULT(present) | 幼年动物跟随成年同类，定期重算路径 |
| `TemptTask<E>` | TEMPTING_PLAYER(present) | 动物被手持诱惑物品的玩家吸引，靠近时停止移动 |

## 关键设计决策

1. **方法签名修正**：所有任务使用 `IWorld*` 参数（与 Task 基类一致），修正了之前使用 `ServerWorld*` 的签名不匹配问题。
2. **requiredMemoryState 声明**：每个任务在构造函数中声明了所需的记忆状态，Brain 框架自动检查这些条件。
3. **FollowOwnerTask 不使用记忆模块**：因为 TameableEntity 当前只使用 Goal 系统，不使用 Brain 系统，直接通过 `dynamic_cast` 和 `getOwner()` 检查驯服状态。
4. **InteractWithDoorTask 的门记录**：使用 OPENED_DOORS 记忆模块记录由任务打开的门，在任务结束时统一关闭，避免门永久敞开。
5. **ProtectOwnerTask 依赖 OWNER_HURT_BY 记忆**：需要对应传感器（OwnerHurtBySensor）在主人被攻击时写入此记忆。
6. **InteractWithDoorTask 依赖 InteractableDoorsSensor**：需要 InteractableDoorsSensor 扫描附近的木门并写入 INTERACTABLE_DOORS 记忆。
7. **TemptTask 依赖 TemptingPlayerSensor**：需要 TemptingPlayerSensor 检测手持诱惑物品的玩家并写入 TEMPTING_PLAYER 记忆。
8. **VillagerEntity 已注册 InteractWithDoorTask**：在 IDLE、WORK、MEET、REST、PANIC 等活动中均注册了 InteractWithDoorTask。

## 上下游依赖

### 上游依赖
- `Brain<E>` — 任务调度和记忆管理
- `Task<E>` — 任务基类，提供生命周期管理
- `MemoryModuleTypes` — 记忆类型定义
- `WalkTarget` / `IPositionTarget` / `BlockPosTarget` — 位置目标类型

### 下游依赖
- `PathNavigator` — 寻路和导航
- `LookController` — 视线控制
- `DoorBlock` — 门的开关操作
- `TameableEntity` — 驯服状态检查和主人获取
- `AgeableEntity` — 幼体/成体判断
- `ItemEntity` — 物品拾取
- `GoalConstants` — 共享常量

## 容易踩的坑

1. **MemoryModuleTypes 必须在 using namespace memory 之后使用**：在模板类的构造函数初始化列表中使用 `MemoryModuleTypes` 需要 `using namespace memory;` 或完全限定名。
2. **FollowOwnerTask 的传送安全检查**：传送前必须检查 `hasNoCollisions`，否则实体可能卡在方块中。
3. **InteractWithDoorTask 仅处理木门**：铁门被过滤掉，与 MC 原版行为一致（铁门需要红石信号或按钮才能打开）。
4. **PickupItemTask 不直接拾取物品**：仅设置 `pickupDelay=0` 并清除记忆，实际拾取由物理系统处理。
5. **TameableEntity 当前仅使用 Goal 系统**：FollowOwnerTask 和 ProtectOwnerTask 是 Brain 版本，与 Goal 版本（FollowOwnerGoal/OwnerHurtByTargetGoal）并行存在。
