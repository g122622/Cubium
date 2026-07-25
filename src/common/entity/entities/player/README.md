#玩家实体模块

玩家实体模块负责封装玩家的移动、能力、背包、经验、脚步声和游泳声等核心状态，是客户端主循环和服务端玩家逻辑之间的共同抽象。

##目录结构

```text src / common / entity / entities / player /
├── ChatVisibility.hpp #聊天可见性枚举（全显示、仅系统、隐藏）
├── GameModeUtils.hpp #游戏模式能力映射工具
├── GameModeUtils.cpp #游戏模式能力映射实现
├── Player.hpp #玩家实体声明，包含状态、移动、权限等级和网络同步接口
├── Player.cpp #玩家实体实现，包含物理、脚步声、游泳声和序列化
├── PlayerModelPart.hpp #玩家皮肤部件位掩码（披风、夹克、袖子、裤腿、帽子）
├── SpawnLocationHelper.hpp #重生点位置辅助工具
└── README.md #本文档
```

                                            ##模块关系

    - `Player` 继承自 `LivingEntity`，复用通用的位置、旋转、碰撞和数据管理能力。 - `Player` 在退出蹲伏、游泳和睡眠姿态时，会通过 `IWorld` 的碰撞查询判断当前空间是否允许切回站立。 - `ClientApplication` 使用 `Player` 的 `distanceWalkedModified` 等价累计值和 `cameraYaw /
        prevCameraYaw` 来驱动原版 `GameRenderer.applyBobbing()` 风格的视图矩阵变换，并读取脚步声
        / 游泳声标志来播放本地音效。
    - `ClientNetwork` 和玩家序列化逻辑负责把服务器传来的传送、位置和状态同步到本地玩家。
    - 服务端玩家管理由 `server / world / player / ServerPlayerEntityManager` 负责。
    - 客户端本地玩家身份由 `client / world / player / LocalPlayerIdentity` 管理。
    - `GameModeUtils` 负责把游戏模式转换为玩家能力，避免重复实现。提供 `getAbilitiesForGameMode()`（能力映射）、`isBlockPlacingRestricted()`（冒险/旁观模式放置限制）等工具方法。
    - `CooldownTracker`（位于 `entity / player /`）管理物品冷却，供 `Player` 持有。

        ##上下游外部依赖关系

        ## #上游依赖（本模块依赖）

    - `entity / core / Entity.hpp`、`entity / core / LivingEntity.hpp` - 实体基类
    - `entity / player / CooldownTracker.hpp` - 物品冷却追踪 - `entity / experience / ExperienceManager.hpp` - 经验管理
    - `entity / inventory / PlayerInventory.hpp` - 玩家背包 - `entity / food / FoodStats.hpp` - 饥饿系统
    - `entity / movement / AutoJump.hpp` - 自动跳跃 - `physics / PhysicsConstants.hpp` - 物理常量
    - `world / IWorld.hpp` - 世界接口 - `world / block / BlockPos.hpp`、`world / block / BlockState.hpp` - 方块相关
    - `network / protocol / EntityEvents.hpp` - 实体状态/动画枚举（权限等级、装备破损等经 EntityEvent 广播）
    - `network / ir / packets / play / PlayPackets.hpp` - 玩家能力/重生等 IR 包定义

    ## #下游依赖（依赖本模块）

    - `server / world / player / ServerPlayer.hpp` - 服务端玩家实体
    - `client / world / player / LocalPlayer.hpp` - 客户端本地玩家 - `client / ClientApplication.hpp` - 客户端主循环
    - `network / NetworkClient.hpp` - 已删除（网络同步改走 IR `pipeline::Connection` + `ClientPlayVisitor`） -
    各种实体交互系统（攻击、物品使用等）

    ##容易踩的坑

    - **装备虚方法重写**：Player 重写了 `getEquipment()`/`setEquipment()`/`getMutableEquipment()` 三个虚方法，通过 `PlayerInventory` 间接管理装备（而非基类 `m_equipment` 数组）。任何需要修改装备的代码必须使用 `getMutableEquipment()` 或 `getMutableMainHandItem()`，而非 `const_cast<ItemStack&>(getEquipment(...))`，否则对 Player 实例会修改基类 `m_equipment` 而非 `PlayerInventory`。
    - **awardCustomStat 虚方法**：`Player::
            awardCustomStat()` 是虚方法，基类空实现（不会崩溃也不会更新统计）；仅 `ServerPlayer` 重写版本实际委托给 `StatisticsManager::
                incrementCustom()`。客户端调用安全但无效果。常量定义在 `common
        / stats / Stats.hpp`，与 `StatRegistry` 注册名必须完全一致。
    -
    **步距统计位置误用**：不要把 `Entity::prevPosition()` 当成脚步声采样位置，它是插值
        / 帧历史状态，不是步距累计基准。步距统计使用 `m_moveDistanceSamplePosition`。 -
    **传送后的步距重置**：不要在外部直接修改玩家位置后继续沿用旧的步距计数，传送和出生都应该调用 `Player::
         setPosition()` 来重置采样。
    - **重复采样问题**：`updateMoveDistance()` 可以在同一帧里被多次调用，但每次都必须只统计 "上次采样之后"的增量。 -
    **视野晃动与脚步声耦合**：视野晃动和脚步声共用同一套移动距离统计，统计语义错了会同时污染音效和镜头。 -
    **姿态切换碰撞检查**：从蹲下、游泳、睡眠切回站立时，不要直接强行改成 `Standing`；应保留 `Player::
            setSneaking()` / `Player::setSwimming()` / `Player::
            setSleeping()` 的碰撞检查结果，否则会在低顶方块下错误穿模。
    - ** 声音事件链路**：玩家受伤和死亡声音已经接入通用实体声音链路，不要再在服务器侧手写单独广播分支。
    - ** 步声系统**：Player 重写 `playStepSound` 处理水中步声（播放游泳声 +
    沉闷步声），非水中情况委托给 `Entity::playStepSound`，后者已包含
        COMBINATION_STEP_SOUND_BLOCKS（组合步声）、INSIDE_STEP_SOUND_BLOCKS（内部步声）和
        CRYSTAL_SOUND_BLOCKS（紫水晶共振）的完整逻辑。Player 不需要也不应该重复判断 INSIDE
        / COMBINATION 标签，否则会导致双重步声。详见 `entity / core / README.md` 的步声系统章节。
    - ** 视野晃动公式**：视野晃动的行走相位使用原版 `distanceWalkedModified =
    水平实际位移 * 0.6`，不要再把未缩放的行走距离直接传给渲染层。 -
    **相机晃动条件 * *：`cameraYaw
        / prevCameraYaw` 是原版平滑晃动强度，只有站在地面、未死亡、未游泳时根据水平速度趋近，骑乘时应清零。
    -
    **输入与物理分离 *
        *：`handleMovementInput()` 只缓存当前输入，不再直接修改速度；客户端必须由 `ClientApplication` 按
        20TPS 调用 `updatePhysics()` 消费输入。测试或逻辑里调用 `handleMovementInput()` 后，需要执行一次 `updatePhysics()` 才会看到速度和位置变化。
    -
    **能力同步来源 *
        *：能力同步以 `Player::abilities()` 为运行时事实来源；构造 IR `ir::play::PlayerAbilities` 时不会根据 GameMode
        重新推导，避免覆盖飞行状态或自定义 walk
        / fly speed。
    - ** 挖掘速度公式**：最终挖掘速度 =
        基础速度 × 效率附魔加成 × 急迫效果乘数 × 挖掘疲劳乘数 × 水下惩罚 × 空中惩罚。各乘数叠加顺序影响结果精度。 -
        **攻击冷却判定 * *：横扫攻击需要玩家 "几乎静止"（`distanceWalkedModified - prevDistanceWalkedModified <
    aiMoveSpeed()`），否则不会触发横扫效果。 - **权限等级与游戏模式分离 * *：`m_permissionLevel`（0 -
        4）独立于游戏模式存储，`setGameMode()` 会重置 `m_abilities` 但不会重置 `m_permissionLevel`。`canUseGameMasterBlocks()` 要求同时满足 `creativeMode` 和 `permissionLevel
    >= 2`。 - ** 权限等级网络同步**：服务端 `/ op`/`/ deop` 后会通过 `ir::play::EntityEvent`（携带 `network::EntityStatus::permissionLevel(level)`，status byte = 24 +
            level）通知客户端权限等级变更，客户端收到后在 `ClientPlayVisitor` 的 `onEntityStatus` 回调中更新本地玩家的 `m_permissionLevel`。 -
            **冒险模式mayInteract检查双手**
                *：`Player::mayInteract()` 在冒险模式下会同时检查主手和副手物品的 CanPlaceOn
                 标签，任一只手的物品匹配即允许交互。参考 MC Java 的 `Player
                     .mayUseItemAt()`，该方法是逐手检查而非合并检查——服务端在处理交互包时，会根据包中指定的
                 InteractionHand 来决定检查哪只手的物品。
            -
            **建造权限与冒险模式限制**
                *：`Player::mayBuild()` 直接返回 `m_abilities.allowEdit`，对应 MC Java 的 `Player.mayBuild()`。生存/创造模式默认 `allowEdit=true`，冒险/旁观模式默认 `allowEdit=false`。`Player::mayUseItemAt(world, pos, facing, itemStack)` 先检查 `mayBuild()`，若为 false 则检查物品的 CanPlaceOn 标签是否匹配 pos 对面（opposite(facing)）的方块。`Player::blockActionRestricted(world, pos)` 先检查 `isBlockPlacingRestricted(gameMode)`，再检查旁观者、`mayBuild()`，最后检查主手物品的 CanDestroy 标签。`GameModeUtils::isBlockPlacingRestricted()` 在冒险/旁观模式下返回 true。这些方法已在 `BlockInteractionManager` 中集成：`handleBlockPlacement` 使用 `mayUseItemAt()`，`_canBreakBlock` 使用 `blockActionRestricted()`。
            -
            **重锤下落攻击流程 *
                *：`Player::attack()` 中在计算附魔伤害前检测重锤下落攻击（`MaceItem::
                    canSmashAttack()`），如果触发则：跳过普通暴击判定、使用 `MaceItem::
                        getSmashAttackDamageBonus()` 计算下落攻击伤害加成（含致密魔咒）、使用 `DamageSources::
                            maceSmash()` 伤害类型、调用 `MaceItem::
                                hitEntity()` 处理砸地效果（停止下落、音效、击退）、调用 `postHitEntity()` 重置下落距离、检测风爆魔咒施加弹起速度。
            -
            **PvP 保护机制 *
                *：`Player::
                    canHarmPlayer()` 控制玩家间伤害判定。基类实现检查队伍友伤规则（攻击者无队伍→可伤害；同队→取决于 `getAllowFriendlyFire()`；不同队→可伤害）。`ServerPlayer` 重写此方法，先检查
                PVP 游戏规则（`IWorld::isPvpAllowed()`），PvP
                禁用时直接返回 false。`ServerPlayer::hurt()` 在伤害来源为玩家时调用 `canHarmPlayer()` 拦截非法 PvP
                伤害。驯服动物（如狼）的 `wantsToAttack()` 也调用 `canHarmPlayer()` 判断主人是否可以攻击目标玩家。
            -
            **疾跑击退与 causeExtraKnockback *
                *：`Player::causeExtraKnockback()` 重写 LivingEntity 基类版本，在基类击退逻辑基础上增加 `setSprinting(
                    false)`。当目标是 ServerPlayer 且 `hurtMarked` 为
                true 时，立即通过 `sendVelocityPacket()` 发送速度包、清除 hurtMarked、恢复 preHurtVelocity，避免
                EntityTracker::tick() 重复发送速度包导致客户端击退速度被重复应用。`sendVelocityPacket()` 是 Player
                基类的虚方法（返回 false），ServerPlayer 重写版本实际发送网络包并返回 true。

                -- -

            ##冲量坠落伤害免疫系统

            Player 实现了 MC Java `Player` 中的 impulse context 系统，用于重锤砸地攻击和风弹爆炸后的坠落伤害减免。

            ## #核心字段

        | 字段 | 类型 | 说明 | | -- -- --| -- -- --| -- -- --|
        | `m_currentImpulseImpactPos` | `std::optional<Vector3>` | 冲量冲击位置（砸地 / 爆炸位置） |
        | `m_ignoreFallDamageFromCurrentImpulse` | `bool` | 是否忽略当前冲量的坠落伤害 |
        | `m_currentImpulseContextResetGraceTime` | `i32` | 冲量上下文重置宽限期（tick） |
        | `m_currentExplosionCause` | `EntityId` | 引起冲量的实体ID（用于进度触发） |

        ## #关键方法

            - `setIgnoreFallDamageFromCurrentImpulse(bool)` — 设置免疫标志并启动 / 清除 40 tick 宽限期
            - `isIgnoringFallDamageFromCurrentImpulse()` — 检查是否忽略冲量坠落伤害
            - `applyPostImpulseGraceTime(i32)` — 扩展宽限期（取最大值，不缩短已有宽限期），风爆附魔用 10 tick
            - `tryResetCurrentImpulseContext()` — 仅当宽限期为 0 时重置（着地 / 水中 / 攀爬时调用）
            - `resetCurrentImpulseContext()` — 完全重置所有冲量状态
            - `calculateMaceImpactPosition()` — 计算重锤冲击位置（防止连续砸地双重获利）
            - `onExplosionHit(Entity*)` — 被爆炸击中时设置冲量上下文（仅风弹启用免疫）

            ## #坠落伤害减免逻辑

