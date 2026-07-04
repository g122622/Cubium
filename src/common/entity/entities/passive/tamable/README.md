#可驯服动物

可被玩家驯服的动物实体。

##目录结构树

``` tamable /
├── TameableEntity.hpp / cpp #可驯服实体基类（IAngerable 接口、getTeam重写、wantsToAttack虚方法）
├── ShoulderRidingEntity.hpp #肩膀乘坐实体基类（鹦鹉专用）
├── WolfEntity.hpp / cpp #狼（骨头驯服、攻击保护、wantsToAttack过滤）
├── CatEntity.hpp / cpp #猫（生鱼驯服、11种皮肤、项圈染色、躺下 / 放松动画、晨间礼物、interactMob交互）
├── OcelotEntity.hpp / cpp #豹猫（信任机制、丛林生物）
└── ParrotEntity.hpp /
        cpp #鹦鹉（种子驯服、可站肩膀、不可繁殖）
```

        ##内部模块关系

``` AnimalEntity
└── TameableEntity(+IAngerable)
    ├── WolfEntity
    ├── CatEntity
    ├── OcelotEntity（信任机制，非完全驯服）
    └── ShoulderRidingEntity
        └── ParrotEntity(+IFlyingAnimal)
```

            ** 关键继承说明**： - `TameableEntity` 继承 `AnimalEntity` 并实现 `IAngerable` 接口
    - `OcelotEntity` 虽在 tamable 目录下，但使用信任机制（`isTrusting()`）而非标准驯服系统
    - `ShoulderRidingEntity` 是 `ParrotEntity` 专用的中间基类

    ##上下游外部依赖关系

    ## #依赖的上游模块
    - `entity / core /` - Entity,
    LivingEntity, MobEntity, CreatureEntity, AgeableEntity,
    AnimalEntity 基类 - `entity / interfaces / IAngerable.hpp` - 愤怒接口
    - `entity / interfaces / IFlyingAnimal.hpp` - 飞行动物接口（鹦鹉） - `entity / ai /` - Goal 系统（SwimGoal,
    PanicGoal, SitGoal, BreedGoal, TemptGoal, FollowOwnerGoal, FollowParentGoal, LeapAtTargetGoal, MeleeAttackGoal,
    AvoidEntityGoal 等） - `entity / attributes /` - 属性系统（MAX_HEALTH, MOVEMENT_SPEED,
    ATTACK_DAMAGE） - `world / IWorld.hpp` - 世界接口 - `item / Items.hpp` -
        物品定义（骨头、生鱼、种子、肉类等）

        ## #被下游模块依赖
        - `server / entity /` - 服务器端实体生成、AI 调度 - `client / renderer / entity /` - 客户端实体渲染
        - `world / spawn /` - 生物群系生成时的实体放置 - `entity / VanillaEntities.hpp` -
        实体类型注册

        ##容易踩的坑

        ## #驯服物品判断
        - **必须重写 `isTameItem()`**：不要在交互逻辑中硬编码物品类型，应重写虚方法 -
        **狼用骨头、猫用生鱼、鹦鹉用种子**：每种动物有特定驯服物品

         ## #AI 目标注册顺序
        - **优先级数字越小越优先**：Goal 注册时优先级参数 0 最高 -
        **子类必须调用父类 `registerGoals()`**：否则丢失基础行为 -
        **动态 AI 管理**：猫和豹猫需根据驯服 /
            信任状态动态添加移除 Goal，参考 `setupTamedAI()` 模式

            ## #OcelotEntity 信任机制
        - **不继承标准驯服系统**：豹猫用 `isTrusting()`/`setTrusting()` 而非 `isTamed()`/`setTamed()` -
        **网络同步**：信任状态通过 `DataParameter<
            bool> DATA_TRUSTING_PARAM` 同步到客户端，客户端通过 `ClientEntity::isTrusting()` 读取
        -
        **NBT 持久化**：信任状态通过 `addAdditionalSaveData`/`readAdditionalSaveData` 序列化为 `"Trusting"` 键（与 MC
            Java 一致）
        - **驯服概率不同**：豹猫信任建立是 1 / 3 概率，鹦鹉是 1 / 10，狼和猫是 1 / 3 -
        **猎物攻击目标**：豹猫目标选择器注册了小鸡攻击目标（NearestAttackableTargetGoal<
            ChickenEntity>，优先级1）和幼年海龟攻击目标（NearestAttackableTargetGoal<TurtleEntity>，优先级1，BABY_ON_LAND_SELECTOR
         过滤：仅攻击 `isChild() &&
    !isInWater()` 的海龟）

        ## #ParrotEntity 特殊性
        - **不能繁殖 * *：`isBreedingItem()` 始终返回 false，`spawnBaby()` 返回 nullptr -
        **可站肩膀 * *：通过 `ShoulderRidingEntity` 基类实现 -
        **免疫摔落伤害 *
            *：作为飞行动物

            ## #狼的食物系统
        - **狼可吃腐肉 * *：且不会获得饥饿效果，因为治疗逻辑只调用 `heal()` 不应用食物效果 -
        **驯服前后属性变化 *
            *：驯服后生命值 8→20，攻击力 2→4

            ## #狼的兴趣状态（乞求食物）元数据同步
        - **DataParameter * *：`WolfEntity::DATA_INTERESTED_PARAM`（bool）对应 MC 1.21.11 `Wolf.DATA_INTERESTED_ID` -
        **registerData 显式调用 * *：由于 C++ 虚函数在基类构造函数中不派发到派生类，
            `WolfEntity` 构造函数必须显式调用 `registerData()`（参考 `ZombieVillagerEntity` 模式），
            否则 `DATA_INTERESTED_PARAM` 永远不会注册到 `EntityDataManager`，成为死代码 -
        **BegGoal 驱动 * *：`BegGoal::startExecuting` 调用 `wolf.setInterested(true)`，
            `BegGoal::resetTask` 调用 `wolf.setInterested(false)`（对应 MC `BegGoal.start()/stop()`） -
        **客户端同步 * *：`ClientEntity::syncMetadataFromDataManager` 在 wolf 分支读取
            `DATA_INTERESTED_PARAM` 并调用 `setWolfIsInterested` 更新客户端镜像 -
        **动画插值 * *：`ClientEntity::tick` 根据 `m_wolfIsInterested` 推进
            `m_wolfInterestedAngle` 向 1.0/0.0 插值（系数 0.4，对应 MC Wolf.tick() 第 318-323 行） -
        **渲染 * *：`EntityRendererManager::updateAnimationContext` 通过
            `lerp(partialTick, wolfInterestedAngleO, wolfInterestedAngle)` 写入
            `AnimationContext::wolfInterestedAngle`，`WolfModel::setAnimState` 读取并应用到头部 Z 旋转

            ## #狼铠系统（WolfEntity + MobEntity + Crackiness）

        **狼铠装备 * *（`WolfEntity::interactMob` 优先级2）：主人右键狼 + 手持狼铠 + 未装备 + 非幼年 → 装备狼铠到 Body 槽位，播放 `ITEM_ARMOR_EQUIP_WOLF` 音效。调用 `MobEntity::setBodyArmorItem()` 自动设置保整掉落和持久化。

        **狼铠修复 * *（`WolfEntity::interactMob` 优先级3）：主人右键坐下的狼 + 手持犰狳鳞甲（`ItemTags::REPAIRS_WOLF_ARMOR`）+ 狼铠已受损 → 恢复 12.5% 最大耐久（`ARMOR_REPAIR_UNIT = 0.125F`），播放 `ENTITY_WOLF_ARMOR_REPAIR` 音效。

        **狼铠染色 * *（`WolfEntity::interactMob` 优先级4）：主人右键狼 + 手持染料 + 已装备狼铠 → 将当前颜色与染料颜色混合（`mixArmorColors` 取 RGB 平均值），调用 `DyeableArmorItem::setColor()` 写入 NBT。清除颜色：手持狼铠右键炼药锅（`LayeredCauldronBlock::_handleLeatherArmorCleaning`，继承自 `DyeableArmorItem` 自动支持）。

        **狼铠剪切 * *（`MobEntity::processInitialInteract` 剪刀分支）：主人手持剪刀 + `canShearEquipment()` 返回 true + 已装备狼铠 → `attemptToShearEquipment()` 剪下狼铠掉落为物品实体，剪刀耐久 -1，播放 `ITEM_ARMOR_UNEQUIP_WOLF` 音效，触发 `SHEAR` 游戏事件。`WolfEntity::canShearEquipment()` 重写为仅主人可剪切。

        **狼铠伤害吸收 * *（`WolfEntity::actuallyHurt`）：穿戴狼铠且伤害源不绕过护甲（`!source.bypassesArmor()`）时，伤害由狼铠耐久吸收（`LivingEntity::hurtAndBreak`，向上取整），狼不扣血。狼铠未破损时播放 `ENTITY_WOLF_ARMOR_DAMAGE`（通过 `getHurtSound` 返回值），裂纹等级变化时追加播放 `ENTITY_WOLF_ARMOR_CRACK`；狼铠破损时播放 `ENTITY_WOLF_ARMOR_BREAK`（取代受损音效）。

        **裂纹程度 * *（`Crackiness::WOLF_ARMOR`）：剩余耐久 < 95% → Low，< 69% → Medium，< 32% → High。用于音效触发，渲染层待实现。

        **音效**：`ITEM_ARMOR_EQUIP_WOLF`（装备）、`ITEM_ARMOR_UNEQUIP_WOLF`（剪切卸下）、`ENTITY_WOLF_ARMOR_DAMAGE`（狼铠吸收伤害时受伤音效，狼未破损时播放）、`ENTITY_WOLF_ARMOR_CRACK`（裂纹等级提升）、`ENTITY_WOLF_ARMOR_REPAIR`（修复）、`ENTITY_WOLF_ARMOR_BREAK`（破损，取代 DAMAGE）。

        **Crackiness 类**（`src/common/entity/core/Crackiness.hpp`）：通用裂纹追踪器，`byFraction`/`byDamage` 方法根据剩余耐久比例返回裂纹等级。`GOLEM`（铁傀儡）和 `WOLF_ARMOR`（狼铠）为静态常量。

            ## #动态 AI 移除
        - **猫驯服后移除躲避行为 * *：`setupTamedAI()` 中移除 `CatAvoidPlayerGoal` -
        **豹猫信任后移除躲避行为 *
            *：`setupTrustingAI()` 中移除 `OcelotAvoidPlayerGoal`

            ## #队伍继承与攻击过滤
        -
        **`getTeam()` 重写 *
            *：TameableEntity 重写了 `getTeam()`，已驯服动物继承主人的队伍，未驯服时回退到 AnimalEntity 默认逻辑
        -
        **`wantsToAttack(target, owner)` 虚方法 *
            *：TameableEntity 提供此虚方法供子类重写攻击过滤逻辑，默认返回 true（允许攻击所有目标）
        -
        **WolfEntity 重写 `wantsToAttack()`* *：实现 MC 精确的攻击过滤规则——永远不攻击苦力怕 / 恶魂 /
            盔甲架，不攻击已驯服的同类（除非主人不同），PvP保护检查（当目标和主人都是玩家时调用 `canHarmPlayer()`），不攻击已驯服的驯服动物和马
        -
        **OwnerHurtByTargetGoal / OwnerHurtTargetGoal 均调用 `wantsToAttack()`*
            *，因此狼的攻击限制自动生效

            ## #猫的交互系统（interactMob）
        - **交互优先级 * *（已驯服 + 主人）：①项圈染色（染料 + 颜色不同）→ ②喂食治疗（猫食
        + 生命未满）→ ③父类处理（繁殖 / 成长）→ ④切换坐下 / 站起 -
        **交互优先级 * *（未驯服）：猫食（生鳕鱼 / 生鲑鱼）→ 尝试驯服（1 / 3概率） -
        **非主人无法交互 * *：已驯服的猫只有主人可以交互，非主人交互直接传递给父类 -
        **驯服成功广播 * *：`broadcastEntityStatus(TamingSucceeded / TamingFailed)` 发送心形 / 烟雾粒子
        - ** 项圈染色**：默认红色，支持 17 种染料物品映射（16 种标准染料 + 骨粉 = 白色 + 墨囊 = 黑色等） -
        **食物治疗量 * *：生鳕鱼 / 生鲑鱼治疗 2.0 生命值 - **目标选择器 * *：猫的目标选择器已注册以下目标：
        - 优先级 1 : `NonTamedTargetGoal<RabbitEntity>` — 攻击兔子（未驯服时） -
    优先级 1 : `NonTamedTargetGoal<TurtleEntity>` — 攻击幼年海龟（未驯服时，仅陆地上的幼体）

    ## #TameableEntity NBT 序列化
    -
    **Sitting * *(byte / bool)-是否坐下 - **OwnerUUID * *(string)-主人 UUID - **AngerTime * *(i32)-愤怒剩余时间 -
    **CatEntity 额外字段 *
        *：CatType(i32) 猫皮肤类型、CollarColor(i32) 项圈颜色（默认红色时省略）

        ## #猫的动画状态系统
    -
    **DataParameter 同步 * *：`DATA_LYING_PARAM`(bool)和 `DATA_RELAX_STATE_ONE_PARAM`(bool)通过数据管理器同步到客户端 -
    **动画插值 *
        *：`lieDownAmount`/`lieDownAmountTail`/`relaxStateOneAmount` 使用前后 tick 值和 partialTick 进行平滑插值
    -
    **动画速率常量 * *：躺下 ±0.15 / +0.22、尾巴 ±0.08 /−0.13、放松 ±0.1 /−0.13（与 MC 1.21.11 一致） -
    **客户端同步 * *：ClientEntity 通过 `isCatLieDown()`/`isCatRelaxStateOne()` 读取同步状态 -
    **呼噜声 * *：躺下或放松时每 5 tick 播放 `ENTITY_CAT_PURR` -
    **睡眠检测 *
        *：`_handleLieDown()` 检测附近 2 格内是否有睡眠玩家，设置 `m_lyingOnTopOfSleepingPlayer`

        ## #猫的 AI 目标（CatGoals）
    -
    **CatLieOnBedGoal * *（优先级 5）：驯服猫寻找床并躺下，继承自 MoveToBlockGoal -
    **CatRelaxOnOwnerGoal * *（优先级 3）：驯服猫在睡眠主人身边放松→躺下→赠送晨间礼物 -
    **晨间礼物流程 * *：主人醒来时（`isPlayerFullyAsleep()` &&
    睡眠计时器 >= 100 tick），70 % 概率掉落礼物 -
            **礼物物品 * *：优先使用战利品表 `minecraft : gameplay / cat_morning_gift`，战利品表不可用时使用后备物品列表
            - **目标结束 * *：`resetTask()` 中检查主人是否充分睡眠后赠送礼物，然后清除所有状态
