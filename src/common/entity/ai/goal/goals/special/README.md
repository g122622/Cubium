#特殊 AI 目标(Special Goals)

本目录包含特定实体专用的 AI 目标，这些目标不适用于通用场景，而是为特定实体类型定制的行为。

        ##目录结构

``` special /
├── SpecialGoals.hpp /
        cpp #通用特殊目标（苦力怕膨胀、马驯服奔跑等）
├── GuardianAttackGoal.hpp / cpp #守卫者激光攻击目标
├── BlazeFireballAttackGoal.hpp / cpp #烈焰人火球攻击目标
├── BreezeGoals.hpp / cpp #旋风人目标（风弹射击、长跳、滑行、卡住射击）
├── EndermanGoals.hpp / cpp #末影人目标（注视玩家、查找玩家）
├── MoveToBlockGoal.hpp / cpp #移动到方块目标基类（搜索方块并导航移动）
├── MoveToLavaGoal.hpp / cpp #炽足兽寻找熔岩目标（继承自MoveToBlockGoal）
├── RaidGardenGoal.hpp / cpp #兔子偷胡萝卜目标（继承自MoveToBlockGoal，啃食成熟胡萝卜）
├── SquidGoals.hpp / cpp #鱿鱼目标（随机游泳、逃跑）
├── BatGoals.hpp / cpp #蝙蝠目标（随机飞行、挂墙休息）
├── DolphinGoals.hpp / cpp #海豚目标（跳跃、寻宝、与玩家同游、玩物品）
├── PhantomGoals.hpp / cpp #幻翼目标（环绕飞行、俯冲攻击）
├── SlimeGoals.hpp / cpp #史莱姆目标（漂浮、攻击、随机转向）
├── IronGolemGoals.hpp / cpp #铁傀儡目标（赠花给村民/铜傀儡、保护村庄、攻击）
├── CopperGolemGoals.hpp / cpp #铜傀儡物品运输目标（在铜箱子与普通箱子间转移物品）
├── EvokerGoals.hpp / cpp #唤魔者目标（尖牙攻击、召唤恼鬼、Wololo法术）
├── VexGoals.hpp / cpp #恼鬼目标（冲锋攻击、随机飞行、复制主人目标）
├── BeeGoals.hpp / cpp #蜜蜂目标（授粉、返回蜂巢、蛰刺攻击、作物生长促进）
├── CatGoals.hpp / cpp #猫目标（躺在床上、在主人身边放松）
├── FoxGoals.hpp / cpp #狐狸目标（跟踪猎物、扑击、睡眠、吃浆果）
├── PandaGoals.hpp / cpp #熊猫目标（打滚）
├── SilverfishGoals.hpp / cpp #蠹虫目标（藏入石头、召唤同伴）
├── WanderingTraderGoals.hpp /
        cpp #流浪商人目标（UseItemGoal、LookAtCustomerGoal、TradeWithPlayerGoal、MoveToWanderTargetGoal）
├── GhastGoals.hpp / cpp #恶魂目标（随机飞行、火球攻击）
├── TurtleGoals.hpp / cpp #海龟目标（下蛋、前往水域）
├── ShulkerGoals.hpp / cpp #潜影贝目标（攻击、张望、最近攻击目标选择、防御攻击目标选择）
├── IllusionerGoals.hpp / cpp #幻术师目标（失明法术、镜像法术）
├── AxolotlGoals.hpp / cpp #美西螈目标（攻击鱼、装死）
├── DrownedGoals.hpp / cpp #溺尸目标（前往水源、三叉戟攻击、近战攻击、前往海滩、向上游泳）
├ PatrolGoals.hpp / cpp #巡逻目标（掠夺者巡逻队）
├── RavagerGoals.hpp /
        cpp #劫掠兽目标
└── README.md #本文档
```

        ##内部模块关系