`Player::causeFallDamage()` 重写实现冲量减免：当 `m_ignoreFallDamageFromCurrentImpulse
    &&
    m_currentImpulseImpactPos` 为
            true 时，坠落距离被限制为 `min(实际坠落距离, 冲击位置Y - 玩家Y)`。玩家在冲击位置上方时不受伤。

            ## #冲量上下文生命周期

            1. *
            *设置 * *：重锤砸地攻击调用 `setIgnoreFallDamageFromCurrentImpulse(true)` +
        设置冲击位置；风弹爆炸调用 `onExplosionHit()` 2. * *宽限期 *
            *：40 tick 计时器逐 tick 递减，期间 `tryResetCurrentImpulseContext()` 不重置 3. * *减免 *
            *：`causeFallDamage()` 中根据冲击位置计算减免后的坠落距离 4. * *重置触发 *
            *：实际受到伤害、切换创造模式、着地 / 水中 /
            攀爬（宽限期结束后）、卡在方块中（宽限期结束后）

            ## #注意事项

        - 冲量上下文字段已通过 `Player::addAdditionalSaveData`/`readAdditionalSaveData` 和 `PlayerSaveData` 实现持久化
        - `m_currentExplosionCause` 不持久化（MC Java 同样不序列化此字段，因为它是运行时瞬时实体引用） -
        对应 MC
        Java `Player` 的 `setIgnoreFallDamageFromCurrentImpulse`、`currentImpulseImpactPos`、`currentImpulseContextResetGraceTime` 系列方法

