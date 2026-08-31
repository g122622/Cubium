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

#pragma once

#include "common/entity/ai/brain/Brain.hpp"
#include "common/entity/ai/brain/memory/BlockPosTarget.hpp"
#include "common/entity/ai/brain/memory/IPositionTarget.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleStatus.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/memory/WalkTarget.hpp"
#include "common/entity/ai/brain/task/Task.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/passive/tamable/TameableEntity.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/DoorBlock.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace task {
namespace interact {

using namespace memory;

/**
 * @brief 与村民互动任务
 *
 * 村民检测到 INTERACTION_TARGET 记忆后，导航到目标实体附近
 * 并与其互动。实际互动逻辑（如交易界面）由子类或外部系统处理。
 */
template <typename E>
class VillagerInteractTask : public Task<E> {
public:
    VillagerInteractTask(f32 range = 3.0f) noexcept
        : Task<E>({{MemoryModuleTypes::INTERACTION_TARGET, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::REGISTERED}},
              30,
              60)
        , m_range(range)
    {}

    std::string getName() const noexcept override { return "VillagerInteractTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto targetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::INTERACTION_TARGET);
        return targetId.has_value() && *targetId != INVALID_ENTITY_ID;
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto targetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::INTERACTION_TARGET);
        if (!targetId.has_value() || *targetId == INVALID_ENTITY_ID) {
            return false;
        }

        Entity* target = (world != nullptr) ? world->getEntity(*targetId) : nullptr;
        if (target == nullptr || !target->isAlive()) {
            return false;
        }

        f32 distSq = owner->distanceSqTo(*target);
        return distSq <= m_range * m_range * 4.0f;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto targetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::INTERACTION_TARGET);
        if (targetId.has_value() && *targetId != INVALID_ENTITY_ID && world != nullptr) {
            Entity* target = world->getEntity(*targetId);
            if (target != nullptr) {
                brain.template setMemory<WalkTarget>(
                    MemoryModuleTypes::WALK_TARGET, WalkTarget(BlockPos(target->position()), 0.5f, 2));
            }
        }
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto targetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::INTERACTION_TARGET);
        if (!targetId.has_value() || *targetId == INVALID_ENTITY_ID) {
            return;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* targetEntity = (world != nullptr) ? world->getEntity(*targetId) : nullptr;
        if (targetEntity == nullptr || !targetEntity->isAlive()) {
            return;
        }

        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *targetEntity, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        f32 distSq = owner->distanceSqTo(*targetEntity);
        f32 rangeSq = m_range * m_range;

        if (distSq <= rangeSq) {
            brain.template removeMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET);
        } else if (distSq > rangeSq * 4.0f) {
            brain.template setMemory<WalkTarget>(
                MemoryModuleTypes::WALK_TARGET, WalkTarget(BlockPos(targetEntity->position()), 0.5f, 2));
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        owner->brain().template removeMemory<EntityInstanceId>(MemoryModuleTypes::INTERACTION_TARGET);
    }

private:
    f32 m_range;
};

/**
 * @brief 与门互动任务
 *
 * 实体在沿路径行走时，检测路径上的木门并自动开关。
 * 基于 INTERACTABLE_DOORS 和 OPENED_DOORS 记忆模块。
 */
template <typename E>
class InteractWithDoorTask : public Task<E> {
public:
    InteractWithDoorTask() noexcept
        : Task<E>({{MemoryModuleTypes::INTERACTABLE_DOORS, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::OPENED_DOORS, MemoryModuleStatus::REGISTERED},
                      {MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_PRESENT}},
              20,
              40)
    {}

    std::string getName() const noexcept override { return "InteractWithDoorTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto doors = brain.template getMemory<std::vector<GlobalPos>>(MemoryModuleTypes::INTERACTABLE_DOORS);
        if (!doors.has_value() || doors->empty()) {
            return false;
        }

        auto* navigator = owner->navigator();
        if (!navigator || navigator->noPath()) {
            return false;
        }

        return true;
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!world) {
            return;
        }

