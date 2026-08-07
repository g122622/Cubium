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
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/illager/AbstractIllagerEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <memory>
#include <optional>

namespace mc {

// ============================================================================
// TraderLlamaEntity
// ============================================================================

TraderLlamaEntity::TraderLlamaEntity(EntityInstanceId id)
    : LlamaEntity(id)
{
    // 补调 registerGoals / registerAttributes：AnimalEntity 构造只调基类版（vtable 指向 AnimalEntity），
    // 派生 override 永不执行，须在派生类构造显式调用。TraderLlama 的 registerGoals 加专属
    // PanicGoal / DefendWanderingTrader 目标。详见 AbstractHorseEntity 构造注释。
    registerGoals();
    registerAttributes();
}

std::optional<ResourceLocation> TraderLlamaEntity::getAmbientSound() const
{
    // 商队羊驼复用普通羊驼的环境音，对齐原版 TraderLlama（继承 Llama.getAmbientSound 返回 LLAMA_AMBIENT）。
    // sounds.json 中无 entity.trader_llama.*（商队羊驼共享 llama.* 音效），默认拼接会生成不存在的
    // trader_llama.ambient。
    return SoundEvents::ENTITY_LLAMA_AMBIENT;
}

std::unique_ptr<Entity> TraderLlamaEntity::create(IWorld* /*world*/)
{
    return std::make_unique<TraderLlamaEntity>(EntityInstanceId(0));
}

bool TraderLlamaEntity::canDespawn(double /*distanceToClosestPlayer*/) const noexcept
{
    // 商队羊驼在以下情况下不会消失：
    // 1. 已被驯服
    // 2. 被拴住（任何拴绳持有者，包括流浪商人）
    // 3. 正好有一名玩家乘客
    // 注意：被拴住时不应被 DespawnManager 距离判断移除，
    // 流浪商人自身的消失机制通过 maybeDespawn() 管理。
    return !isTame() && !isLeashed() && !hasExactlyOnePlayerPassenger();
}

ActionResultType TraderLlamaEntity::interactMob(Player& player, Hand hand)
{
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

void TraderLlamaEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    LlamaEntity::finalizeSpawn(world, difficulty, spawnReason);

    // 确保消失倒计时已设置（自然生成时使用默认值）
    // 当由流浪商人的 spawnLlamas() 生成时，消失倒计时会在生成后通过
    // setDespawnDelay() 显式设置，因此这里只需确保非商人路径的默认值
    if (m_despawnDelay <= 0) {
        m_despawnDelay = DEFAULT_DESPAWN_DELAY;
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
                // 排除僵尸猪灵
                return entity != nullptr && entity->entityType() != entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN;
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
    // 对应 MC 1.21.11 TraderLlama.maybeDespawn() 中的私有 canDespawn() 判定
    // （注意：此处不是 MobEntity::canDespawn(double)，后者供 DespawnManager 距离判断使用，
    //  对任何拴绳状态均返回 false 以避免被距离判断误删）。
    // MC 私有 canDespawn() 语义：
    //   !isTamed && !isLeashedToSomethingOtherThanTheWanderingTrader && !hasExactlyOnePlayerPassenger
    // 即"拴在流浪商人身上"仍允许消失（倒计时与流浪商人同步），
    // 仅"拴在其他实体/栅栏上"才阻止消失。
    const bool leashedToOther = isLeashed() && !isLeashedToWanderingTrader();
    if (isTame() || leashedToOther || hasExactlyOnePlayerPassenger()) {
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

    // 使用 IWorld::getEntityByUuid() 进行 O(1) UUID 查找
    return m_world->getEntityByUuid(*leashHolderUuid());
}

} // namespace mc