---

## 最后死亡位置（LastDeathLocation）

Player 实现了 MC Java `Player` 中的最后死亡位置记录系统，用于恢复指南针（Recovery Compass）指向。

### 核心字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `m_lastDeathLocation` | `std::optional<GlobalPos>` | 最后死亡位置（维度ID + 方块坐标），未死亡时为空 |

### 关键方法

- `getLastDeathLocation()` — 获取最后死亡位置
- `setLastDeathLocation(std::optional<GlobalPos>)` — 设置/清除最后死亡位置

### 死亡记录逻辑

`Player::die()` 在玩家死亡时自动记录 `m_lastDeathLocation = GlobalPos(m_dimension, onPos())`，包含当前维度和脚下方块位置。

### 持久化

最后死亡位置通过两条路径持久化：
1. **Entity NBT 路径**: `Player::addAdditionalSaveData()`/`readAdditionalSaveData()` — NBT 格式为 `{LastDeathLocation: {dimension: "minecraft:overworld", pos: [x, y, z]}}`，与 MC Java 兼容
2. **PlayerSaveData 路径**: `PlayerSaveData::toNbt()`/`fromNbt()` + `PlayerDataManager::fromPlayer()`/`applyToPlayer()` — 独立的玩家存储系统

### 网络同步

维度切换时，IR `ir::play::Respawn` 携带 `lastDeathLocation` 字段同步到客户端，客户端在 `ClientPlayVisitor` 的 `onRespawn` 回调中更新本地玩家状态。