        auto& brain = owner->brain();
        auto doors = brain.template getMemory<std::vector<GlobalPos>>(MemoryModuleTypes::INTERACTABLE_DOORS);
        auto openedDoors = brain.template getMemory<std::unordered_set<GlobalPos>>(MemoryModuleTypes::OPENED_DOORS);

        if (!doors.has_value() || doors->empty()) {
            return;
        }

        Vector3 ownerPos = owner->position();

        for (const auto& doorGlobalPos : *doors) {
            if (doorGlobalPos.getDimensionId() != owner->dimension()) {
                continue;
            }

            const BlockPos& doorPos = doorGlobalPos.getPos();

            f64 dx = static_cast<f64>(ownerPos.x) - static_cast<f64>(doorPos.x + 0.5);
            f64 dz = static_cast<f64>(ownerPos.z) - static_cast<f64>(doorPos.z + 0.5);
            f64 distSq = dx * dx + dz * dz;

            if (distSq > 4.0) {
                continue;
            }

            const BlockState* state = world->getBlockState(doorPos);
            if (!state) {
                continue;
            }

            const Block& block = state->getBlock();
            auto* doorBlock = dynamic_cast<const blocks::DoorBlock*>(&block);
            if (!doorBlock || doorBlock->isIronDoor()) {
                continue;
            }

            bool isCurrentlyOpen = blocks::DoorBlock::isOpen(*state);

            auto* navigator = owner->navigator();
            bool needsOpen = false;
            if (navigator && navigator->hasPath()) {
                if (!isCurrentlyOpen && distSq <= 2.25) {
                    needsOpen = true;
                }
            }

            if (needsOpen) {
                const_cast<blocks::DoorBlock*>(doorBlock)->toggleDoor(*world, doorPos, true);

                std::unordered_set<GlobalPos> updatedDoors;
                if (openedDoors.has_value()) {
                    updatedDoors = *openedDoors;
                }
                updatedDoors.insert(doorGlobalPos);
                brain.template setMemory<std::unordered_set<GlobalPos>>(
                    MemoryModuleTypes::OPENED_DOORS, std::move(updatedDoors));
            }
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!world) {
            return;
        }

        auto& brain = owner->brain();
        auto openedDoors = brain.template getMemory<std::unordered_set<GlobalPos>>(MemoryModuleTypes::OPENED_DOORS);

        if (openedDoors.has_value() && !openedDoors->empty()) {
            for (const auto& doorGlobalPos : *openedDoors) {
                if (doorGlobalPos.getDimensionId() != owner->dimension()) {
                    continue;
                }

                const BlockPos& doorPos = doorGlobalPos.getPos();
                const BlockState* state = world->getBlockState(doorPos);
                if (!state) {
                    continue;
                }

                const Block& block = state->getBlock();
                auto* doorBlock = dynamic_cast<const blocks::DoorBlock*>(&block);
                if (doorBlock && blocks::DoorBlock::isOpen(*state)) {
                    const_cast<blocks::DoorBlock*>(doorBlock)->toggleDoor(*world, doorPos, false);
                }
            }
        }

        brain.template removeMemory<std::unordered_set<GlobalPos>>(MemoryModuleTypes::OPENED_DOORS);
    }
};

/**
 * @brief 跟随主人任务
 *
 * 驯服动物（狼、猫等）在未坐下时跟随主人移动，
 * 距离过远时传送到主人身边。
 *
 * TODO: 此任务需要 TameableEntity 子类（WolfEntity、CatEntity 等）集成 Brain 系统后
 * 才能注册使用。当前这些实体仅使用 Goal 系统，对应的 Goal 版本为 FollowOwnerGoal。
 * 集成 Brain 后需在对应实体的 initializeBrain() 中注册此任务和 OwnerHurtBySensor。
 */
