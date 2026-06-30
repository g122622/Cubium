#Action Tasks（行动任务）

Brain 系统中的行动类任务，负责执行具体的动作行为（攻击、繁殖、装死、跳跃、踢击等）。

        ##目录结构

``` action /
├── ActionTasks.hpp #所有行动任务的模板类定义
└── README.md #本文件
```

        ##任务列表

    | 任务 | 对应 MC | 状态 | 记忆模块需求 | 说明 | | -- -- --| -- -- -- -- -| -- -- --| -- -- -- -- -- -- -| -- -- --|
    | AttackTask | MeleeAttack | ✅ 已实现 | ATTACK_TARGET(P),
    ATTACK_COOLING_DOWN(A),
    LOOK_TARGET(R) | 单次触发近战攻击 | | BreedTask | AnimalMakeLove | ✅ 已实现 | VISIBLE_MOBS(P), BREED_TARGET(A),
    WALK_TARGET(R),
    LOOK_TARGET(R) | 动物繁殖 | | EatTask | — | ⏳ 占位 | — | 饥饿系统未实现，shouldExecute 始终返回 false |
    | PlayDeadTask | BecomePassiveIfMemoryPresent | ✅ 已实现 | PLAY_DEAD_TICKS(P),
    ATTACK_TARGET(R),
    PACIFIED(R) | 装死 | | JumpTask | — | ✅ 已实现 | JUMP_COOLDOWN(A) | 单次触发跳跃，设置冷却记忆 | | KickTask
    | — | ✅ 已实现 | ATTACK_TARGET(P),
    ATTACK_COOLING_DOWN(A) | 单次触发踢击攻击 |

    记忆模块状态：P = VALUE_PRESENT,
    A = VALUE_ABSENT,
    R = REGISTERED

    ##上下游外部依赖

    ## #上游依赖
    - `brain / memory /` — 记忆模块类型（BREED_TARGET,
    PLAY_DEAD_TICKS,
    JUMP_COOLDOWN 等） - `brain / task / Task.hpp` — 任务基类 - `entity / core / AgeableEntity` — 繁殖相关接口（isInLove
    ,
    canMateWith,
    spawnBaby 等） - `entity / entities / passive / basic / AnimalEntity` — 动物实体基类
    - `ai / controller /` — LookController,
    JumpController - `ai / goal / GoalConstants.hpp` — 繁殖常量（BREED_DETECTION_RANGE, BREED_DISTANCE_SQ,
    SPAWN_BABY_DELAY）

    ## #下游依赖
    -
    未来 AnimalEntity 子类的 `initializeBrain()` 中注册这些任务

    ##容易踩的坑

    -
    **BreedTask 依赖 AnimalEntity 集成 Brain **：当前动物实体仍使用 Goal 系统的 BreedGoal，BreedTask
     需要实体拥有 `brain()` 方法才能使用。当动物迁移到 Brain 系统后，需在 `initializeBrain()` 中注册此任务和配套传感器。
    -
    **EatTask 饥饿系统缺失 **：`isHungry()` 方法尚未在 AnimalEntity
        / AgeableEntity 中实现，EatTask 的 `shouldExecute()` 始终返回 false。
    -
    **单次触发 vs 持续执行 **：AttackTask、JumpTask、KickTask 是单次触发型（`shouldContinueExecuting` 返回
     false），冷却由 TTL 记忆管理；BreedTask、PlayDeadTask 是持续执行型，有 `shouldContinueExecuting` 逻辑。
    -
    **冷却由 TTL 管理 **：AttackTask 和 KickTask 的冷却通过 `ATTACK_COOLING_DOWN` 记忆的 TTL 自动过期，JumpTask
     的冷却通过 `JUMP_COOLDOWN` 记忆的 TTL 管理，无需手动计数器。
    - **IWorld *而非 ServerWorld ***：任务方法签名使用 `IWorld *`，与 Task 基类一致，不能使用 ServerWorld 独有的 API。