### 注意事项

- 反序列化同时支持字符串维度名（`"minecraft:overworld"`）和整数维度ID（向后兼容）
- 项目不重新创建 Player 实体（与 MC Java 的 `restoreFrom` 不同），因此无需 `restoreFrom` 逻辑
- 同维度重生的 Respawn IR 包发送属于尚未实现的死亡重生基础设施，超出 LastDeathLocation TODO 范围

---

## 旁观者摄像机跟踪（Spectator Camera Tracking）

Player 基类中包含旁观者模式摄像机跟踪的核心状态字段，由 `ServerPlayer` 负责完整的网络同步和每 tick 位置更新。

### 核心字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `m_cameraEntityId` | `std::optional<EntityId>` | 旁观目标实体 ID，空值表示自身视角 |

### 关键方法

- `getCameraEntityId()` — 获取旁观目标实体 ID（optional）
- `isSpectating()` — 是否正在旁观某个实体（`m_cameraEntityId.has_value()`）
- `setCameraEntityId(std::optional<EntityId>)` — 设置/清除旁观目标，值变化时触发 `onCameraEntityChanged()` 虚方法通知
- `onCameraEntityChanged(oldId, newId)` — 摄像机目标变更通知虚方法，基类空操作，ServerPlayer 重写以发送 IR `ir::play::SetCamera` 和传送
- `isInputSneaking()` — 获取潜行输入状态（用于 ServerPlayer::tickSpectator 检测退出旁观）
- `attack(Entity&)` 虚方法 — 旁观者模式下调用 `setCameraEntityId()` 设置旁观目标，通过 `onCameraEntityChanged()` 自动同步网络