template <typename E>
class FollowOwnerTask : public Task<E> {
public:
    FollowOwnerTask(
        f32 speed = 1.0f, f32 startDistance = 10.0f, f32 stopDistance = 2.0f, f32 teleportDistance = 12.0f) noexcept
        : Task<E>({}, 60, 200)
        , m_speed(speed)
        , m_startDistance(startDistance)
        , m_stopDistance(stopDistance)
        , m_teleportDistance(teleportDistance)
        , m_pathRecalcCounter(0)
    {}

    std::string getName() const noexcept override { return "FollowOwnerTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto* tameable = dynamic_cast<TameableEntity*>(owner);
        if (!tameable || !tameable->isTamed() || tameable->isSitting()) {
            return false;
        }

        Player* ownerPlayer = tameable->getOwner();
        if (!ownerPlayer || !ownerPlayer->isAlive()) {
            return false;
        }

        f32 distSq = owner->distanceSqTo(*ownerPlayer);
        return distSq > m_startDistance * m_startDistance;
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto* tameable = dynamic_cast<TameableEntity*>(owner);
        if (!tameable || !tameable->isTamed() || tameable->isSitting()) {
            return false;
        }

        Player* ownerPlayer = tameable->getOwner();
        if (!ownerPlayer || !ownerPlayer->isAlive()) {
            return false;
        }

        f32 distSq = owner->distanceSqTo(*ownerPlayer);
        return distSq > m_stopDistance * m_stopDistance;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override { m_pathRecalcCounter = 0; }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto* tameable = dynamic_cast<TameableEntity*>(owner);
        if (!tameable) {
            return;
        }

        Player* ownerPlayer = tameable->getOwner();
        if (!ownerPlayer) {
            return;
        }

        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *ownerPlayer, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        f32 distSq = owner->distanceSqTo(*ownerPlayer);

        if (distSq > m_teleportDistance * m_teleportDistance) {
            if (_tryTeleportToOwner(world, owner, ownerPlayer)) {
                return;
            }
        }

        if (--m_pathRecalcCounter <= 0) {
            m_pathRecalcCounter = 10;

            if (distSq > m_stopDistance * m_stopDistance) {
                auto* navigator = owner->navigator();
                if (navigator) {
                    navigator->moveTo(*ownerPlayer, static_cast<f64>(m_speed));
                }
            }
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto* navigator = owner->navigator();
        if (navigator) {
            navigator->clearPath();
        }
    }

private:
    bool _tryTeleportToOwner(IWorld* world, E* owner, Player* ownerPlayer)
    {
        if (!world) {
            return false;
        }

        math::Random& random = owner->getRandom();

        for (i32 attempt = 0; attempt < 10; ++attempt) {
            f32 angle = random.nextFloat() * math::TWO_PI;
            f32 distance = 1.0f + random.nextFloat() * 2.0f;

            f32 targetX = ownerPlayer->x() + std::cos(angle) * distance;
            f32 targetZ = ownerPlayer->z() + std::sin(angle) * distance;
            f32 targetY = ownerPlayer->y();

            for (i32 yOffset = -1; yOffset <= 1; ++yOffset) {
                f32 testY = targetY + static_cast<f32>(yOffset);

                AxisAlignedBB entityBox = owner->boundingBox();
                AxisAlignedBB testBox(targetX - entityBox.width() / 2.0f,
                    testY,
                    targetZ - entityBox.width() / 2.0f,
                    targetX + entityBox.width() / 2.0f,
                    testY + entityBox.height(),
                    targetZ + entityBox.width() / 2.0f);

                if (world->hasNoCollisions(testBox)) {
                    owner->setPosition(targetX, testY, targetZ);
                    auto* navigator = owner->navigator();
                    if (navigator) {
                        navigator->clearPath();
                    }
                    return true;
                }
            }
        }

        return false;
    }

    f32 m_speed;
    f32 m_startDistance;
    f32 m_stopDistance;
    f32 m_teleportDistance;
    i32 m_pathRecalcCounter;
};

/**
 * @brief 保护主人任务
 *
 * 当驯服动物的主人被攻击时，设置攻击目标到攻击者。
 * 通过读取 OWNER_HURT_BY 记忆模块获取攻击者信息。
 *
 * TODO: 此任务需要 TameableEntity 子类（WolfEntity、CatEntity 等）集成 Brain 系统后
 * 才能注册使用。当前这些实体仅使用 Goal 系统，对应的 Goal 版本为 OwnerHurtByTargetGoal。
 * 集成 Brain 后需在对应实体的 initializeBrain() 中注册此任务和 OwnerHurtBySensor。
 */
template <typename E>
class ProtectOwnerTask : public Task<E> {
public:
    ProtectOwnerTask(f32 speed = 1.5f, f32 protectRange = 16.0f) noexcept
        : Task<E>({{MemoryModuleTypes::OWNER_HURT_BY, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::REGISTERED}},
              60,
              600)
        , m_speed(speed)
        , m_protectRange(protectRange)
    {}

