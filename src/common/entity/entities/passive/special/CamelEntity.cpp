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

#include "CamelEntity.hpp"

#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/CamelSitGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>

namespace mc {

entity::DataParameter<bool> CamelEntity::DASH_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<i64> CamelEntity::LAST_POSE_CHANGE_TICK_PARAM = entity::EntityDataManager::createKey<i64>();

const entity::EntityClassInfo& CamelEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"CamelEntity", &AbstractHorseEntity::classInfo()};
    return s_classInfo;
}

void CamelEntity::registerData()
{
    AbstractHorseEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 对齐 vanilla Camel.defineSynchedData：DASH(Boolean,false) + LAST_POSE_CHANGE_TICK(Long,0)。
    m_dataManager.registerParam(DASH_PARAM, false);
    m_dataManager.registerParam(LAST_POSE_CHANGE_TICK_PARAM, static_cast<i64>(0));
}

CamelEntity::CamelEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractHorseEntity(id, registry)
{
    registerData();
    registerAttributes();
    registerGoals();
}

std::unique_ptr<Entity> CamelEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<CamelEntity>(0, registry);
}

void CamelEntity::registerAttributes()
{
    // 对齐 vanilla Camel.createAttributes：createBaseHorseAttributes + 生命 32 / 移速 0.09 /
    // 跳跃力 0.42 / 步高 1.5。
    AbstractHorseEntity::registerAttributes();
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 32.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.09);
    attributes().setBaseValue("generic.jump_strength", 0.42);
    attributes().setBaseValue("generic.step_height", 1.5);
    setHealth(maxHealth());
}

void CamelEntity::registerGoals()
{
    // vanilla Camel.registerGoals 为空（全部由 Brain 驱动）；Cubium 以 GoalSelector 等价实现。
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));
    auto panicGoal = std::make_unique<entity::ai::goal::PanicGoal>(this, 4.0);
    m_panicGoal = panicGoal.get();
    m_goalSelector.addGoal(1, std::move(panicGoal));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this, 2.5, [](const ItemStack& stack) { return stack.getItem() == Items::CACTUS; }, false));
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 2.5));
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 2.0));
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));
    // 对齐 vanilla CamelAi.RandomSitting：空闲 20 秒后才可能随机坐下。
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::CamelSitGoal>(this));
}

// ============================================================================
// 坐姿
// ============================================================================

i64 CamelEntity::lastPoseChangeTick() const
{
    return m_dataManager.get(LAST_POSE_CHANGE_TICK_PARAM);
}

void CamelEntity::resetLastPoseChangeTick(i64 tick)
{
    m_dataManager.set(LAST_POSE_CHANGE_TICK_PARAM, tick);
}

bool CamelEntity::isCamelSitting() const
{
    return lastPoseChangeTick() < 0;
}

i64 CamelEntity::getPoseTime() const
{
    if (m_world == nullptr) {
        return 0;
    }
    const i64 gameTime = static_cast<i64>(m_world->getGameTime());
    const i64 changeTick = lastPoseChangeTick();
    // 坐姿时存 -(now + 1)，取绝对值再减 1 得"姿态变化发生的游戏时间"；站姿时直接存该时间。
    return gameTime - (changeTick < 0 ? -changeTick - 1 : changeTick);
}

bool CamelEntity::isInPoseTransition() const
{
    const i64 transitionTicks = isCamelSitting() ? SITDOWN_DURATION_TICKS : STANDUP_DURATION_TICKS;
    return getPoseTime() < transitionTicks;
}

bool CamelEntity::hasControllingPassenger() const
{
    return getControllingPassenger() != INVALID_ENTITY_ID;
}

bool CamelEntity::isPanicking() const
{
    return m_panicGoal != nullptr && m_panicGoal->isRunning();
}

void CamelEntity::sitDown()
{
    if (isCamelSitting()) {
        return;
    }
    _playSound("sit");
    setPose(EntityPose::Sitting);
    // 坐姿时间戳存为 -(now + 1)；+1 对齐 vanilla 的 gameTime 从 1 起算，避免 now=0 时取负仍为 0
    // （非负 = 站立）。getPoseTime() 中 +1 还原。
    resetLastPoseChangeTick(-(static_cast<i64>(m_world != nullptr ? m_world->getGameTime() : 0) + 1));
}

void CamelEntity::standUp()
{
    if (!isCamelSitting()) {
        return;
    }
    _playSound("stand");
    setPose(EntityPose::Standing);
    resetLastPoseChangeTick(static_cast<i64>(m_world != nullptr ? m_world->getGameTime() : 0));
}

void CamelEntity::standUpInstantly()
{
    setPose(EntityPose::Standing);
    // vanilla resetLastPoseChangeTickToFullStand 取 max(0, now - 52 - 1)，以"已站起一段时间"跳过过渡。
    // Cubium 的 gameTime 可能小于 53（含 0），此时 vanilla 的夹取会得到 0 而仍处于过渡窗口；
    // 这里改取 (STANDUP_DURATION_TICKS + 1) - now 的正值，使 |tick| - now >= 52 恒成立：
    // 既非负（=站姿），也保证 getPoseTime() 已越过站起动画。
    const i64 now = static_cast<i64>(m_world != nullptr ? m_world->getGameTime() : 0);
    resetLastPoseChangeTick(STANDUP_DURATION_TICKS + 1 - now);
}

// ============================================================================
// 冲刺
// ============================================================================

bool CamelEntity::isDashing() const
{
    return m_dataManager.get(DASH_PARAM);
}

