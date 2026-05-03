#include "ZombieEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../combat/DifficultyHelper.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../world/block/BlockPos.hpp"

namespace mc {

ZombieEntity::ZombieEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // MC 1.16.5: 僵尸可以在阳光下燃烧
    setBurnsInDaylight(true);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> ZombieEntity::create(IWorld* /*world*/) {
    return std::make_unique<ZombieEntity>(LegacyEntityType::Unknown, 0);
}

std::optional<ResourceLocation> ZombieEntity::getAmbientSound() const {
    // MC 1.16.5: entity.zombie.ambient
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> ZombieEntity::getHurtSound(DamageSource& /*source*/) const {
    // MC 1.16.5: entity.zombie.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> ZombieEntity::getDeathSound() const {
    // MC 1.16.5: entity.zombie.death
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> ZombieEntity::getStepSound() const {
    // MC 1.16.5: entity.zombie.step
    return makeSoundEventId("step");
}

void ZombieEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/) {
    // MC 1.16.5: ZombieEntity.playStepSound()
    // 僵尸播放固定的脚步声，忽略脚下方块类型
    auto sound = getStepSound();
    if (sound) {
        playSound(*sound, 0.15f, 1.0f);
    }
}

void ZombieEntity::setBreakDoorsAbility(bool canBreak) {
    if (m_canBreakDoors == canBreak) {
        return;
    }

    m_canBreakDoors = canBreak;

    // MC 1.16.5: 动态添加/移除破门目标
    // 注意: 需要 BreakDoorGoal 实现
    // if (canBreak && m_breakDoorGoal == nullptr) {
    //     m_breakDoorGoal = new BreakDoorGoal(this);
    //     m_goalSelector.addGoal(1, m_breakDoorGoal);
    // } else if (!canBreak && m_breakDoorGoal != nullptr) {
    //     m_goalSelector.removeGoal(m_breakDoorGoal);
    //     m_breakDoorGoal = nullptr;
    // }
}

void ZombieEntity::startDrowning(i32 conversionTime) {
    // MC 1.16.5: 开始溺水转化
    m_converting = true;
    m_conversionTime = conversionTime;
}

void ZombieEntity::setBaby(bool baby) {
    if (m_isBaby == baby) {
        return;
    }

    m_isBaby = baby;
    refreshDimensions();

    // MC 1.16.5: 婴儿僵尸速度加成
    if (baby) {
        // 注意: 需要属性修饰符系统支持
        // m_attributes.applyModifier(SPEED_MODIFIER_BABY);
    } else {
        // m_attributes.removeModifier(SPEED_MODIFIER_BABY);
    }
}

bool ZombieEntity::canSummonReinforcements() const {
    // MC 1.16.5: 检查 ZOMBIE_SPAWN_REINFORCEMENTS 属性
    // 属性值范围 0.0-1.0，表示召唤概率
    // 注意: 需要属性系统支持 ZOMBIE_SPAWN_REINFORCEMENTS 属性
    // return m_attributes.getValue(Attributes::ZOMBIE_SPAWN_REINFORCEMENTS) > 0.0;
    return true;
}

void ZombieEntity::trySummonReinforcements() {
    // MC 1.16.5: 召唤增援逻辑在 hurt() 中处理
    // 此方法为占位符
}

bool ZombieEntity::hurt(DamageSource& source, f32 amount) {
    if (!MonsterEntity::hurt(source, amount)) {
        return false;
    }

    // MC 1.16.5: 增援召唤逻辑
    // 只在困难模式下有概率召唤增援
    IWorld* worldPtr = world();
    if (worldPtr && entity::combat::DifficultyHelper::canZombieReinforce(worldPtr->difficulty())) {
        // 注意: 需要属性系统获取召唤概率
        // f32 spawnChance = m_attributes.getValue(Attributes::ZOMBIE_SPAWN_REINFORCEMENTS);
        // if (worldPtr->random().nextFloat() < spawnChance) {
        //     // 在附近生成僵尸
        // }
    }

    return true;
}

bool ZombieEntity::attackEntityAsMob(LivingEntity& target) {
    // MC 1.16.5 ZombieEntity.attackEntityAsMob()
    // 首先调用父类方法进行基础攻击
    if (!MonsterEntity::attackEntityAsMob(target)) {
        return false;
    }

    // 燃烧传递逻辑
    // MC 1.16.5: 如果僵尸正在燃烧，且主手为空，且目标是生物
    // 则有概率点燃目标
    if (isOnFire() && getMainHandItem().isEmpty()) {
        IWorld* worldPtr = world();
        if (worldPtr) {
            // 获取区域难度（简化实现，直接使用难度）
            f32 regionalDifficulty = entity::combat::DifficultyHelper::getRegionalDifficultyBase(worldPtr->difficulty());

            // MC 1.16.5: 燃烧概率 = regionalDifficulty * 0.3
            // 区域难度 Easy=0.75, Normal=1.0, Hard=1.0
            // 所以概率大约是 22.5%, 30%, 30%
            math::Random rng = getRandom();
            if (rng.nextFloat() < regionalDifficulty * 0.3f) {
                // MC 1.16.5: 燃烧时间 = 2 * regionalDifficulty（秒）
                // 区域难度 0.75 -> 1.5秒 = 30 ticks
                // 区域难度 1.0 -> 2秒 = 40 ticks
                i32 fireDuration = static_cast<i32>(2.0f * regionalDifficulty * 20.0f);  // 转换为 ticks
                target.setFire(fireDuration);
            }
        }
    }

    return true;
}

void ZombieEntity::tick() {
    MonsterEntity::tick();

    // MC 1.16.5: 更新溺水转化
    updateDrowning();
}

void ZombieEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // MC 1.16.5 僵尸 AI 目标
    // 目标选择器（行为）
    // 优先级 2: 近战攻击
    m_goalSelector.addGoal(2, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));