    std::string getName() const noexcept override { return "ProtectOwnerTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto* tameable = dynamic_cast<TameableEntity*>(owner);
        if (!tameable || !tameable->isTamed() || tameable->isSitting()) {
            return false;
        }

        Player* ownerPlayer = tameable->getOwner();
        if (!ownerPlayer || !ownerPlayer->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto hurtById = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::OWNER_HURT_BY);
        if (!hurtById.has_value() || *hurtById == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        LivingEntity* hurtBy = nullptr;
        if (world != nullptr) {
            Entity* hurtByEntity = world->getEntity(*hurtById);
            if (hurtByEntity != nullptr && hurtByEntity->isAlive()) {
                hurtBy = dynamic_cast<LivingEntity*>(hurtByEntity);
            }
        }
        if (hurtBy == nullptr) {
            return false;
        }

        if (hurtBy == ownerPlayer) {
            return false;
        }

        f32 distSq = owner->distanceSqTo(*hurtBy);
        return distSq <= m_protectRange * m_protectRange;
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto attackTargetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET);
        if (!attackTargetId.has_value() || *attackTargetId == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* attackTargetEntity = (world != nullptr) ? world->getEntity(*attackTargetId) : nullptr;
        if (attackTargetEntity == nullptr || !attackTargetEntity->isAlive()) {
            return false;
        }

        f32 distSq = owner->distanceSqTo(*attackTargetEntity);
        return distSq <= m_protectRange * m_protectRange * 4.0f;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto hurtById = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::OWNER_HURT_BY);
        if (hurtById.has_value() && *hurtById != INVALID_ENTITY_ID && world != nullptr) {
            Entity* hurtByEntity = world->getEntity(*hurtById);
            if (hurtByEntity != nullptr && hurtByEntity->isAlive()) {
                LivingEntity* attacker = dynamic_cast<LivingEntity*>(hurtByEntity);
                if (attacker != nullptr) {
                    brain.template setMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET, attacker->id());
                    owner->setAttackTarget(attacker);
                }
            }
        }
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto attackTargetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET);
        if (!attackTargetId.has_value() || *attackTargetId == INVALID_ENTITY_ID) {
            return;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* attackTargetEntity = (world != nullptr) ? world->getEntity(*attackTargetId) : nullptr;
        if (attackTargetEntity == nullptr || !attackTargetEntity->isAlive()) {
            return;
        }

        LivingEntity* target = dynamic_cast<LivingEntity*>(attackTargetEntity);
        if (target == nullptr) {
            return;
        }

        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *target, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        f32 distSq = owner->distanceSqTo(*target);
        if (distSq > goal::constants::MELEE_ATTACK_STOP_DISTANCE * goal::constants::MELEE_ATTACK_STOP_DISTANCE) {
            auto* navigator = owner->navigator();
            if (navigator) {
                navigator->moveTo(*target, static_cast<f64>(m_speed));
            }
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        owner->setAttackTarget(nullptr);
        auto& brain = owner->brain();
        brain.template removeMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET);
        brain.template removeMemory<EntityInstanceId>(MemoryModuleTypes::OWNER_HURT_BY);
    }

private:
    f32 m_speed;
    f32 m_protectRange;
};

