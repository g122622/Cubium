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

#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/interact/LandOnOwnersShoulderGoal.hpp"
#include "../../../ai/goal/goals/interact/TameableGoals.hpp"
#include "../../../ai/goal/goals/movement/FollowMobGoal.hpp"
#include "../../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../../ai/goal/goals/movement/WaterAvoidingRandomFlyingGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../entities/player/Player.hpp"
#include "common/network/protocol/EntityEvents.hpp"

namespace mc {

ParrotEntity::ParrotEntity(EntityInstanceId id)
    : ShoulderRidingEntity(id)
{
    randomizeVariant();
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> ParrotEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ParrotEntity>(0);
}

void ParrotEntity::randomizeVariant()
{
    math::Random& rng = getRandom();
    m_variant = static_cast<ParrotVariant>(rng.nextInt(0, 4));
}

bool ParrotEntity::isTameItem(const ItemStack& itemStack) const
{
    // 鹦鹉用种子驯服
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
        math::Random& rng = getRandom();
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

    // 优先级 0: 游泳和恐慌逃跑（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::PanicGoal>(this, 1.25));

    // 优先级 1: 看向玩家
    m_goalSelector.addGoal(1,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 2: 坐下、跟随主人、随机飞行
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::SitGoal>(this));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::FollowOwnerGoal>(this, 1.0, 5.0f, 10.0f, 32.0f));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::WaterAvoidingRandomFlyingGoal>(this, 1.0));

    // 优先级 3: 落到主人肩膀、跟随其他生物
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::LandOnOwnersShoulderGoal>(this));
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::FollowMobGoal>(this, 1.0, 3.0f, 7.0f));
}

void ParrotEntity::registerAttributes()
{
    ShoulderRidingEntity::registerAttributes();

    // 注册飞行速度属性（LivingEntity 基类不注册此属性）
    m_attributes.registerAttribute(*entity::attribute::Attributes::flyingSpeed());

    // 鹦鹉属性：生命值6，移动速度0.2，飞行速度0.4
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.4);
}

void ParrotEntity::onTamed(bool tamed)
{
    // 驯服状态改变时触发。
    // interactMob 喂食路径已在调用 setTamed 前播放 ENTITY_PARROT_EAT，此处不再重复播放，
    // 否则 1/10 驯服成功时会产生两次 eat 音效（与 MC 行为不符且导致测试 flaky）。
    // 驯服成功粒子通过 network::EntityStatus::TamingSucceeded 广播，由客户端生成心形粒子。
    MC_UNUSED(tamed);
}

ActionResultType ParrotEntity::interactMob(Player& player, Hand hand)
{
    ItemStack& itemStack = player.getHeldItem(hand);
    const Item* item = itemStack.getItem();

    // 检查是否用种子驯服
    if (!isTamed() && isTameItem(itemStack)) {
        // 消耗物品（非创造模式）
        if (!player.abilities().creativeMode) {
            itemStack.shrink(1);
        }

        // 播放吃东西声音
        if (!isSilent()) {
            playSound(SoundEvents::ENTITY_PARROT_EAT,
                1.0f,
                1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f);
        }

        // 服务端处理驯服逻辑
        if (m_world != nullptr && !m_world->isClientSide()) {
            // 驯服概率 1/10 (10%)
            math::Random& rng = getRandom();
            if (rng.nextInt(10) == 0) {
                // 驯服成功
                setTamed(true);
                setOwnerId(player.playerId());

                // 通知世界动物被驯服，触发进度检测
                m_world->onTameAnimal(player.playerId(), this);

                // 广播驯服成功状态（心形粒子）
                m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::TamingSucceeded));
            } else {
                // 驯服失败，广播烟雾粒子
                m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::TamingFailed));
            }
        }

        return ActionResultType::Success;
    }

    // 已驯服的鹦鹉可以切换坐下状态
    if (isTamed() && isOwner(player.playerId())) {
        // 切换坐下状态
        toggleSitting();
        return ActionResultType::Success;
    }

    // 调用父类处理
    return ShoulderRidingEntity::interactMob(player, hand);
}

} // namespace mc