void CamelEntity::setDashing(bool dashing)
{
    // 对齐 vanilla onSyncedDataUpdated：首次进入冲刺时重置冷却为 55。
    if (dashing && !isDashing() && m_dashCooldown == 0) {
        m_dashCooldown = DASH_COOLDOWN_TICKS;
    }
    m_dataManager.set(DASH_PARAM, dashing);
}

void CamelEntity::executeDash(f32 jumpScale, Player& rider)
{
    // 对齐 vanilla executeRidersJump：
    //   水平 = normalize(lookAngle.xz) * 22.2222 * scale * MOVEMENT_SPEED * blockSpeedFactor
    //   垂直 = 1.4285 * scale * jumpPower
    const f32 yawRad = static_cast<f32>(math::toRadians(rider.yaw()));
    Vector3 lookXZ(-std::sin(yawRad), 0.0, std::cos(yawRad));
    if (lookXZ.length() > 1.0e-6) {
        lookXZ = lookXZ.normalized();
    }
    const f64 movementSpeed = attributes().getValue(entity::attribute::Attributes::MOVEMENT_SPEED);
    const f32 jumpPower = getJumpPower();

    const Vector3 dash(lookXZ.x * DASH_HORIZONTAL_MOMENTUM * jumpScale * movementSpeed,
        DASH_VERTICAL_MOMENTUM * jumpScale * jumpPower,
        lookXZ.z * DASH_HORIZONTAL_MOMENTUM * jumpScale * movementSpeed);
    addVelocity(dash.x, dash.y, dash.z);

    m_dashCooldown = DASH_COOLDOWN_TICKS;
    setDashing(true);
}

f32 CamelEntity::getRiddenSpeed(Player& rider) const
{
    // 对齐 vanilla getRiddenSpeed：MOVEMENT_SPEED +（玩家冲刺且冲刺冷却就绪时的 0.1 加成）。
    const f32 movementSpeed = static_cast<f32>(attributes().getValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    const f32 bonus = (rider.isSprinting() && m_dashCooldown == 0) ? RUNNING_SPEED_BONUS : 0.0f;
    return movementSpeed + bonus;
}

// ============================================================================
// 交互
// ============================================================================

ActionResultType CamelEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 vanilla Camel.mobInteract：
    //   Shift + 右键（非幼年）→ 打开坐骑背包；否则交给基类（装备鞍/食物/骑乘）。
    if (player.isSneaking() && !isChild()) {
        openInventory(player);
        return ActionResultType::Success;
    }
    return AbstractHorseEntity::interactMob(player, hand);
}

void CamelEntity::actuallyHurt(DamageSource& source, f32 amount)
{
    // 对齐 vanilla Camel.actuallyHurt：先立刻站起，再结算伤害。
    standUpInstantly();
    AbstractHorseEntity::actuallyHurt(source, amount);
}

// ============================================================================
// 声音
// ============================================================================

void CamelEntity::_playSound(const char* suffix, f32 pitch)
{
    const std::optional<ResourceLocation> sound = makeSoundEventId(suffix);
    if (sound.has_value()) {
        playSound(*sound, getSoundVolume(), pitch);
    }
}

std::optional<ResourceLocation> CamelEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> CamelEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> CamelEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> CamelEntity::getEatSound() const
{
    return makeSoundEventId("eat");
}

// ============================================================================
// 繁殖 / 食物
// ============================================================================

bool CamelEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 对齐 vanilla Camel.isFood：ItemTags.CAMEL_FOOD（仙人掌）。
    return itemStack.getItem() == Items::CACTUS && !itemStack.isEmpty();
}

std::unique_ptr<AnimalEntity> CamelEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }
    auto baby = std::make_unique<CamelEntity>(0, *registry);
    baby->setChild(true);
    baby->setPosition(x(), y(), z());
    return baby;
}

void CamelEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    // 对齐 vanilla finalizeSpawn：以"已完成站起动画"的姿态时间戳初始化。
    standUpInstantly();
    AbstractHorseEntity::finalizeSpawn(world, difficulty, spawnReason);
}

// ============================================================================
// 生命周期
// ============================================================================

void CamelEntity::tick()
{
    AbstractHorseEntity::tick();

    // 冲刺结束判定（对齐 vanilla：冷却 < 50 且落地/入水/乘载具时结束冲刺）。
    if (isDashing() && m_dashCooldown < DASH_END_COOLDOWN_THRESHOLD &&
        (onGround() || isInWater() || getVehicle() != INVALID_ENTITY_ID)) {
        setDashing(false);
    }

    if (m_dashCooldown > 0) {
        --m_dashCooldown;
        if (m_dashCooldown == 0) {
            _playSound("dash_ready");
        }
    }

    // 坐下时落入水中立刻站起（对齐 vanilla tick 的 isCamelSitting && isInWater）。
    if (isCamelSitting() && isInWater()) {
        standUpInstantly();
    }
}

void CamelEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    AbstractHorseEntity::addAdditionalSaveData(tag);
    tag.put(entity::serialization::nbt_keys::LAST_POSE_TICK, lastPoseChangeTick());
}

Result<void> CamelEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace entity::serialization;
    MC_TRY(AbstractHorseEntity::readAdditionalSaveData(tag));
    i64 poseTick = nbt_helper::tryGetLong(tag, nbt_keys::LAST_POSE_TICK).value_or(0);
    if (poseTick < 0) {
        setPose(EntityPose::Sitting);
    }
    resetLastPoseChangeTick(poseTick);
    return Result<void>::ok();
}

} // namespace mc