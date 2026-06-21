/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ZombieEntity.hpp"
#include "DrownedEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/interact/BreakDoorGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySpawnPlacementRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/SpecialDates.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {

// 使用序列化命名空间
using namespace entity::serialization;

// 属性修饰符ID常量
namespace {
const std::string BABY_SPEED_BOOST_ID = "baby";
const std::string REINFORCEMENT_CALLER_CHARGE_ID = "reinforcement_caller_charge";
const std::string ZOMBIE_REINFORCEMENT_CALLEE_CHARGE_ID = "reinforcement_callee_charge";
const std::string LEADER_ZOMBIE_BONUS_ID = "leader_zombie_bonus";
const std::string ZOMBIE_RANDOM_SPAWN_BONUS_ID = "zombie_random_spawn_bonus";
const std::string RANDOM_SPAWN_BONUS_ID = "random_spawn_bonus";
} // namespace

ZombieEntity::ZombieEntity(EntityId id)
    : MonsterEntity(id)
{
    // 僵尸可以在阳光下燃烧
    setBurnsInDaylight(true);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> ZombieEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ZombieEntity>(EntityId(0));
}

std::optional<ResourceLocation> ZombieEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> ZombieEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> ZombieEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> ZombieEntity::getStepSound() const
{
    return makeSoundEventId("step");
}

void ZombieEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 僵尸播放固定的脚步声，忽略脚下方块类型
    auto sound = getStepSound();
    if (sound) {
        playSound(*sound, 0.15f, 1.0f);
    }
}

void ZombieEntity::setBreakDoorsAbility(bool canBreak)
{
    if (m_canBreakDoors == canBreak) {
        return;
    }

    m_canBreakDoors = canBreak;

    // 对应 MC Java 版 Zombie.setCanBreakDoors：
    // 破门能力与导航器开门能力绑定
    auto* nav = navigator();
    if (nav) {
        nav->setCanOpenDoors(canBreak);
    }

    // 动态添加/移除破门目标
    if (canBreak && m_breakDoorGoal == nullptr) {
        m_breakDoorGoal =
            new entity::ai::goal::BreakDoorGoal(this, entity::ai::goal::defaultDoorBreakDifficultyPredicate());
        m_goalSelector.addGoal(1, m_breakDoorGoal);
    } else if (!canBreak && m_breakDoorGoal != nullptr) {
        m_goalSelector.removeGoal(m_breakDoorGoal);
        m_breakDoorGoal = nullptr;
    }
}

void ZombieEntity::startDrowning(i32 conversionTime)
{
    // 开始溺水转化
    m_converting = true;
    m_conversionTime = conversionTime;
}

void ZombieEntity::setBaby(bool baby)
{
    if (m_isBaby == baby) {
        return;
    }

    m_isBaby = baby;
    refreshDimensions();

    // 婴儿僵尸速度加成：+50%（MultiplyBase 操作）
    if (baby) {
        entity::attribute::AttributeModifier modifier(
            BABY_SPEED_BOOST_ID, "Baby speed boost", 0.5, entity::attribute::Operation::MultiplyBase);
        m_attributes.addModifier(entity::attribute::Attributes::MOVEMENT_SPEED, modifier);
    } else {
        m_attributes.removeModifier(entity::attribute::Attributes::MOVEMENT_SPEED, BABY_SPEED_BOOST_ID);
    }
}

bool ZombieEntity::canSummonReinforcements() const
{
    return m_attributes.getValue(entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS) > 0.0;
}

void ZombieEntity::trySummonReinforcements()
{
    // 召唤增援逻辑在 hurt() 中处理
    // 此方法为占位符
}

bool ZombieEntity::hurt(DamageSource& source, f32 amount)
{
    if (!MonsterEntity::hurt(source, amount)) {
        return false;
    }

    // 增援召唤逻辑
    // 只在困难模式下有概率召唤增援
    IWorld* worldPtr = world();
    if (worldPtr && entity::combat::DifficultyHelper::canZombieReinforce(worldPtr->difficulty())) {
        f64 spawnChance = m_attributes.getValue(entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS);
        if (getRandom().nextDouble() < spawnChance) {
            // TODO: 在附近生成增援僵尸实体（需要实体生成系统完善后实现）
        }
    }

    return true;
}