    // 优先级 7: 避水随机行走
    // 注意: 需要 WaterAvoidingRandomWalkingGoal 实现
    // m_goalSelector.addGoal(7, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 1.0));
    m_goalSelector.addGoal(7, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f,
        [](const LivingEntity* /*entity*/) -> bool {
            // 注意: 需要 Player 类型完整定义来检查是否是玩家
            return true;
        }));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择器（攻击目标）
    // 优先级 2: 攻击玩家
    // 注意: 需要 Player 类型完整定义
    // m_targetSelector.addGoal(2, new entity::ai::goal::NearestAttackableTargetGoal<Player>(this, true));

    // 优先级 3: 攻击村民
    // 注意: 需要 VillagerEntity 实现
    // m_targetSelector.addGoal(3, new NearestAttackableTargetGoal<AbstractVillagerEntity>(this, true));

    // 优先级 3: 攻击铁傀儡
    // 注意: 需要 IronGolemEntity 实现

    // 优先级 5: 攻击海龟
    // 注意: 需要 TurtleEntity 实现
}

void ZombieEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // MC 1.16.5 僵尸属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, 2.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 35.0);  // MC 1.16.5: 35 而非默认 32

    // 注意: 需要属性系统支持 ZOMBIE_SPAWN_REINFORCEMENTS 属性
    // m_attributes.registerAttribute(Attributes::ZOMBIE_SPAWN_REINFORCEMENTS);
}

void ZombieEntity::convertToDrowned() {
    // MC 1.16.5: 转化为溺尸
    // TODO: 实现转化逻辑
    // 1. 创建新的 DrownedEntity
    // 2. 复制装备和状态
    // 3. 移除当前实体
    m_converting = false;
}

void ZombieEntity::updateDrowning() {
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // MC 1.16.5: 检查是否在水中
    bool inWater = isInWater();

    if (inWater && shouldDrown()) {
        m_inWaterTime++;

        // 开始转化条件：在水下 30 秒 (600 ticks)
        if (m_inWaterTime >= IN_WATER_TIME_THRESHOLD && !m_converting) {
            startDrowning(CONVERSION_DURATION);
        }
    } else {
        m_inWaterTime = 0;
    }

    // 更新转化计时器
    if (m_converting && m_conversionTime > 0) {
        m_conversionTime--;

        // 转化完成
        if (m_conversionTime <= 0) {
            convertToDrowned();
        }
    }
}

} // namespace mc
