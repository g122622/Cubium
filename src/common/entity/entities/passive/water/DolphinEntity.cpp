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

#include "DolphinEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/math/MathConstants.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../ai/pathfinding/Path.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/GoalFlag.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/FindWaterGoal.hpp"
#include "../../../ai/goal/goals/RandomSwimmingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../ai/goal/goals/special/DolphinGoals.hpp"
#include "../../../ai/pathfinding/PathNavigator.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include <cmath>

namespace mc {

DolphinEntity::DolphinEntity(LegacyEntityType type, EntityId id)
    : WaterMobEntity(type, id)
{
    // 设置空气值（MC 1.16.5: 4800 tick = 4分钟）
    setAir(MAX_AIR);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> DolphinEntity::create(IWorld* /*world*/)
{
    return std::make_unique<DolphinEntity>(LegacyEntityType::Unknown, 0);
}

bool DolphinEntity::canJumpOutOfWater() const
{
    // MC 1.16.5: 海豚可以跳出水面当且仅当:
    // 1. 当前在水中
    // 2. 上方有空气（接近水面）

    if (!isInWater()) {
        return false;
    }

    // 检查上方是否有空气
    const Vector3 pos = position();
    const BlockPos headPos(static_cast<i32>(std::floor(pos.x)),
        static_cast<i32>(std::floor(pos.y)) + 1,
        static_cast<i32>(std::floor(pos.z)));
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return false;
    }

    const BlockState* stateAbove = worldPtr->getBlockState(headPos);
    return stateAbove != nullptr && stateAbove->isAir();
}

void DolphinEntity::setTreasurePos(const BlockPos& pos)
{
    m_treasurePos = pos;
    m_hasTreasure = true;
}

void DolphinEntity::clearTreasureTarget()
{
    m_hasTreasure = false;
    m_guidingPlayer = false;
    m_guidedPlayerId = 0;
    m_guideTimer = 0;
}

void DolphinEntity::setGuidingPlayer(bool guiding, u64 playerId)
{
    m_guidingPlayer = guiding;
    m_guidedPlayerId = playerId;
    if (guiding) {
        m_guideTimer = GUIDE_DURATION;
    }
}

bool DolphinEntity::isFoodItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: 海豚食物 - 鳕鱼、鲑鱼、河豚、热带鱼
    const Item* item = itemStack.getItem();
    return item == Items::COD || item == Items::SALMON || item == Items::PUFFERFISH || item == Items::TROPICAL_FISH;
}

bool DolphinEntity::closeToTarget() const
{
    // MC 1.16.5: closeToTarget() - 检查是否接近导航目标
    auto* nav = navigator();
    if (nav == nullptr) {
        return false;
    }

    const entity::ai::pathfinding::Path* path = nav->getPath();
    if (path == nullptr || path->isFinished()) {
        return false;
    }

    BlockPos targetPos = path->getTarget();
    if (targetPos.x == 0 && targetPos.y == 0 && targetPos.z == 0) {
        return false;
    }

    // 检查是否在 12 格范围内
    Vector3 targetCenter(
        static_cast<f64>(targetPos.x) + 0.5,
        static_cast<f64>(targetPos.y),
        static_cast<f64>(targetPos.z) + 0.5
    );

    f64 distSq = (position() - targetCenter).lengthSquared();
    constexpr f64 CLOSE_DISTANCE_SQ = 12.0 * 12.0;

    return distSq < CLOSE_DISTANCE_SQ;
}

bool DolphinEntity::hasPath() const
{
    auto* nav = navigator();
    return nav != nullptr && nav->hasPath();
}

void DolphinEntity::clearNavigationPath()
{
    auto* nav = navigator();
    if (nav != nullptr) {
        nav->clearPath();
    }
}

bool DolphinEntity::tryMoveToEntity(const Entity& entity, f64 speed)
{
    // 使用 CreatureEntity::tryMoveTo
    return tryMoveTo(entity.x(), entity.y(), entity.z(), speed);
}

void DolphinEntity::onLeaveWater()
{
    WaterMobEntity::onLeaveWater();
    playSound(SoundEvents::ENTITY_DOLPHIN_JUMP, 1.0f, 1.0f);
}

std::optional<ResourceLocation> DolphinEntity::getAmbientSound() const
{
    if (isInWater()) {
        return SoundEvents::ENTITY_DOLPHIN_AMBIENT_WATER;
    }
    return SoundEvents::ENTITY_DOLPHIN_AMBIENT;
}

void DolphinEntity::playAttackSound(LivingEntity& /*target*/)
{
    playSound(SoundEvents::ENTITY_DOLPHIN_ATTACK, 1.0f, 1.0f);
}

void DolphinEntity::tick()
{
    WaterMobEntity::tick();

    // 更新引导计时器
    if (m_guidingPlayer && m_guideTimer > 0) {
        m_guideTimer--;
        if (m_guideTimer <= 0) {
            clearTreasureTarget();
        }
    }

    // 更新游泳行为
    if (isInWater()) {
        m_swimTimer++;

        // 随机跳跃
        if (m_swimTimer >= 200 && canJumpOutOfWater()) {
            math::Random rng = getRandom();
            if (rng.nextInt(1, 100) == 1) {
                m_jumping = true;
                m_swimTimer = 0;
            }
        }
    } else {
        m_jumping = false;
    }
}

void DolphinEntity::registerGoals()
{
    // MC 1.16.5: 海豚 AI 目标优先级
    // 参考: net.minecraft.entity.passive.DolphinEntity.registerGoals()

    // 优先级 0: 呼吸空气和寻找水源
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::FindWaterGoal>(this));

    // 优先级 1: 游向宝藏
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::SwimToTreasureGoal>(this));

    // 优先级 2: 与玩家同游
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::SwimWithPlayerGoal>(this, 4.0));

    // 优先级 4: 随机游泳和随机看向
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::RandomSwimmingGoal>(this, 1.0, 10));
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 优先级 5: 看向玩家和跳跃
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f, 0.02f,
        [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->legacyType() == LegacyEntityType::Player;
        }));
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::DolphinJumpGoal>(this, 10));

    // 优先级 6: 近战攻击
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.2, true));

    // 优先级 8: 玩物品和跟随船
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::PlayWithItemsGoal>(this));
    // TODO: FollowBoatGoal - 当船实体实现后添加

    // 优先级 9: 避开守卫者
    // m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::AvoidEntityGoal>(this, GuardianEntity::class, 8.0f, 1.0, 1.0));

    // 目标选择器
    // 优先级 1: 被攻击后反击，并呼叫同类
    // m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this).setCallsForHelp());
}

void DolphinEntity::registerAttributes()
{
    // 调用父类方法
    WaterMobEntity::registerAttributes();

    // MC 1.16.5: 海豚属性
    // 最大生命值: 10.0
    // 移动速度: 1.2 (MC 中使用 1.2F 作为基础移动速度乘数)
    // 攻击伤害: 3.0
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 1.2);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0);
}

} // namespace mc
