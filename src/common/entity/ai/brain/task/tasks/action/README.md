#Brain 动作任务(action /)

Brain 系统的动作类任务，控制实体的攻击、繁殖、进食、装死、跳跃、踢击等行为。

        ##目录结构

``` action /
├── ActionTasks.hpp #6个动作任务模板类
└── README.md #本文件
```

        ##任务类概览

            所有任务均为模板类 `Task<E>`，E 为实体类型（需继承 MobEntity），通过 Brain 的记忆模块通信。

    | 任务类 | 对应 MC 行为 | 记忆依赖 | 功能说明 | 状态 | | -- -- -- --| -- -- -- -- -- -- -| -- -- -- -- -|
    -- -- -- -- -| -- -- --| | `AttackTask<E>` | MeleeAttack | ATTACK_TARGET(present),
    ATTACK_COOLING_DOWN(absent),
    LOOK_TARGET(registered) | 近战攻击：面向目标→挥臂→attackEntityAsMob→设置冷却 | ✅ 已实现 |
    | `BreedTask<E>` | AnimalMakeLove | BREED_TARGET | 动物繁殖：导航到配偶→繁殖→清除记忆 | ❌ 未实现 |
    | `EatTask<E>` | — | — | 进食行为 | ❌ 未实现 | | `PlayDeadTask<E>` | BecomePassiveIfMemoryPresent | PLAY_DEAD
    | 装死行为 | ❌ 未实现 | | `JumpTask<E>` | JumpOnBed / LongJumpToRandomPos | JUMP_COOLDOWN | 跳跃行为 | ❌ 未实现 |
    | `KickTask<E>` | — | ATTACK_TARGET | 踢击攻击 | ❌ 未实现 |

    ##AttackTask 详细说明

    AttackTask 是单次触发型任务，每次满足条件时执行一次攻击，与 ChaseTask 配合使用：

    1. **ChaseTask **追踪目标并接近，当距离足够近时停止设置 WALK_TARGET
    2. **AttackTask **检测目标在近战范围内且不在冷却中，执行攻击
    3. 攻击后设置 `ATTACK_COOLING_DOWN` 记忆（TTL = cooldownTicks），Brain 的记忆过期机制自动清除

    攻击执行流程（对应 MC MeleeAttack）： 1. 检查 ATTACK_TARGET 存在且目标存活
    2. 检查不在冷却中（ATTACK_COOLING_DOWN 为空） 3. 检查未手持远程武器（canUseNonMeleeWeapon）
    4. 检查在近战攻击范围内（`(宽度 * 2) ^
    2 +
        目标宽度`） 5. 面向目标（LookController::setLookPositionWithEntity） 6. 挥动手臂（swingArm）
            7. 执行近战攻击（attackEntityAsMob） 8. 设置冷却记忆（setMemoryWithTTL）

            ##数据流

``` ChaseTask ──设置 WALK_TARGET──→ MoveToTargetTask ──执行导航──→ 到达攻击范围
                                                                    │ AttackTask ←──读取 ATTACK_TARGET────
            Brain记忆 ←──传感器
            / HurtBySensor
          ←──检查 ATTACK_COOLING_DOWN
          ──→ swingArm() +
        attackEntityAsMob()
          ──→ setMemoryWithTTL(ATTACK_COOLING_DOWN, true, cooldown)
```

        ##上下游依赖

        ## #上游依赖
        - `Brain<E>` — 任务调度和记忆管理 - `Task<E>` — 任务基类，提供生命周期管理
        - `MemoryModuleTypes` — 记忆类型定义（ATTACK_TARGET,
                                        ATTACK_COOLING_DOWN, LOOK_TARGET） - `MobEntity` — canUseNonMeleeWeapon(),
                                        attackEntityAsMob(), swingArm(),
                                        lookController()

                                            ## #下游依赖
    - `ChaseTask` — 通常与 AttackTask 配合使用，负责追踪和接近目标 - `MoveToTargetTask` — 读取 WALK_TARGET 执行实际移动

        ##容易踩的坑

        1. *
        *AttackTask 不设置 WALK_TARGET **：与 ChaseTask 不同，AttackTask 不负责导航。应将 ChaseTask 和 AttackTask
        注册在同一 Activity 中。 2. *
        *冷却由 TTL 管理 **：AttackTask 不使用手动计数器，而是通过 `ATTACK_COOLING_DOWN` 记忆的 TTL
        自动过期。Brain 的 `_tickMemories()` 在每 tick 递减 TTL，到期后自动清除。 3. *
        *canUseNonMeleeWeapon 检查 **：骷髅类实体手持弓时应使用远程攻击而非近战，此检查确保 AttackTask
        在实体持有远程武器时不触发。 4. *
        *IWorld *而非 ServerWorld ***：任务方法签名使用 `IWorld *`，与 Task 基类一致，不能使用 ServerWorld 独有的 API。
        5. *
        *attackEntityAsMob 而非 hurt **：AttackTask 调用 `MobEntity::
            attackEntityAsMob()` 而非直接调用 `hurt()`，因为前者包含附魔伤害加成、火焰附加、击退等完整逻辑。