``` Goal(基类)
├── CreeperSwellGoal ─────────────── 苦力怕膨胀爆炸
├── RunAroundLikeCrazyGoal ───────── 未驯服马疯狂奔跑
├── LlamaFollowCaravanGoal ──────── 羊驼跟随商队（两阶段搜索、拴绳检查、栅栏拴绳早返回）
├── LlamaDefendTargetGoal ───────── 羊驼防御目标（吐口水反击）
├── TraderLlamaDefendWanderingTraderGoal ── 商队羊驼保卫流浪商人
├── TriggerSkeletonTrapGoal ──────── 骷髅马陷阱触发
├── GuardianAttackGoal ───────────── 守卫者激光攻击（充能→发射→冷却）
├── BlazeFireballAttackGoal ──────── 烈焰人连发火球（充能→连发→冷却）
├── BreezeShootGoal ─────────────── 旋风人风弹射击（充能→发射→恢复→冷却）
├── BreezeLongJumpGoal ──────────── 旋风人长跳（吸气→跳跃→着陆）
├── BreezeSlideGoal ─────────────── 旋风人滑行（逃离内圈或移动到中圈
        / 目标身后）
├── BreezeShootWhenStuckGoal ────── 旋风人卡住时紧急射击（一次性目标，设置射击许可）
├── MoveToBlockGoal(抽象基类)
│   ├── MoveToLavaGoal ───────────── 炽足兽寻找熔岩
│   ├── DrownedGoToBeachGoal ─────── 溺尸前往海滩
│   ├── CatLieOnBedGoal ──────────── 猫躺在床上
│   └── RaidGardenGoal ──────────── 兔子偷胡萝卜（饥饿时啃食成熟胡萝卜）
├── DrownedGoToWaterGoal ─────────── 溺尸前往水源（白天陆地上寻找水）
├── DrownedTridentAttackGoal : RangedAttackGoal ─ 溺尸三叉戟远程攻击
├── DrownedAttackGoal : MeleeAttackGoal ─ 溺尸近战攻击（带 okTarget 过滤）
├── DrownedSwimUpGoal ────────────── 溺尸向上游泳（夜间深水向海面游泳）
├── EndermanStareGoal ────────────── 末影人注视玩家
├── EndermanFindPlayerGoal ───────── 末影人查找注视自己的玩家
├── PuffGoal ─────────────────────── 河豚膨胀
├── SquidMoveRandomGoal ──────────── 鱿鱼随机游泳
├── SquidFleeGoal ────────────────── 鱿鱼逃跑
├── BatRandomFlyGoal ─────────────── 蝙蝠随机飞行
├── BatRestGoal ──────────────────── 蝙蝠挂墙休息
├── DolphinJumpGoal ──────────────── 海豚跳跃
├── SwimToTreasureGoal ───────────── 海豚引导玩家到宝藏
├── SwimWithPlayerGoal ───────────── 海豚与玩家同游（给予海豚恩惠）
├── PlayWithItemsGoal ────────────── 海豚玩物品
├── PhantomAttackPlayerTargetGoal ── 幻翼攻击玩家目标选择器
├── PhantomOrbitPointGoal ────────── 幻翼环绕飞行
├── PhantomPickAttackGoal ────────── 幻翼攻击阶段选择
├── PhantomSweepAttackGoal ───────── 幻翼俯冲攻击
├── OfferFlowerGoal ─────────────── 铁傀儡向村民/铜傀儡赠花
├── TransportItemsBetweenContainersGoal ── 铜傀儡物品运输（主手空取铜箱物品，主手有物放普通箱）
├── EvokerAttackSpellGoal ────────── 唤魔者尖牙攻击
├── EvokerSummonSpellGoal ────────── 唤魔者召唤恼鬼
├── EvokerWololoSpellGoal ────────── 唤魔者蓝色羊变红
├── IllusionerSpellGoal(抽象基类) ─ 幻术师法术基类（施法准备→施法→冷却生命周期）
│   ├── IllusionerBlindnessSpellGoal ─ 幻术师失明法术（难度
    >= Normal，不能重复施法同一目标）
│   └── IllusionerMirrorSpellGoal ─── 幻术师镜像法术（隐身 + 分身，未隐身时施放）
├── VexChargeAttackGoal ──────────── 恼鬼冲锋攻击
├── VexMoveRandomGoal ────────────── 恼鬼随机飞行
├── VexCopyOwnerTargetGoal ───────── 恼鬼复制主人目标
├── BeePassiveGoal(抽象基类)
│   ├── BeeEnterHiveGoal ─────────── 蜜蜂进入蜂巢
│   ├── BeePollinateGoal ─────────── 蜜蜂授粉
│   ├── BeeUpdateHiveGoal ────────── 蜜蜂更新蜂巢位置
│   ├── BeeFindHiveGoal ──────────── 蜜蜂寻找蜂巢
│   ├── BeeFindFlowerGoal ────────── 蜜蜂寻找花朵
│   └── BeeFindPollinationTargetGoal 蜜蜂寻找授粉目标（促进作物生长）
├── BeeStingGoal : MeleeAttackGoal ─ 蜜蜂蛰刺攻击（攻击后死亡）
├── BeeWanderGoal ────────────────── 蜜蜂随机飞行（使用RandomPositionGenerator，离蜂巢远时偏向回飞）
├── BeeAngerGoal : HurtByTargetGoal─ 蜜蜂愤怒（召唤同伴）
├── BeeAttackPlayerGoal : TargetGoal 蜜蜂攻击玩家（使用EntityUtils::findClosestEntity<Player> 搜索）
├── BeeResetAngerGoal ────────────── 蜜蜂重置愤怒（愤怒时间结束后清除愤怒状态）
├── ShulkerNearestAttackGoal : NearestAttackableTargetGoal 潜影贝攻击最近玩家（和平难度下禁用）
├── ShulkerDefenseAttackGoal : NearestAttackableTargetGoal 潜影贝防御攻击（队伍中的潜影贝攻击IMob）
├── CatLieOnBedGoal : MoveToBlockGoal ─ 猫躺在床上（驯服猫寻找床并躺下）
├── CatRelaxOnOwnerGoal : Goal ─ 猫在睡眠主人身边放松（看向主人→躺下→赠送礼物）
├── FoxPassiveGoal(抽象基类)
│   ├── FoxFindShelterGoal ───────── 狐狸寻找庇护所
│   ├── FoxSleepGoal ─────────────── 狐狸睡眠
│   └── FoxSitAndLookGoal ────────── 狐狸坐下观察
├── FoxFollowTargetGoal ──────────── 狐狸跟踪猎物
├── FoxPounceGoal ────────────────── 狐狸扑击
├── FoxBiteGoal : MeleeAttackGoal ── 狐狸咬击
├── PandaRollGoal ────────────────── 熊猫打滚
├── SilverfishHideInStoneGoal ────── 蠹虫藏入石头
├── SilverfishSummonOthersGoal ───── 蠹虫召唤同伴
├── UseItemGoal ──────────────────── 通用物品使用目标（通过条件函数控制，如流浪商人夜间喝隐身药水、白天喝牛奶）
├── LookAtCustomerGoal ───────────── 看向顾客（使用 LookController）
├── TradeWithPlayerGoal ──────────── 与玩家交易（占用 Jump +
                   Move 标志，交易时停止导航）
└── MoveToWanderTargetGoal ───────── 向游荡目标移动（分段接近策略：远距离先移动10格中间航点）
```

                   ##上下游外部依赖关系

                       ** 上游依赖（本目录依赖的模块）**： - `src / common / entity / ai / goal /` -
                   Goal 基类、GoalSelector、MeleeAttackGoal、TargetGoal、RandomWalkingGoal 等
                   - `src / common / entity /` -
                   各实体类型（CreeperEntity、BlazeEntity、GuardianEntity 等） - `src / common / world /` -
                   IWorld、BlockState、FluidState - `src / common / world / block /` -
                   Block、BlockTags、IGrowable、CropBlock、StemBlock、SweetBerryBushBlock - `src / common / util /` -
                   Random、AxisAlignedBB、Direction - `src / common / core /` -
                   Constants（游戏常量）

                       ** 下游依赖（依赖本目录的模块）**： - `src / common / entity /` -
                   各实体类型的 `registerGoals()` 方法中注册这些特殊目标 - `src / server / entity /` -
                   服务端实体 AI 调度

                   ##容易踩的坑

                   ## #1. 互斥标志设置错误导致 AI 行为冲突

                       ** 问题**：特殊目标与其他目标冲突，导致实体行为异常（如一边攻击一边逃跑）。

                           ** 解决**：始终设置正确的互斥标志。常见组合：
                   - `Move` -
                   移动类目标（导航、游泳、飞行） - `Look` - 看向类目标（看向玩家、看向目标） - `Jump` - 跳跃类目标
                   - `Target` -
                   目标选择器专用（不应与 Goal 的 Move /
                       Look 混用）

