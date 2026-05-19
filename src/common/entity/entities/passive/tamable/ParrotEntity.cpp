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
#include "../../../../network/packet/EntityPackets.hpp"
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

namespace mc {

ParrotEntity::ParrotEntity(EntityId id)
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
    m_goalSelector.addGoal(1,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 2: 坐下、跟随主人、随机飞行
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::SitGoal>(this));
    // FollowOwnerGoal: speed=1.0, minDistance=5.0, maxDistance=1.0, canTeleportToLeaves=true
    // 注意：MC原版的 maxDistance 参数是 1.0F，表示距离主人很近时停止跟随
    // 这里的参数含义与项目中的 FollowOwnerGoal 略有不同，需要适配
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

    // MC 1.16.5: 鹦鹉属性
    // MAX_HEALTH = 6.0
    // MOVEMENT_SPEED = 0.2
    // FLYING_SPEED = 0.4
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.4);
}

void ParrotEntity::onTamed(bool tamed)
{
    // MC 1.16.5: ParrotEntity 没有重写 setTamed 或 setTamedBy
    // onTamed 回调在驯服状态改变时触发
    // 驯服成功后粒子效果通过 EntityStatusPacket::TamingSucceeded 广播
    // 由客户端 ClientApplicationNetwork 处理并生成心形粒子

    if (tamed) {
        // 驯服成功后播放吃东西声音
        // MC 1.16.5: this.world.playSound((PlayerEntity)null, this.getPosX(), this.getPosY(), this.getPosZ(),
        //             SoundEvents.ENTITY_PARROT_EAT, this.getSoundCategory(), 1.0F, 1.0F);
        playSound(SoundEvents::ENTITY_PARROT_EAT, 1.0f, 1.0f);
    }
}

ActionResultType ParrotEntity::interactMob(Player& player, Hand hand)
{
    // MC 1.16.5: ParrotEntity.func_230254_b_()
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
            // MC 1.16.5: 驯服概率 1/10 (10%)
            math::Random rng = getRandom();
            if (rng.nextInt(10) == 0) {
                // 驯服成功
                setTamed(true);
                setOwnerId(player.playerId());

                // 广播驯服成功状态（心形粒子）
                m_world->broadcastEntityStatus(
                    id(), static_cast<u8>(network::EntityStatusPacket::Status::TamingSucceeded));
            } else {
                // 驯服失败，广播烟雾粒子
                m_world->broadcastEntityStatus(
                    id(), static_cast<u8>(network::EntityStatusPacket::Status::TamingFailed));
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
