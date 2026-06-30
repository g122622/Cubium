#Brain Task System

##目录结构

``` task /
├── Task.hpp #任务基类，定义核心接口
├── README.md
└── tasks / #具体任务实现
    ├── movement /
    │   ├── MovementTasks.hpp #6个移动任务模板类（已实现）
    │   └── README.md #移动任务详细说明
    ├── action /
    │   ├── ActionTasks.hpp #攻击、繁殖、进食等动作任务
    │   └── README.md #动作任务详细说明
    └── interact /
        ├── InteractTasks.hpp #7个交互任务模板类（已实现）
        └── README.md #交互任务详细说明
```

             ##内部模块关系

``` Task<E>(基类)
    │
    ├── 依赖 memory::MemoryModuleType(读取 / 写入记忆)
    │
    ├── E(实体模板参数)
    │     ├── MobEntity
    │     ├── CreatureEntity
    │     └── AgeableEntity
    │
    └── IWorld(世界访问)

                 数据流：Sensor 更新 Memory → Schedule 选择 Activity → Task 根据 Memory 执行行为
```

             ##已实现的任务

             ## #movement
        / — 移动类任务（6个）

    | 任务类 | 对应 MC 行为 | 记忆依赖 | 功能说明 | | -- -- -- --| -- -- -- -- -- -- -| -- -- -- -- -| -- -- -- -- -|
    | `MoveToTargetTask<E>` | MoveToTargetSink | WALK_TARGET(present),
    PATH(registered) | 读取 WALK_TARGET 记忆执行实际寻路和移动，所有行走移动的核心执行器 |
    | `StrollTask<E>` | RandomStroll |
    WALK_TARGET(absent) | WALK_TARGET 缺失时随机选择目标位置，设置到记忆中由 MoveToTargetTask 执行 |
    | `LookAtEntityTask<E>` | SetEntityLookTarget / LookAtTargetSink |
    LOOK_TARGET(absent) | 概率触发，搜索附近实体并设置 LOOK_TARGET 记忆，持续更新视线方向 |
    | `FindHiddenBlockTask<E>` | LocateHidingPlace | HIDING_PLACE(absent),
    WALK_TARGET(absent) | 被伤害或听到铃声时触发，从 HOME / NEAREST_BED 记忆寻找隐蔽点 |
    | `ChaseTask<E>` | SetWalkTargetFromAttackTargetIfTargetOutOfRange | ATTACK_TARGET(present),
    WALK_TARGET(registered),
    LOOK_TARGET(registered) | 追踪攻击目标，动态更新 WALK_TARGET 和视线方向 | | `FleeTask<E>` | SetWalkTargetAwayFrom |
    AVOID_TARGET(present),
    WALK_TARGET(absent) | 逃离威胁，使用 RandomPositionGenerator 生成远离方向的位置 |

    详细说明参见[movement / README.md](tasks / movement / README.md)。

        ## #interact
        / — 交互类任务（7个）

    | 任务类 | 记忆依赖 | 功能说明 | | -- -- -- --| -- -- -- -- -| -- -- -- -- -| | `VillagerInteractTask<E>` |
    INTERACTION_TARGET(present),
    WALK_TARGET(registered) | 村民导航到互动目标 | | `InteractWithDoorTask<E>` | INTERACTABLE_DOORS(present),
    OPENED_DOORS(registered),
    WALK_TARGET(present) | 沿路径自动开关门 | | `FollowOwnerTask<E>` | 无（直接检查 TameableEntity 状态）
    | 驯服动物跟随主人 | | `ProtectOwnerTask<E>` | OWNER_HURT_BY(present),
    ATTACK_TARGET(registered) | 驯服动物保护主人 | | `PickupItemTask<E>` |
    NEAREST_VISIBLE_WANTED_ITEM(present) | 导航到物品位置并拾取 | | `FollowParentTask<E>` |
    NEAREST_VISIBLE_ADULT(present) | 幼年动物跟随成年同类 | | `TemptTask<E>` |
    TEMPTING_PLAYER(present) | 动物被手持诱惑物品的玩家吸引 |

    详细说明参见[interact / README.md](tasks / interact / README.md)。

        ## #action
        / — 动作类任务（1个已实现 / 5个待实现）

    | 任务类 | 对应 MC 行为 | 记忆依赖 | 功能说明 | 状态 | | -- -- -- --| -- -- -- -- -- -- -| -- -- -- -- -|
    -- -- -- -- -| -- -- --| | `AttackTask<E>` | MeleeAttack | ATTACK_TARGET(present),
    ATTACK_COOLING_DOWN(absent),
    LOOK_TARGET(registered) | 近战攻击：面向目标→挥臂→attackEntityAsMob→设置冷却 | ✅ |
    | `BreedTask<E>` | AnimalMakeLove | BREED_TARGET | 动物繁殖 | ❌ | | `EatTask<E>` | — | — | 进食行为 | ❌ |
    | `PlayDeadTask<E>` | BecomePassiveIfMemoryPresent | PLAY_DEAD | 装死行为 | ❌ |
    | `JumpTask<E>` | JumpOnBed / LongJumpToRandomPos | JUMP_COOLDOWN | 跳跃行为 | ❌ |
    | `KickTask<E>` | — | ATTACK_TARGET | 踢击攻击 | ❌ |

    详细说明参见[action / README.md](tasks / action / README.md)。

        ##上下游外部依赖关系

            ** 被依赖（下游）**： - `entity / entities / villager / VillagerEntity.hpp` -
        村民 Brain 初始化中注册了所有 6 个移动任务和 InteractWithDoorTask - `entity / entities / passive / tamable /` -
        驯服动物可使用 FollowOwnerTask、ProtectOwnerTask 等

            ** 依赖（上游）**： - `brain / Brain.hpp` -
        Brain 主控制器 - `brain / memory /` - 记忆模块系统 - `entity / core / MobEntity.hpp` - 生物实体基类
        - `entity / core / CreatureEntity.hpp` - 陆地生物基类（StrollTask、FleeTask 使用 RandomPositionGenerator）
        - `entity / ai / pathfinding / PathNavigator.hpp` - 寻路导航器
        - `entity / ai / controller / LookController.hpp` - 视线控制器
        - `entity / ai / controller / MovementController.hpp` - 移动控制器
        - `entity / ai / util / RandomPositionGenerator.hpp` - 随机位置生成器 - `world / IWorld.hpp` -
        世界接口

        ##容易踩的坑

        ## #1. 记忆模块依赖

        任务启动前必须确保所需记忆模块已设置。Task
        基类通过 `m_requiredMemoryState` 检查记忆状态，若记忆不存在或状态不匹配，任务不会启动。

        ## #2. 生命周期管理

        任务可能被中断，必须在 `resetTask()` 中清理状态。典型问题：导航路径未清理、攻击目标未清除等。

        ## #3. 模板实例化

        每个实体类型需要单独实例化任务模板：`ChaseTask<MobEntity>`、`ChaseTask<VillagerEntity>` 等。

        ## #4. 与 Goal 系统的区别

    | 特性 | Goal 系统 | Brain Task 系统 | | -- -- --| -- -- -- -- -- -| -- -- -- -- -- -- -- -- -| | 状态管理
    | 手动管理 | 记忆模块驱动 | | 优先级 | mutex 标志 | 记忆状态要求 | | 复杂度 | 简单行为 | 复杂行为链 | | 适用场景
    | 基础生物 | 村民、铁傀儡等 |

    ## #5. 任务超时机制

    Task
    基类内置持续时间机制(`m_durationMin`, `m_durationMax`)，任务会在超时后自动停止。不要在 `shouldContinueExecuting()` 中忽略超时逻辑。

    ## #6. WALK_TARGET 记忆冲突

    同一 Activity 中不能同时有多个任务写入 WALK_TARGET，优先级控制哪个任务先执行。MoveToTargetTask 只读取
    WALK_TARGET（不写入），其他任务（StrollTask、ChaseTask、FleeTask、FindHiddenBlockTask）写入 WALK_TARGET。

    ## #7. PATH 记忆同步

    MoveToTargetTask 在 `updateTask()` 中同步 PATH 记忆，在 `resetTask()` 中清除 PATH 记忆。其他任务不应直接操作 PATH。

    ## #8. 动态类型转换

    StrollTask 和 FleeTask 使用 `dynamic_cast<CreatureEntity*>(owner)` 以访问 RandomPositionGenerator。非 CreatureEntity
    实体将使用备用策略（FleeTask）或直接跳过（StrollTask）。
