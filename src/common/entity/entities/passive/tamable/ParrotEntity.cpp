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

#include "ParrotEntity.hpp"

#include "../../../../item/Items.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/interact/TameableGoals.hpp"
#include "../../../ai/goal/goals/interact/LandOnOwnersShoulderGoal.hpp"
#include "../../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../../ai/goal/goals/movement/WaterAvoidingRandomFlyingGoal.hpp"
#include "../../../ai/goal/goals/movement/FollowMobGoal.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../entities/player/Player.hpp"

namespace mc {

ParrotEntity::ParrotEntity(LegacyEntityType type, EntityId id)
    : ShoulderRidingEntity(type, id)
{
    randomizeVariant();
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> ParrotEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ParrotEntity>(LegacyEntityType::Unknown, 0);
}

void ParrotEntity::randomizeVariant()
{
    math::Random rng = getRandom();
    m_variant = static_cast<ParrotVariant>(rng.nextInt(0, 4));
}

bool ParrotEntity::isTameItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: 鹦鹉用种子驯服
    // 参考: net.minecraft.entity.passive.ParrotEntity.TAME_ITEMS
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::WHEAT_SEEDS || item == Items::PUMPKIN_SEEDS || item == Items::MELON_SEEDS ||
        item == Items::BEETROOT_SEEDS;
}

void ParrotEntity::tick()
{
    ShoulderRidingEntity::tick();

    if (isOnShoulder()) {
        return;
    }

    if (m_imitating) {
        --m_imitateTimer;
        if (m_imitateTimer <= 0) {
            m_imitating = false;
            m_imitatingTarget = 0;
        }
    }

    if (!m_imitating && isTamed()) {
        math::Random rng = getRandom();
        if (rng.nextInt(1, 100) == 1) {
            m_imitateTimer = 60;
        }
    }

    if (m_flying) {
        m_flapSpeed = FLAP_SPEED_FLYING;
        ++m_flapTimer;
    } else {
        m_flapSpeed = FLAP_SPEED_GROUND;
        m_flapTimer = 0;
    }
}

void ParrotEntity::registerGoals()
{
    ShoulderRidingEntity::registerGoals();

    // MC 1.16.5: ParrotEntity.registerGoals()
    // 优先级 0: 游泳和恐慌逃跑（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::PanicGoal>(this, 1.25));

    // 优先级 1: 看向玩家
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::LookAtGoal>(
        this, 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE,
        entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 2: 坐下、跟随主人、随机飞行
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::SitGoal>(this));
    // FollowOwnerGoal: speed=1.0, minDistance=5.0, maxDistance=1.0, canTeleportToLeaves=true
    // 注意：MC原版的 maxDistance 参数是 1.0F，表示距离主人很近时停止跟随
    // 这里的参数含义与项目中的 FollowOwnerGoal 略有不同，需要适配
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::FollowOwnerGoal>(
        this, 1.0, 5.0f, 10.0f, 32.0f));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::WaterAvoidingRandomFlyingGoal>(this, 1.0));

    // 优先级 3: 落到主人肩膀、跟随其他生物
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::LandOnOwnersShoulderGoal>(this));
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::FollowMobGoal>(
        this, 1.0, 3.0f, 7.0f));
}

void ParrotEntity::registerAttributes()
{
    ShoulderRidingEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.4);
}

void ParrotEntity::onTamed(bool tamed)
{
    if (tamed) {
        // TODO: 如后续需要，可在此接入驯服粒子或额外同步。
    }
}

} // namespace mc