```cpp
                       // 正确示例：攻击目标需要控制移动和看向
                       BlazeFireballAttackGoal::BlazeFireballAttackGoal(BlazeEntity* blaze)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
```

    ## #2. 空指针检查缺失

    * *问题 * *：实体指针在目标执行期间可能失效（如目标死亡、卸载区块）。

    * *解决 *
    *：在 `shouldExecute()`、`tick()`、`resetTask()` 等方法开始处检查空指针。

```cpp void CreeperSwellGoal::tick()
{
    if (!m_creeper) return; // 必须检查
    if (!m_attackTarget || !m_attackTarget->isAlive()) {
        m_creeper->setCreeperState(-1);
        return;
    }
    // 正常逻辑...
}
```

    ## #3. 距离计算频繁使用 sqrt

        ** 问题**：频繁调用 `sqrt()` 严重影响性能，尤其是每 tick 执行的目标。

            ** 解决**：使用距离平方比较，避免 `sqrt()` 调用。

```cpp
    // 低效
    f32 distance = std::sqrt(dx * dx + dy * dy + dz * dz);
if (distance < 7.0f) {}

// 高效
f32 distSq = dx * dx + dy * dy + dz * dz;
if (distSq < 49.0f) {} // 7 * 7 = 49
```

    ## #4. BeePassiveGoal 模板方法模式理解错误

        ** 问题**：子类重写 `shouldExecute()` 而非 `canBeeStart()`，导致蜜蜂愤怒时不被打断。

            ** 解决**：BeePassiveGoal
    使用模板方法模式，子类应实现 `canBeeStart()` 和 `canBeeContinue()`，而非直接重写 `shouldExecute()`。

    ## #5. MoveToBlockGoal 的螺旋搜索理解错误

        ** 问题**：子类实现 `shouldMoveTo()` 时误以为会自动处理 Y 轴搜索。

            ** 解决**：MoveToBlockGoal 的 `searchForDestination()` 使用 Y 轴交替搜索（0,
    1, -1, 2,
    -2...），`shouldMoveTo()` 只需判断目标位置是否符合条件，搜索逻辑由基类处理。

        ## #6. 幻翼攻击阶段切换逻辑复杂

            ** 问题**：幻翼有 CIRCLE（环绕）和 SWOOP（俯冲）两个阶段，切换时机难以理解。

                ** 解决**： - `PhantomPickAttackGoal` 负责阶段切换决策
        - `PhantomOrbitPointGoal` 执行环绕飞行 - `PhantomSweepAttackGoal` 执行俯冲攻击 -
        三个目标配合工作，通过 PhantomEntity 的状态标志协调

            ## #7. 狐狸扑击状态的正确设置顺序

                ** 问题**：狐狸扑击需要蹲伏→感兴趣→扑击的状态链，顺序错误会导致扑击失败。

                    ** 解决**： 1. `FoxFollowTargetGoal` 设置蹲伏和感兴趣状态 2. `FoxPounceGoal` 检测完全蹲伏后执行扑击
            3. 确保 `isFullyCrouched()` 检查所有前置状态

            ## #8. 守卫者
            /
            烈焰人攻击的充能阶段

                ** 问题**：攻击目标的充能计时器从负数开始，容易理解错误。

                    ** 解决**： -
        GuardianAttackGoal：tickCounter 从 - 10 开始，0 时发送音效，80 时造成伤害
        - BlazeFireballAttackGoal：chargeTime 从 0 开始递增到 60，然后进入火球阶段 -
        注意两个目标的计时逻辑不同

        ## #9. 末影人瞬移冷却

            ** 问题**：末影人频繁瞬移导致难以攻击或攻击过于简单。

                ** 解决**：EndermanFindPlayerGoal 有瞬移冷却（30 ticks），近距离（ <
    4 格）瞬移躲避，远距离（ > 16 格）瞬移接近。

        ## #10. 蠹虫召唤同伴的 mobGriefing 检查

            ** 问题**：mobGriefing 规则影响蠹虫召唤同伴的行为。

                ** 解决**： - `mobGriefing = true`：破坏虫蚀方块并生成蠹虫
    - `mobGriefing = false`：只将虫蚀方块转换为原版方块，不生成蠹虫

    ## #11. 史莱姆攻击目标的创造
    / 旁观者检查

    * *问题 * *：史莱姆攻击目标未检查玩家游戏模式，导致创造 / 旁观者玩家也被追击。

    * *解决 *
    *：`SlimeAttackGoal::shouldExecute()` 需要使用 `dynamic_cast<Player*>(
        target)` 检查目标是否为玩家，并检查 `isCreative()` 或 `isSpectator()`，如果是则不执行攻击。

