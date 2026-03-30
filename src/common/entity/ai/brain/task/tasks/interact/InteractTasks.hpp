#pragma once

#include "../Task.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../world/ServerWorld.hpp"
#include "../../memory/Brain.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace task {
namespace interact {

/**
 * @brief 与村民互动任务
 *
 * 控制村民与玩家互动。
 *
 * 参考 MC 1.16.5 VillagerInteractTask
 */
template <typename E>
class VillagerInteractTask : public Task<E> {
public:
    VillagerInteractTask(f32 range = 3.0f)
        : Task<E>({}, 30, 60)
        , m_range(range)
    {}

    std::string getName() const override {
        return "VillagerInteractTask";
    }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override {
        if (!owner || !owner->isAlive()) return false;

        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::INTERACTION_TARGET);
        // return target.has_value() && (*target)->isAlive();
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override {
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::INTERACTION_TARGET);
        // if (target.has_value()) {
        //     owner->getNavigator()->moveTo(*target, 0.5f);
        // }
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::INTERACTION_TARGET);
        // if (!target.has_value()) return;
        //
        // auto targetEntity = *target;
        // owner->getLookController()->setLookAt(targetEntity);
        //
        // if (owner->distanceTo(*targetEntity) <= m_range) {
        //     // 执行互动
        //     interactWith(owner, targetEntity);
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // owner->getBrain()->removeMemory(memory::MemoryModuleTypes::INTERACTION_TARGET);
    }

private:
    f32 m_range;

    // void interactWith(E* owner, LivingEntity* target) {
    //     // 根据职业类型执行不同的互动
    // }
};

/**
 * @brief 与门互动任务
 *
 * 控制村民开门/关门。
 *
 * 参考 MC 1.16.5 InteractWithDoorTask
 */
template <typename E>
class InteractWithDoorTask : public Task<E> {
public:
    InteractWithDoorTask()
        : Task<E>({}, 20, 40)
    {}

    std::string getName() const override {
        return "InteractWithDoorTask";
    }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override {
        if (!owner || !owner->isAlive()) return false;

        // 检查是否需要开门
        // auto brain = owner->getBrain();
        // auto path = owner->getNavigator()->getPath();
        // return path && path->containsDoor();
        return false;
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // 检查路径上的门
        // auto doors = getDoorsInPath(owner);
        // for (auto& doorPos : doors) {
        //     toggleDoor(world, owner, doorPos);
        // }
    }

private:
    // void toggleDoor(ServerWorld* world, E* owner, const BlockPos& pos) {
    //     auto blockState = world->getBlockState(pos);
    //     if (blockState && blockState->isDoor()) {
    //         // 切换门状态
    //         world->setBlockState(pos, blockState->cycleProperty(DoorBlock::OPEN));
    //     }
    // }
};

/**
 * @brief 跟随玩家任务
 *
 * 控制驯服动物跟随玩家。
 *
 * 参考 MC 1.16.5 FollowOwnerTask
 */
template <typename E>
class FollowOwnerTask : public Task<E> {
public:
    FollowOwnerTask(f32 speed = 1.0f, f32 startDistance = 10.0f, f32 stopDistance = 2.0f)
        : Task<E>({}, 60, 200)
        , m_speed(speed)
        , m_startDistance(startDistance)
        , m_stopDistance(stopDistance)
    {}

    std::string getName() const override {
        return "FollowOwnerTask";
    }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override {
        if (!owner || !owner->isAlive()) return false;

        // 检查是否是驯服动物
        // TameableEntity* tameable = dynamic_cast<TameableEntity*>(owner);
        // if (!tameable || !tameable->isTamed()) return false;
        //
        // Player* owner = tameable->getOwner();
        // if (!owner || !owner->isAlive()) return false;
        //
        // return owner->distanceTo(*owner) > m_startDistance;
        return false;
    }