### 旁观者 noclip

`Player::setGameMode()` 中：
- 切换到旁观者模式 → `setNoClip(true)`
- 离开旁观者模式 → `setNoClip(false)` + `setCameraEntityId(std::nullopt)`（触发 `onCameraEntityChanged()` 通知 ServerPlayer 发送 `ir::play::SetCamera`）

### 数据流

1. **设置旁观**：SpectateCommand / 旁观者攻击 → `setCameraEntityId()` → `onCameraEntityChanged()` → ServerPlayer 发送 `ir::play::SetCamera` + 传送到目标
2. **每 tick 同步**：`ServerPlayer::tick()` → `tickSpectator()` → 同步位置到目标、检查有效性、潜行退出
3. **模式切换**：`setGameMode()` 离开旁观模式时 → `setCameraEntityId(nullopt)` → `onCameraEntityChanged()` → ServerPlayer 发送 `ir::play::SetCamera`
4. **客户端**：收到 `ir::play::SetCamera` → 设置 `m_cameraEntityId` → 渲染循环跟随目标 ClientEntity

### 注意事项

- `setCameraEntityId()` 内含相等性检查，值未变化时不触发 `onCameraEntityChanged()`，避免重复发包
- `Player::attack()` 基类中旁观者路径调用 `setCameraEntityId()`，通过虚方法 `onCameraEntityChanged()` 自动触发 ServerPlayer 的网络同步，无需手动发包
- 客户端旁观目标眼高已通过 `ClientEntity::eyeHeight()` 接口实现，根据实体类型和姿态返回正确的眼高值