```cpp auto* targetPlayer = dynamic_cast<Player*>(target);
if (targetPlayer != nullptr && (targetPlayer->isCreative() || targetPlayer->isSpectator())) {
    return false;
}
```

    ## #12. 史莱姆随机转向目标的漂浮效果检查

        ** 问题**：有漂浮效果的史莱姆在空中不会随机转向，导致漂浮时行为不自然。

            ** 解决**：`SlimeFaceRandomGoal::shouldExecute()` 需要检查 `hasEffect(EffectType::Levitation)`，与 MC 一致：

```cpp return m_slime->onGround() ||
    m_slime->isInWater() || m_slime->isInLava() || m_slime->hasEffect(entity::effect::EffectType::Levitation);
```

            ## #13. 美西螈装死时给予再生效果

                **问题 **：美西螈装死时不给予再生效果，缺少 MC 原版的生存优势。

                    **解决 **：`AxolotlPlayDeadGoal::startExecuting()` 中添加 `addEffect(
                        EffectInstance(EffectType::Regeneration, 200))`，给予 Regeneration I 效果 200 ticks(10秒)。

            ## #14. 烈焰人火球发射音效

                **问题 **：烈焰人发射火球时缺少音效事件通知。

                    **解决 **：`BlazeFireballAttackGoal::_performFireballAttack()` 中在 `spawnEntity` 前添加 `world
                        ->playEvent(WorldEvents::BLAZE_SHOOT_SOUND, ...)` 播放音效事件(ID 1018)。

            ## #15. 蜜蜂授粉目标的作物生长逻辑

                **问题 **：蜜蜂促进作物生长时需区分不同作物类型的生长方式，逻辑容易出错。

                    **解决 **：`BeeFindPollinationTargetGoal::_growCrop()` 按以下优先级处理： 1. *
            *CropBlock **（小麦、胡萝卜、马铃薯、甜菜根）：检查 `isMaxAge()`，未成熟则 `age
        + 1` 2. * *StemBlock **（西瓜茎、南瓜茎）：检查 `AGE_0_7 <
    7`，未成熟则 `age + 1` 3. * *SweetBerryBushBlock **：检查 `isMaxAge()`，未成熟则 `age +
        1` 4. * *IGrowable **（CaveVines / CaveVinesPlant）：调用 `canGrow()` + `grow()` 接口

        注意事项：
        - 蜜蜂授粉只增加1个生长阶段（不同于骨粉的2 -
        5个阶段） - 作物计数器 `MAX_CROPS_GROWN = 10`：每次采粉后最多促进10棵作物
    - `canBeeStart()` 和 `canBeeContinue()` 均检查作物计数器上限
    - `BeeEnterHiveGoal::startExecuting()` 调用 `resetCropCounter()` 重置计数器 -
    生长粒子使用 `WorldEvents::PLANT_GROWTH_PARTICLES(2011)`，与骨粉使用的 `BONEMEAL_PARTICLES(
        2005)` 的区别是不播放骨粉使用音效
    - `BEE_GROWABLES` 标签包含：wheat,
                               carrots, potatoes, beetroots, melon_stem, pumpkin_stem, sweet_berry_bush, cave_vines,
                               cave_vines_plant

        ## #16. 蜜蜂授粉状态管理与服务端冷却

            **问题 **：BeePollinateGoal
        的授粉状态（setPollinating）和服务端冷却递减逻辑容易遗漏或放错位置。

            **解决 *
                *： - `BeePollinateGoal::startExecuting()` 必须调用 `m_bee
                          ->setPollinating(true)`，`resetTask()` 必须调用 `m_bee
                          ->setPollinating(false)` 并设置花朵冷却
                      200 tick（`m_bee->setFlowerCooldown(
                          200)`） - `BeeEntity::
                                        tick()` 中的三个冷却计时器（`m_stayOutOfHiveCountdown`、`m_remainingCooldownBeforeLocatingNewHive`、`m_remainingCooldownBeforeLocatingNewFlower`）必须在服务端守卫 `!m_world
                                            ->isClientSide()` 内递减，客户端不递减
        - `BeePollinateGoal::canBeeStart()` 在花朵冷却
    > 0 时返回 false，阻止蜜蜂在刚完成授粉后立即重新授粉

        ## #17. 蜜蜂花朵判定中的向日葵半方块处理

            **问题 *
                *：蜜蜂 `_isFlower()` 判定花朵时，向日葵（Sunflower）是双格高植物（DoublePlantBlock），下半方块不是真正的花朵，蜜蜂不应停留在向日葵的下半部分授粉。

                    **解决 **：`BeePollinateGoal::_isFlower()` 中对 `TALL_FLOWERS` 标签方块做特殊处理：
        - 向日葵（Sunflower）：检查 `DOUBLE_BLOCK_HALF` 属性，只有上半（Upper）才返回 true -
        其他高花（丁香、玫瑰丛、牡丹、大型蕨）：上下半均返回 true（与 MC 一致）

