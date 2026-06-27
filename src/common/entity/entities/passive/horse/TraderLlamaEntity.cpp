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

#include "TraderLlamaEntity.hpp"

#include "common/core/Result.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/illager/AbstractIllagerEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

// ============================================================================
// TraderLlamaEntity
// ============================================================================

TraderLlamaEntity::TraderLlamaEntity(EntityId id)
    : LlamaEntity(id)
{}

std::unique_ptr<Entity> TraderLlamaEntity::create(IWorld* /*world*/)
{
    return std::make_unique<TraderLlamaEntity>(EntityId(0));
}

bool TraderLlamaEntity::canDespawn(double /*distanceToClosestPlayer*/) const noexcept
{
    // 对齐 MC Java TraderLlama.canDespawn()：
    // 商队羊驼在以下情况下不会消失：
    // 1. 已被驯服
    // 2. 被拴在流浪商人以外的实体上（如玩家或栅栏）
    // 3. 正好有一名玩家乘客
    return !isTame() && !isLeashedToSomethingOtherThanWanderingTrader() && !hasExactlyOnePlayerPassenger();
}

ActionResultType TraderLlamaEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 MC Java TraderLlama.doPlayerRide()：
    // 当商队羊驼被拴在流浪商人身上时，不允许玩家骑乘
    if (isLeashedToWanderingTrader()) {
        return ActionResultType::Pass;
    }
    return LlamaEntity::interactMob(player, hand);
}

void TraderLlamaEntity::tick()
{
    LlamaEntity::tick();

    // 仅在服务端执行消失逻辑
    if (m_world != nullptr && !m_world->isClientSide()) {
        maybeDespawn();
    }
}

void TraderLlamaEntity::registerGoals()
{
    LlamaEntity::registerGoals();

    // 优先级 1：恐慌目标（受到攻击时逃跑）
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PanicGoal>(this, 2.0));

    // 目标优先级 1：保卫流浪商人（当流浪商人被攻击时反击）
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::TraderLlamaDefendWanderingTraderGoal>(this));

    // 目标优先级 2：攻击僵尸（排除僵尸猪灵）
    m_targetSelector.addGoal(2,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<ZombieEntity>>(
            this, true, 0, [](const LivingEntity* entity) -> bool {
                // 排除僵尸猪灵（对齐 MC Java TraderLlama.registerGoals() 中的谓词）
                return entity != nullptr && entity->typeId() != entity::EntityTypeIdNumber::ZOMBIFIED_PIGLIN;
            }));

    // 目标优先级 2：攻击灾厄村民
    m_targetSelector.addGoal(
        2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<AbstractIllagerEntity>>(this, true));
}

void TraderLlamaEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    LlamaEntity::addAdditionalSaveData(tag);

    tag.put(entity::serialization::nbt_keys::DESPAWN_DELAY, m_despawnDelay);
}

Result<void> TraderLlamaEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(LlamaEntity::readAdditionalSaveData(tag));

    if (auto val = entity::serialization::nbt_helper::tryGetInt(tag, entity::serialization::nbt_keys::DESPAWN_DELAY)) {
        m_despawnDelay = *val;
    }

    return Result<void>::ok();
}

// ============================================================================
// 私有方法
// ============================================================================

void TraderLlamaEntity::maybeDespawn()
{
    // 对齐 MC Java TraderLlama.maybeDespawn()
    if (!canDespawn(0.0)) {
        return;
    }

    // 如果拴在流浪商人身上，同步流浪商人的消失倒计时（-1）
    if (isLeashedToWanderingTrader()) {
        Entity* holder = getLeashHolderEntity();
        if (holder != nullptr) {
            auto* trader = dynamic_cast<entity::WanderingTraderEntity*>(holder);
            if (trader != nullptr) {
                m_despawnDelay = trader->despawnDelay() - 1;
            }
        }
    } else {
        // 否则自行递减消失倒计时
        --m_despawnDelay;
    }

    if (m_despawnDelay <= 0) {
        // 消失前解除拴绳（不掉落拴绳物品）
        if (isLeashed()) {
            clearLeash();
        }
        discard();
    }
}

bool TraderLlamaEntity::isLeashedToWanderingTrader() const
{
    if (!isLeashed()) {
        return false;
    }
    Entity* holder = getLeashHolderEntity();
    return holder != nullptr && dynamic_cast<entity::WanderingTraderEntity*>(holder) != nullptr;
}

bool TraderLlamaEntity::isLeashedToSomethingOtherThanWanderingTrader() const
{
    if (!isLeashed()) {
        return false;
    }
    return !isLeashedToWanderingTrader();
}

bool TraderLlamaEntity::hasExactlyOnePlayerPassenger() const
{
    const auto& passengers = getPassengers();
    if (passengers.size() != 1) {
        return false;
    }
    if (m_world == nullptr) {
        return false;
    }
    Entity* passenger = m_world->getEntity(passengers[0]);
    return passenger != nullptr && dynamic_cast<Player*>(passenger) != nullptr;
}

Entity* TraderLlamaEntity::getLeashHolderEntity() const
{
    if (!isLeashed() || !leashHolderUuid().has_value()) {
        return nullptr;
    }
    if (m_world == nullptr) {
        return nullptr;
    }

    // 通过 UUID 在世界中查找拴绳持有者实体
    const std::string& uuid = *leashHolderUuid();

    // 搜索附近实体（使用较大范围）
    auto entities = m_world->getEntitiesInRange(position(), 128.0);
    for (auto* entity : entities) {
        if (entity->uuid() == uuid) {
            return entity;
        }
    }
    return nullptr;
}

} // namespace mc