bool ZombieEntity::attackEntityAsMob(LivingEntity& target)
{
    // 首先调用父类方法进行基础攻击
    if (!MonsterEntity::attackEntityAsMob(target)) {
        return false;
    }

    // 燃烧传递逻辑
    // 如果僵尸正在燃烧，且主手为空，且目标是生物
    // 则有概率点燃目标
    if (isOnFire() && getMainHandItem().isEmpty()) {
        IWorld* worldPtr = world();
        if (worldPtr) {
            // 获取区域难度（简化实现，直接使用难度）
            f32 regionalDifficulty =
                entity::combat::DifficultyHelper::getRegionalDifficultyBase(worldPtr->difficulty());

            // 燃烧概率 = regionalDifficulty * 0.3
            // 区域难度 Easy=0.75, Normal=1.0, Hard=1.0
            // 所以概率大约是 22.5%, 30%, 30%
            math::Random rng = getRandom();
            if (rng.nextFloat() < regionalDifficulty * 0.3f) {
                // 燃烧时间 = 2 * regionalDifficulty（秒）
                // 区域难度 0.75 -> 1.5秒 = 30 ticks
                // 区域难度 1.0 -> 2秒 = 40 ticks
                i32 fireDuration = static_cast<i32>(2.0f * regionalDifficulty * 20.0f); // 转换为 ticks
                target.setFire(fireDuration);
            }
        }
    }

    return true;
}

void ZombieEntity::tick()
{
    MonsterEntity::tick();

    // 更新溺水转化
    _updateDrowning();
}

void ZombieEntity::registerGoals()
{
    // 调用父类方法
    MonsterEntity::registerGoals();

    // 僵尸 AI 目标
    // 目标选择器（行为）
    // 优先级 2: 近战攻击
    m_goalSelector.addGoal(2, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));

    // 注意：MC原版僵尸不会躲避阳光，它们只是直接在阳光下燃烧
    // 骷髅才会使用 FleeSunGoal 和 RestrictSunGoal 来躲避阳光

    // 优先级 7: 避水随机行走
    m_goalSelector.addGoal(7, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(
        8, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* /*entity*/) -> bool {
            // TODO: 需要 Player 类型完整定义来检查是否是玩家
            return true;
        }));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择器（攻击目标）
    // 优先级 2: 攻击玩家
    // TODO: 需要 Player 类型完整定义
    // m_targetSelector.addGoal(2, new entity::ai::goal::NearestAttackableTargetGoal<Player>(this, true));

    // 优先级 3: 攻击村民
    // TODO: 需要 VillagerEntity 实现
    // m_targetSelector.addGoal(3, new NearestAttackableTargetGoal<AbstractVillagerEntity>(this, true));

    // 优先级 3: 攻击铁傀儡
    // TODO: 需要 IronGolemEntity 实现

    // 优先级 5: 攻击海龟
    // TODO: 需要 TurtleEntity 实现
}

void ZombieEntity::registerAttributes()
{
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 僵尸属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, 2.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 35.0);

    // 注册僵尸增援概率属性
    auto reinforcementAttr = entity::attribute::Attributes::zombieSpawnReinforcements();
    m_attributes.registerAttribute(*reinforcementAttr);
}