```cpp if (BlockTags::TALL_FLOWERS().contains(*state))
{
    if (state->is(block_registry::VegetationBlocks::SUNFLOWER)) {
        auto half = state->get(BlockStateProperties::DOUBLE_BLOCK_HALF());
        return half == blocks::DoublePlantBlock::DoubleBlockHalf::Upper;
    }
    return true;
}
```

            ## #18. 蜜蜂攻击玩家目标的目标搜索

                ** 问题**：`BeeAttackPlayerGoal` 需要在范围内搜索最近的玩家作为目标，手动搜索代码冗长且容易出错。

                    ** 解决**：使用 `EntityUtils::findClosestEntity<
                        Player>()` 工具方法，传入蜜蜂位置和搜索范围（10格），返回最近的未创造
            /
            旁观模式的玩家。

            ## #19. 蜜蜂漫游目标和导航的智能位置生成

                **解决**：`BeeWanderGoal` 使用 MC 1.21.11 对应的空中位置算法生成飞行位置：
        - 主策略：`RandomPositionGenerator::findHoverPosition()` 对应 `HoverRandomPos.getPos`，在固体方块上方1~3格范围内选择悬停位置，确保有足够空气空间
        - 备选策略：`RandomPositionGenerator::findAirAndWaterPosition()` 对应 `AirAndWaterRandomPos.getPos`，向上移出固体方块即可
        - 方向偏好：离蜂巢超过阈值时飞回蜂巢方向，否则使用朝向；阈值计算对应 MC 的 `getWanderThreshold()`
        - `WaterAvoidingRandomFlyingGoal` 同样使用 `findHoverPosition` + `findAirAndWaterPosition` 替代原来的 `findRandomTargetBlock`
        - `BeeEntity::pathfindRandomlyTowards()` 对应 MC 1.21.11 的 `Bee.pathfindRandomlyTowards()`，使用 `findAirPositionTowards` 在目标方向18度锥形内生成随机空中航点，产生蜜蜂漂移飞行效果；被 `BeeFindHiveGoal`（远距离时）和 `BeeFindFlowerGoal` 调用
        - `BeeEntity::pathfindDirectlyTowards()` 对应 MC 1.21.11 的 `BeeGoToHiveGoal.pathfindDirectlyTowards()`，近距离（16格内）精确导航，用于 `BeeFindHiveGoal`
        - `BeeFindHiveGoal` 和 `BeeFindFlowerGoal` 的 `_isTooFar` 距离阈值从32格修正为48格（对应 MC 的 `isTooFarAway` → `!closerThan(48)`）

            ## #20. 幻术师法术 CASTING_TIME 必须为 20

            * *问题**：`IllusionerBlindnessSpellGoal` 和 `IllusionerMirrorSpellGoal` 的 `CASTING_TIME` 被误设为
               0，导致施法动画时间为 0 tick，施法立即完成。

                   ** 解决**：MC 1.21.11 中 `IllusionerSpellGoal.getCastingTime()` 返回
               20，两个子类的 `CASTING_TIME` 都应为
               20。施法时间决定了实体在施法姿态中停留的 tick 数，`m_spellTicks = warmup +
            castingTime`。

                ## #21. 幻术师失明法术的难度检查

                * *问题 * *：`IllusionerBlindnessSpellGoal::shouldExecute()` 的难度检查使用 `difficulty
        !=
        Difficulty::Hard`，导致只在困难难度施放失明法术，与 MC 原版不一致。

                * *解决 * *：MC 1.21.11 使用 `world.getDifficulty().isHarderThan(Difficulty.NORMAL)`，即难度
            >= Normal（Normal 和 Hard）时可以施放。应使用 `difficulty < Difficulty::Normal` 判断不施放。

                    ## #22. 幻术师法术的施法准备音效

                    * *问题 *
                    *：`IllusionerSpellGoal::
                        startExecuting()` 未播放施法准备音效，玩家无法听到幻术师正在准备施法的提示音。

                    * *解决 * *：在 `startExecuting()` 中通过 `getSpellPrepareSoundId()` 获取准备音效并播放：
                - 失明法术：`entity.illusioner.prepare_blindness` - 镜像法术：`entity.illusioner.prepare_mirror` -
                施法完成时播放共用音效：`entity.illusioner.cast_spell`

                    ## #23. 幻术师失明法术不可重复施放同一目标

                    * *问题 *
                    *：`IllusionerBlindnessSpellGoal` 缺少对同一目标的重复施法检查，导致幻术师可能反复对同一目标施加失明效果。

                    * *解决 * *：`shouldExecute()` 中检查 `target->id() ==
        m_lastTargetId`，如果目标 ID 与上次施法目标相同则不执行。`startExecuting()` 中记录 `m_lastTargetId`。

                ## #24. LlamaFollowCaravanGoal 拴绳逻辑

                * *问题 *
                *：`LlamaFollowCaravanGoal` 的 `shouldExecute()` 和 `tick()` 拴绳逻辑实现不完整，导致商队行为与原版不一致。

                * *解决 *
                *：三个关键点：

                1. *
                *`_firstIsLeashed()` 递归检查 *
                *：沿 `caravanHead` 链向上追溯时必须检查 `isLeashed()` 状态，到达链头时返回 `head
                     ->isLeashed()` 而非简单返回 `true`。递归深度限制为 `MAX_CARAVAN_LENGTH(8)`。

                 2. *
                *`shouldExecute()` 两阶段搜索 * *： -
            前置条件：自己未被拴绳拴住且未在商队中 - 第一阶段：寻找已在商队中但无尾部的羊驼（`isInCaravan() &&
    !hasCaravanTail()`） - 第二阶段：若未找到，寻找被拴住且无尾部的羊驼（`isLeashed() &&
    !hasCaravanTail()`） - 最终检查：候选羊驼自己未被拴住且链上也无被拴住的羊驼时不加入（`!candidate->isLeashed() &&
    !_firstIsLeashed(candidate, 1)`）

        3. *
        *`tick()` 栅栏拴绳早返回 * *：被拴在栅栏柱上的羊驼（`isLeashed() &&
    leashFencePos().has_value()`）不移动跟随商队。

    ## #25. 铜傀儡物品运输目标的三状态机与 MC 1.21.11 常量

        **问题**：`TransportItemsBetweenContainersGoal` 的状态机（TRAVELLING → QUEUING → INTERACTING）与 MC 1.21.11 `TransportItemsBetweenContainers` 行为常量需要对齐，否则铜傀儡物品运输行为会偏离原版。

        **解决**：关键常量与时序点：
        - 搜索半径：`HORIZONTAL_SEARCH_RADIUS=32`、`VERTICAL_SEARCH_RADIUS=8`（对应 MC `TRANSPORTED_ITEMS_SEARCH_RADIUS`）
        - 交互时长：`TARGET_INTERACTION_TIME=60` tick（tick 1 startOpen+setOpenedChestPos+setState、tick 9 playSound、tick 60 transfer+stopOpen+clearOpenedChestPos）
        - 运输堆叠上限：`TRANSPORTED_ITEM_MAX_STACK_SIZE=16`
        - 冷却：`IDLE_COOLDOWN=140` tick（运输完成后进入冷却）
        - 位置记忆：`MAX_VISITED_POSITIONS=10`（已访问位置集）、`MAX_UNREACHABLE_POSITIONS=50`（不可达位置集）
        - 交互触发距离：水平 `CLOSE_ENOUGH_TO_INTERACT_SQ=1.0`（1 格内）、垂直 `CLOSE_ENOUGH_VERTICAL=2.0`
        - 目标筛选：主手为空 → 在 `BlockTags::COPPER_CHESTS` 中找非空铜箱子（取物）；主手非空 → 在普通箱子（`VanillaBlocks::CHEST`/`TRAPPED_CHEST`）中找可堆叠箱子（放物）
        - 双箱处理：`_tickInteracting` tick 1 在目标箱子及其连通箱子（`ChestBlock::getConnectedDirection`）上分别调用 `startOpen`/`stopOpen`，对应 MC `CompoundContainer` 转发
        - 物品转移顺序：`_addItemsToContainer` 单次遍历容器，同时处理空槽（整堆放入）和可堆叠槽（增量堆叠），与 MC 原版 `addItemsToContainer` 一致（与项目 `IInventory::addItem` 默认顺序相反）