/**
 * @brief 拾取物品任务
 *
 * 实体检测到 NEAREST_VISIBLE_WANTED_ITEM 记忆后，
 * 导航到物品位置并拾取物品。
 *
 * TODO: 此任务需要注册到具有拾取物品行为的实体（如 VillagerEntity、PiglinEntity 等）的
 * Brain 中。需要配合对应的传感器来写入 NEAREST_VISIBLE_WANTED_ITEM 记忆。
 */
template <typename E>
class PickupItemTask : public Task<E> {
public:
    PickupItemTask(f32 speed = 1.0f, f32 pickupRange = 1.5f) noexcept
        : Task<E>({{MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM, MemoryModuleStatus::VALUE_PRESENT}}, 20, 60)
        , m_speed(speed)
        , m_pickupRange(pickupRange)
        , m_pathRecalcCounter(0)
    {}

    std::string getName() const noexcept override { return "PickupItemTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto wantedItemId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
        if (!wantedItemId.has_value() || *wantedItemId == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* wantedEntity = (world != nullptr) ? world->getEntity(*wantedItemId) : nullptr;
        if (wantedEntity == nullptr || !wantedEntity->isAlive()) {
            return false;
        }

        auto* item = dynamic_cast<ItemEntity*>(wantedEntity);
        return item != nullptr && item->isAlive() && item->canBePickedUp();
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto wantedItemId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
        if (!wantedItemId.has_value() || *wantedItemId == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* wantedEntity = (world != nullptr) ? world->getEntity(*wantedItemId) : nullptr;
        if (wantedEntity == nullptr || !wantedEntity->isAlive()) {
            return false;
        }

        f32 distSq = owner->distanceSqTo(*wantedEntity);
        return distSq <= goal::constants::TEMPT_RANGE * goal::constants::TEMPT_RANGE;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        m_pathRecalcCounter = 0;

        auto& brain = owner->brain();
        auto wantedItemId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
        if (wantedItemId.has_value() && *wantedItemId != INVALID_ENTITY_ID && world != nullptr) {
            Entity* wantedEntity = world->getEntity(*wantedItemId);
            if (wantedEntity != nullptr && wantedEntity->isAlive()) {
                auto* navigator = owner->navigator();
                if (navigator) {
                    navigator->moveTo(*wantedEntity, static_cast<f64>(m_speed));
                }
            }
        }
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto wantedItemId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
        if (!wantedItemId.has_value() || *wantedItemId == INVALID_ENTITY_ID) {
            return;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* wantedEntity = (world != nullptr) ? world->getEntity(*wantedItemId) : nullptr;
        if (wantedEntity == nullptr || !wantedEntity->isAlive()) {
            return;
        }

        ItemEntity* item = dynamic_cast<ItemEntity*>(wantedEntity);
        if (item == nullptr) {
            return;
        }

        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(*item, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        if (!item->isAlive() || !item->canBePickedUp()) {
            brain.template removeMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
            return;
        }

        f32 distSq = owner->distanceSqTo(*item);

        if (distSq <= m_pickupRange * m_pickupRange) {
            item->setPickupDelay(0);
            brain.template removeMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
        } else {
            if (--m_pathRecalcCounter <= 0) {
                m_pathRecalcCounter = 10;
                auto* navigator = owner->navigator();
                if (navigator) {
                    navigator->moveTo(*item, static_cast<f64>(m_speed));
                }
            }
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto* navigator = owner->navigator();
        if (navigator) {
            navigator->clearPath();
        }
    }

private:
    f32 m_speed;
    f32 m_pickupRange;
    i32 m_pathRecalcCounter;
};

/**
 * @brief 跟随父母任务
 *
 * 幼年动物跟随附近的成年同类。
 * 基于 NEAREST_VISIBLE_ADULT 记忆模块。
 *
 * TODO: 此任务需要 AnimalEntity 子类（如 CowEntity、PigEntity、SheepEntity 等）
 * 集成 Brain 系统后才能注册使用。当前这些实体仅使用 Goal 系统，
 * 对应的 Goal 版本为 FollowParentGoal。
 * 集成 Brain 后需在对应实体的 initializeBrain() 中注册此任务和 BabySensor。
 */
template <typename E>
class FollowParentTask : public Task<E> {
public:
    FollowParentTask(f32 speed = 1.0f, f32 followDistance = 4.0f) noexcept
        : Task<E>({{MemoryModuleTypes::NEAREST_VISIBLE_ADULT, MemoryModuleStatus::VALUE_PRESENT}}, 60, 200)
        , m_speed(speed)
        , m_followDistance(followDistance)
        , m_pathRecalcCounter(0)
    {}

    std::string getName() const noexcept override { return "FollowParentTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto* ageable = dynamic_cast<AgeableEntity*>(owner);
        if (!ageable || !ageable->isChild()) {
            return false;
        }

        auto& brain = owner->brain();
        auto parentId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_ADULT);
        if (!parentId.has_value() || *parentId == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* parentEntity = (world != nullptr) ? world->getEntity(*parentId) : nullptr;
        if (parentEntity == nullptr || !parentEntity->isAlive()) {
            return false;
        }

        f32 distSq = owner->distanceSqTo(*parentEntity);
        f32 minDistSq = goal::constants::FOLLOW_PARENT_MIN_DISTANCE * goal::constants::FOLLOW_PARENT_MIN_DISTANCE;
        return distSq > minDistSq;
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto* ageable = dynamic_cast<AgeableEntity*>(owner);
        if (!ageable || !ageable->isChild()) {
            return false;
        }

        auto& brain = owner->brain();
        auto parentId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_ADULT);
        if (!parentId.has_value() || *parentId == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* parentEntity = (world != nullptr) ? world->getEntity(*parentId) : nullptr;
        if (parentEntity == nullptr || !parentEntity->isAlive()) {
            return false;
        }

        f32 distSq = owner->distanceSqTo(*parentEntity);
        f32 minDistSq = goal::constants::FOLLOW_PARENT_MIN_DISTANCE * goal::constants::FOLLOW_PARENT_MIN_DISTANCE;
        f32 maxDistSq = goal::constants::FOLLOW_PARENT_MAX_DISTANCE * goal::constants::FOLLOW_PARENT_MAX_DISTANCE;
        return distSq > minDistSq && distSq < maxDistSq;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        m_pathRecalcCounter = 0;

        auto& brain = owner->brain();
        auto parentId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_ADULT);
        if (parentId.has_value() && *parentId != INVALID_ENTITY_ID && world != nullptr) {
            Entity* parentEntity = world->getEntity(*parentId);
            if (parentEntity != nullptr && parentEntity->isAlive()) {
                auto* navigator = owner->navigator();
                if (navigator) {
                    navigator->moveTo(*parentEntity, static_cast<f64>(m_speed));
                }
            }
        }
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto parentId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::NEAREST_VISIBLE_ADULT);
        if (!parentId.has_value() || *parentId == INVALID_ENTITY_ID) {
            return;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* parentEntity = (world != nullptr) ? world->getEntity(*parentId) : nullptr;
        if (parentEntity == nullptr || !parentEntity->isAlive()) {
            return;
        }

        AgeableEntity* parent = dynamic_cast<AgeableEntity*>(parentEntity);
        if (parent == nullptr) {
            return;
        }

        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *parent, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        if (--m_pathRecalcCounter <= 0) {
            m_pathRecalcCounter = goal::constants::FOLLOW_DELAY_INTERVAL;

            f32 distSq = owner->distanceSqTo(*parent);
            f32 followDistSq = m_followDistance * m_followDistance;

            if (distSq > followDistSq) {
                auto* navigator = owner->navigator();
                if (navigator) {
                    navigator->moveTo(*parent, static_cast<f64>(m_speed));
                }
            } else {
                auto* navigator = owner->navigator();
                if (navigator) {
                    navigator->clearPath();
                }
            }
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto* navigator = owner->navigator();
        if (navigator) {
            navigator->clearPath();
        }
    }

private:
    f32 m_speed;
    f32 m_followDistance;
    i32 m_pathRecalcCounter;
};

/**
 * @brief 诱惑任务
 *
 * 动物被手持诱惑物品的玩家吸引，导航走向玩家。
 * 基于 TEMPTING_PLAYER 记忆模块。
 *
 * TODO: 此任务需要 AnimalEntity 子类集成 Brain 系统后才能注册使用。
 * 当前这些实体仅使用 Goal 系统，对应的 Goal 版本为 TemptGoal。
 * 集成 Brain 后需在对应实体的 initializeBrain() 中注册此任务和 TemptingPlayerSensor。
 */
template <typename E>
class TemptTask : public Task<E> {
public:
    TemptTask(f32 speed = 1.0f, f32 range = 10.0f) noexcept
        : Task<E>({{MemoryModuleTypes::TEMPTING_PLAYER, MemoryModuleStatus::VALUE_PRESENT}}, 60, 200)
        , m_speed(speed)
        , m_range(range)
        , m_pathRecalcCounter(0)
    {}

    std::string getName() const noexcept override { return "TemptTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto temptingPlayerId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::TEMPTING_PLAYER);
        if (!temptingPlayerId.has_value() || *temptingPlayerId == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* playerEntity = (world != nullptr) ? world->getEntity(*temptingPlayerId) : nullptr;
        return playerEntity != nullptr && playerEntity->isAlive();
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto temptingPlayerId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::TEMPTING_PLAYER);
        if (!temptingPlayerId.has_value() || *temptingPlayerId == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* playerEntity = (world != nullptr) ? world->getEntity(*temptingPlayerId) : nullptr;
        if (playerEntity == nullptr || !playerEntity->isAlive()) {
            return false;
        }

        f32 distSq = owner->distanceSqTo(*playerEntity);
        f32 rangeSq = m_range * m_range;
        return distSq <= rangeSq;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        m_pathRecalcCounter = 0;

        auto& brain = owner->brain();
        auto temptingPlayerId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::TEMPTING_PLAYER);
        if (temptingPlayerId.has_value() && *temptingPlayerId != INVALID_ENTITY_ID && world != nullptr) {
            Entity* playerEntity = world->getEntity(*temptingPlayerId);
            if (playerEntity != nullptr && playerEntity->isAlive()) {
                auto* navigator = owner->navigator();
                if (navigator) {
                    navigator->moveTo(*playerEntity, static_cast<f64>(m_speed));
                }
            }
        }
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto temptingPlayerId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::TEMPTING_PLAYER);
        if (!temptingPlayerId.has_value() || *temptingPlayerId == INVALID_ENTITY_ID) {
            return;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* playerEntity = (world != nullptr) ? world->getEntity(*temptingPlayerId) : nullptr;
        if (playerEntity == nullptr || !playerEntity->isAlive()) {
            return;
        }

        Player* player = dynamic_cast<Player*>(playerEntity);
        if (player == nullptr) {
            return;
        }

        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *player, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        f32 distSq = owner->distanceSqTo(*player);
        f32 closeDistSq = goal::constants::TEMPT_CLOSE_DISTANCE * goal::constants::TEMPT_CLOSE_DISTANCE;

        if (distSq <= closeDistSq) {
            auto* navigator = owner->navigator();
            if (navigator) {
                navigator->clearPath();
            }
        } else {
            if (--m_pathRecalcCounter <= 0) {
                m_pathRecalcCounter = 10;
                auto* navigator = owner->navigator();
                if (navigator) {
                    navigator->moveTo(*player, static_cast<f64>(m_speed));
                }
            }
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto* navigator = owner->navigator();
        if (navigator) {
            navigator->clearPath();
        }

        owner->brain().template removeMemory<EntityInstanceId>(MemoryModuleTypes::TEMPTING_PLAYER);
    }

private:
    f32 m_speed;
    f32 m_range;
    i32 m_pathRecalcCounter;
};

} // namespace interact
} // namespace task
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