    bool shouldContinueExecuting(ServerWorld* world, E* owner, i64 gameTime) override {
        if (!owner || !owner->isAlive()) return false;

        // Player* owner = tameable->getOwner();
        // return owner && owner->isAlive() &&
        //        owner->distanceTo(*owner) > m_stopDistance;
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override {
        // TameableEntity* tameable = dynamic_cast<TameableEntity*>(owner);
        // Player* owner = tameable->getOwner();
        // if (owner) {
        //     owner->getNavigator()->moveTo(*owner, m_speed);
        // }
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // TameableEntity* tameable = dynamic_cast<TameableEntity*>(owner);
        // Player* owner = tameable->getOwner();
        // if (owner) {
        //     owner->getLookController()->setLookAt(*owner);
        //     if (owner->distanceTo(*owner) > m_stopDistance) {
        //         owner->getNavigator()->moveTo(*owner, m_speed);
        //     }
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // owner->getNavigator()->clearPath();
    }

private:
    f32 m_speed;
    f32 m_startDistance;
    f32 m_stopDistance;
};

/**
 * @brief 保护主人任务
 *
 * 控制驯服动物保护主人。
 *
 * 参考 MC 1.16.5 OwnerHurtByTargetTask / OwnerHurtTargetTask
 */
template <typename E>
class ProtectOwnerTask : public Task<E> {
public:
    ProtectOwnerTask(f32 speed = 1.5f, f32 protectRange = 16.0f)
        : Task<E>({}, 60, 600)
        , m_speed(speed)
        , m_protectRange(protectRange)
    {}

    std::string getName() const override {
        return "ProtectOwnerTask";
    }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override {
        if (!owner || !owner->isAlive()) return false;

        // 检查是否是驯服动物
        // TameableEntity* tameable = dynamic_cast<TameableEntity*>(owner);
        // if (!tameable || !tameable->isTamed()) return false;
        //
        // Player* owner = tameable->getOwner();
        // if (!owner || !owner->isAlive()) return false;
        //
        // // 检查主人是否有攻击者
        // LivingEntity* attacker = owner->getLastHurtBy();
        // if (!attacker || !attacker->isAlive()) return false;
        //
        // return owner->distanceTo(*attacker) <= m_protectRange;
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override {
        // TameableEntity* tameable = dynamic_cast<TameableEntity*>(owner);
        // Player* owner = tameable->getOwner();
        // LivingEntity* attacker = owner->getLastHurtBy();
        //
        // if (attacker && attacker->isAlive()) {
        //     owner->setAttackTarget(attacker);
        //     owner->getBrain()->setMemory(memory::MemoryModuleTypes::ATTACK_TARGET, attacker);
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // owner->setAttackTarget(nullptr);
        // owner->getBrain()->removeMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
    }

private:
    f32 m_speed;
    f32 m_protectRange;
};

/**
 * @brief 拾取物品任务
 *
 * 控制实体拾取附近物品。
 *
 * 参考 MC 1.16.5 PickupItemTask
 */
template <typename E>
class PickupItemTask : public Task<E> {
public:
    PickupItemTask(f32 range = 8.0f, f32 speed = 1.0f)
        : Task<E>({}, 20, 60)
        , m_range(range)
        , m_speed(speed)
    {}

    std::string getName() const override {
        return "PickupItemTask";
    }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override {
        if (!owner || !owner->isAlive()) return false;

        // auto brain = owner->getBrain();
        // auto wantedItem = brain->getMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
        // return wantedItem.has_value() && (*wantedItem)->isAlive();
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override {
        // auto brain = owner->getBrain();
        // auto wantedItem = brain->getMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
        // if (wantedItem.has_value()) {
        //     owner->getNavigator()->moveTo(**wantedItem, m_speed);
        // }
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // auto brain = owner->getBrain();
        // auto wantedItem = brain->getMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
        // if (!wantedItem.has_value()) return;
        //
        // auto item = *wantedItem;
        // owner->getLookController()->setLookAt(*item);
        //
        // // 如果足够近，拾取物品
        // if (owner->distanceTo(*item) < 1.5f) {
        //     owner->pickupItem(item);
        //     brain->removeMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // owner->getNavigator()->clearPath();
    }

private:
    f32 m_range;
    f32 m_speed;
};

/**
 * @brief 跟随父母任务
 *
 * 控制幼年动物跟随父母。
 *
 * 参考 MC 1.16.5 FollowParentTask
 */
template <typename E>
class FollowParentTask : public Task<E> {
public:
    FollowParentTask(f32 speed = 1.0f, f32 followDistance = 4.0f)
        : Task<E>({}, 60, 200)
        , m_speed(speed)
        , m_followDistance(followDistance)
    {}