void ZombieEntity::convertToDrowned()
{
    // 转化为溺尸
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // 1. 从注册表获取溺尸实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* drownedType = registry.getType("minecraft:drowned");

    // 2. 创建溺尸实体
    std::unique_ptr<Entity> newEntity;
    if (drownedType && drownedType->canSummon()) {
        newEntity = drownedType->create(worldPtr);
    } else {
        // 回退：直接创建实体类
        newEntity = std::make_unique<DrownedEntity>(EntityId(0));
    }

    DrownedEntity* drowned = dynamic_cast<DrownedEntity*>(newEntity.get());
    if (drowned == nullptr) {
        return;
    }

    // 3. 复制位置和旋转
    drowned->setPosition(m_position);
    drowned->setRotation(m_yaw, m_pitch);

    // 4. 复制生命值（按比例）
    f32 healthRatio = health() / maxHealth();
    drowned->setHealth(drowned->maxHealth() * healthRatio);

    // 5. 复制装备（所有槽位）
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        EquipmentSlot slot = static_cast<EquipmentSlot>(i);
        const ItemStack& equipment = getEquipment(slot);
        if (!equipment.isEmpty()) {
            drowned->setEquipment(slot, equipment);
        }
    }

    // 6. 复制婴儿状态
    drowned->setBaby(m_isBaby);

    // 7. 复制自定义名称
    if (hasCustomName()) {
        drowned->setCustomName(customNameText());
        drowned->setCustomNameVisible(isCustomNameVisible());
    }

    // 8. 复制持久化状态
    if (isNoDespawnRequired()) {
        drowned->enablePersistence();
    }

    // 9. 释放所有权并生成到世界
    newEntity.release();
    EntityId newId = worldPtr->spawnEntity(std::unique_ptr<Entity>(drowned));

    if (newId == 0) {
        // 生成失败，删除实体
        delete drowned;
        return;
    }

    // 10. 清空原实体装备（防止死亡时掉落）
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        setEquipment(static_cast<EquipmentSlot>(i), ItemStack());
    }

    // 11. 播放转化音效
    // 事件 1040 是僵尸转化为溺尸的视觉/音效事件
    playSound(SoundEvents::ENTITY_ZOMBIE_CONVERTED_TO_DROWNED, 1.0f, 1.0f);
    worldPtr->playEvent(1040,
        BlockPos(static_cast<i32>(m_position.x), static_cast<i32>(m_position.y), static_cast<i32>(m_position.z)),
        0);

    // 12. 重置转化状态
    m_converting = false;
    m_conversionTime = 0;
    m_inWaterTime = 0;

    // 13. 移除原实体
    remove();
}

void ZombieEntity::_updateDrowning()
{
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 检查是否在水中
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

// ============================================================================
// NBT 序列化
// ============================================================================

void ZombieEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 先调用基类实现
    MonsterEntity::addAdditionalSaveData(tag);

    // IsBaby (byte/bool) - 是否是婴儿僵尸
    tag.put(nbt_keys::IS_BABY, static_cast<i8>(m_isBaby ? 1 : 0));

    // DrownedConversionTime (i32) - 溺水转化时间
    // 只有正在转化时才保存（>= 0 表示正在转化）
    if (m_converting && m_conversionTime >= 0) {
        tag.put(nbt_keys::DROWNED_CONVERSION_TIME, m_conversionTime);
    }

    // InWaterTime (i32) - 在水中的时间
    // 只有当与溺水转化相关时才保存
    if (m_inWaterTime > 0) {
        tag.put(nbt_keys::IN_WATER_TIME, m_inWaterTime);
    }

    // CanBreakDoors (byte/bool) - 是否可以破门
    if (m_canBreakDoors) {
        tag.put(nbt_keys::CAN_BREAK_DOORS, static_cast<i8>(1));
    }
}

Result<void> ZombieEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 先调用基类实现
    MC_TRY(MonsterEntity::readAdditionalSaveData(tag));

    // IsBaby (byte/bool) - 是否是婴儿僵尸
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::IS_BABY)) {
        setBaby(*val);
    }

    // DrownedConversionTime (i32) - 溺水转化时间
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::DROWNED_CONVERSION_TIME)) {
        if (*val >= 0) {
            m_conversionTime = *val;
            m_converting = true;
        }
    }

    // InWaterTime (i32) - 在水中的时间
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::IN_WATER_TIME)) {
        m_inWaterTime = *val;
    }

    // CanBreakDoors (byte/bool) - 是否可以破门
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::CAN_BREAK_DOORS)) {
        setBreakDoorsAbility(*val);
    }

    return Result<void>::ok();
}

// ============================================================================
// 生成初始化
// ============================================================================

void ZombieEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    // 调用父类 finalizeSpawn，处理拾取物品能力、默认装备和附魔
    MonsterEntity::finalizeSpawn(world, difficulty, spawnReason);

    f32 specialMultiplier = difficulty.getSpecialMultiplier();

    // 设置破门能力：概率 = specialMultiplier * 0.1
    math::Random rng = getRandom();
    if (rng.nextFloat() < specialMultiplier * 0.1f) {
        setBreakDoorsAbility(true);
    }

    // 僵尸特有的装备已在 populateDefaultEquipmentSlots 覆写中处理

    // 万圣节南瓜头：10月31日，25% 概率
    if (util::SpecialDates::isHalloween() && rng.nextFloat() < 0.25f) {
        if (getEquipment(EquipmentSlot::Head).isEmpty()) {
            const Item* pumpkinItem = rng.nextFloat() < 0.1f ? Items::JACK_O_LANTERN : Items::CARVED_PUMPKIN;
            if (pumpkinItem != nullptr) {
                setEquipment(EquipmentSlot::Head, ItemStack(*pumpkinItem, 1));
                setEquipmentDropChance(EquipmentSlot::Head, 0.0f);
            }
        }
    }

    // 处理属性修饰符（随机增援概率、击退抗性、跟随范围、领袖僵尸判定）
    _handleAttributes(rng, specialMultiplier);
}

void ZombieEntity::_handleAttributes(math::Random& rng, f32 specialMultiplier)
{

    // 随机设置增援概率基础值（0.0 ~ 0.1）
    m_attributes.setBaseValue(entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, rng.nextDouble() * 0.1);

    // 击退抗性添加随机生成加成
    entity::attribute::AttributeModifier knockbackModifier(
        RANDOM_SPAWN_BONUS_ID, "Random spawn bonus", rng.nextDouble() * 0.05, entity::attribute::Operation::Addition);
    m_attributes.addModifier(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, knockbackModifier);

    // 跟随范围添加随机生成加成（条件性）
    f64 followRangeBonus = rng.nextDouble() * 1.5 * static_cast<f64>(specialMultiplier);
    if (followRangeBonus > 1.0) {
        entity::attribute::AttributeModifier followModifier(ZOMBIE_RANDOM_SPAWN_BONUS_ID,
            "Zombie random spawn bonus",
            followRangeBonus,
            entity::attribute::Operation::MultiplyTotal);
        m_attributes.addModifier(entity::attribute::Attributes::FOLLOW_RANGE, followModifier);
    }

    // 领袖僵尸判定（概率 = specialMultiplier * 0.05）
    if (rng.nextFloat() < specialMultiplier * 0.05f) {
        // 领袖僵尸增援概率增加
        entity::attribute::AttributeModifier leaderReinforcementModifier(LEADER_ZOMBIE_BONUS_ID,
            "Leader zombie bonus",
            rng.nextDouble() * 0.25 + 0.5,
            entity::attribute::Operation::Addition);
        m_attributes.addModifier(
            entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, leaderReinforcementModifier);

        // 领袖僵尸最大生命值增加
        entity::attribute::AttributeModifier leaderHealthModifier(LEADER_ZOMBIE_BONUS_ID,
            "Leader zombie bonus",
            rng.nextDouble() * 3.0 + 1.0,
            entity::attribute::Operation::MultiplyTotal);
        m_attributes.addModifier(entity::attribute::Attributes::MAX_HEALTH, leaderHealthModifier);

        // 领袖僵尸可以破门
        setBreakDoorsAbility(true);
    }
}

void ZombieEntity::populateDefaultEquipmentSlots(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty)
{
    // 先调用父类方法：基于难度填充护甲
    MonsterEntity::populateDefaultEquipmentSlots(random, difficulty);

    // 僵尸特有：主手武器
    // Hard 难度: 5% 概率，其他难度: 1% 概率
    f32 weaponChance = (difficulty.getDifficulty() == Difficulty::Hard) ? 0.05f : 0.01f;
    if (random.nextFloat() < weaponChance) {
        i32 weaponType = random.nextInt(6);
        const Item* weapon = nullptr;
        if (weaponType == 0) {
            weapon = Items::IRON_SWORD;
        } else {
            weapon = Items::IRON_SHOVEL;
        }
        if (weapon != nullptr) {
            setEquipment(EquipmentSlot::MainHand, ItemStack(*weapon, 1));
        }
    }
}

} // namespace mc