    std::string getName() const override {
        return "FollowParentTask";
    }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override {
        if (!owner || !owner->isAlive()) return false;

        // 检查是否是幼年动物
        // AgeableEntity* ageable = dynamic_cast<AgeableEntity*>(owner);
        // if (!ageable || !ageable->isChild()) return false;
        //
        // auto brain = owner->getBrain();
        // auto parent = brain->getMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT);
        // return parent.has_value() && (*parent)->isAlive();
        return false;
    }

    bool shouldContinueExecuting(ServerWorld* world, E* owner, i64 gameTime) override {
        // auto brain = owner->getBrain();
        // auto parent = brain->getMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT);
        // return parent.has_value() && (*parent)->isAlive() &&
        //        owner->distanceTo(**parent) > m_followDistance;
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override {
        // auto brain = owner->getBrain();
        // auto parent = brain->getMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT);
        // if (parent.has_value()) {
        //     owner->getNavigator()->moveTo(**parent, m_speed);
        // }
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // auto brain = owner->getBrain();
        // auto parent = brain->getMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT);
        // if (parent.has_value()) {
        //     owner->getLookController()->setLookAt(**parent);
        //     if (owner->distanceTo(**parent) > m_followDistance) {
        //         owner->getNavigator()->moveTo(**parent, m_speed);
        //     }
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // owner->getNavigator()->clearPath();
    }

private:
    f32 m_speed;
    f32 m_followDistance;
};

/**
 * @brief 诱惑任务
 *
 * 控制动物被食物诱惑。
 *
 * 参考 MC 1.16.5 TemptTask
 */
template <typename E>
class TemptTask : public Task<E> {
public:
    TemptTask(f32 speed = 1.0f, f32 range = 10.0f)
        : Task<E>({}, 60, 200)
        , m_speed(speed)
        , m_range(range)
    {}

    std::string getName() const override {
        return "TemptTask";
    }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override {
        if (!owner || !owner->isAlive()) return false;

        // auto brain = owner->getBrain();
        // auto temptingPlayer = brain->getMemory(memory::MemoryModuleTypes::TEMPTING_PLAYER);
        // return temptingPlayer.has_value() && (*temptingPlayer)->isAlive();
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override {
        // auto brain = owner->getBrain();
        // auto temptingPlayer = brain->getMemory(memory::MemoryModuleTypes::TEMPTING_PLAYER);
        // if (temptingPlayer.has_value()) {
        //     owner->getNavigator()->moveTo(**temptingPlayer, m_speed);
        // }
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // auto brain = owner->getBrain();
        // auto temptingPlayer = brain->getMemory(memory::MemoryModuleTypes::TEMPTING_PLAYER);
        // if (temptingPlayer.has_value()) {
        //     auto player = *temptingPlayer;
        //     owner->getLookController()->setLookAt(*player);
        //
        //     // 跟随玩家但保持一定距离
        //     f32 distance = owner->distanceTo(*player);
        //     if (distance > 2.0f && distance < m_range) {
        //         owner->getNavigator()->moveTo(*player, m_speed);
        //     }
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override {
        // owner->getNavigator()->clearPath();
        // owner->getBrain()->removeMemory(memory::MemoryModuleTypes::TEMPTING_PLAYER);
    }

private:
    f32 m_speed;
    f32 m_range;
};

} // namespace interact
} // namespace task
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
