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

#include "BreedGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"

#include <cmath>
#include <memory>
#include <utility>

namespace mc::entity::ai::goal {

using namespace constants;

BreedGoal::BreedGoal(AnimalEntity* animal, f64 speed)
    : m_animal(animal)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool BreedGoal::shouldExecute()
{
    if (!m_animal) return false;

    // 检查是否处于爱心状态
    if (!m_animal->isInLove()) {
        return false;
    }

    // 寻找配偶
    m_targetMate = findNearbyMate();
    return m_targetMate != nullptr;
}

bool BreedGoal::shouldContinueExecuting()
{
    if (!m_targetMate) return false;

    // 检查配偶是否存活且仍处于爱心状态，且未超时
    if (!m_targetMate->isAlive()) return false;
    if (!m_targetMate->isInLove()) return false;

    return m_spawnBabyDelay < constants::SPAWN_BABY_DELAY;
}

void BreedGoal::startExecuting()
{
    m_spawnBabyDelay = 0;
}

void BreedGoal::resetTask()
{
    m_targetMate = nullptr;
    m_spawnBabyDelay = 0;
    if (m_animal) {
        m_animal->clearNavigation();
    }
}

void BreedGoal::tick()
{
    if (!m_animal || !m_targetMate) return;

    // 使用 LookController 看向配偶
    if (auto* lookCtrl = m_animal->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_targetMate, 10.0f, m_animal->getVerticalFaceSpeed());
    }

    // 移动向配偶
    if (auto* nav = m_animal->navigator()) {
        (void)nav->moveTo(*m_targetMate, m_speed);
    }

    m_spawnBabyDelay++;

    // 检查是否足够接近以进行繁殖
    f64 distSq = m_animal->distanceSqTo(*m_targetMate);
    if (m_spawnBabyDelay >= constants::SPAWN_BABY_DELAY && distSq < constants::BREED_DISTANCE_SQ) {
        spawnBaby();
    }
}

AnimalEntity* BreedGoal::findNearbyMate()
{
    if (!m_animal || !m_animal->world()) return nullptr;

    // 在检测范围内寻找配偶
    return EntityUtils::findClosestEntity<AnimalEntity>(m_animal->world(),
        m_animal->position(),
        constants::BREED_DETECTION_RANGE,
        m_animal,
        [this](AnimalEntity* animal) { return m_animal->canMateWith(*animal); });
}

void BreedGoal::spawnBaby()
{
    if (!m_animal || !m_targetMate) return;

    // 重置爱心状态
    m_animal->resetInLove();
    m_targetMate->resetInLove();

    // 设置繁殖冷却 (6000 ticks = 5分钟)
    m_animal->setGrowingAge(AgeableEntity::BREEDING_COOLDOWN);
    m_targetMate->setGrowingAge(AgeableEntity::BREEDING_COOLDOWN);

    // 生成幼体
    auto baby = m_animal->spawnBaby(*m_targetMate);
    if (baby) {
        baby->setTypeId(m_animal->getTypeId());

        IWorld* world = m_animal->world();
        if (world) {
            // 设置幼体位置（在父母之间随机偏移）
            mc::math::Random& rng = m_animal->getRandom();
            f32 babyX = static_cast<f32>(m_animal->x() + (rng.nextDouble() - 0.5) * 2.0);
            f32 babyY = static_cast<f32>(m_animal->y());
            f32 babyZ = static_cast<f32>(m_animal->z() + (rng.nextDouble() - 0.5) * 2.0);
            baby->setPosition(babyX, babyY, babyZ);

            // 设置幼体年龄
            baby->setGrowingAge(AgeableEntity::BABY_AGE);

            // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化（使用位置感知的区域难度）
            auto* babyMob = dynamic_cast<MobEntity*>(baby.get());
            if (babyMob != nullptr) {
                entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*world,
                    BlockPos(static_cast<i32>(std::floor(babyX)),
                        static_cast<i32>(babyY),
                        static_cast<i32>(std::floor(babyZ))));
                babyMob->finalizeSpawn(*world, difficultyInstance, world::spawn::SpawnReason::Breeding);
            }

            // 获取繁殖发起者玩家（优先从第一个动物获取 loveCause，如果为空则从第二个动物获取）
            u64 loveCause = m_animal->getLoveCause();
            if (loveCause == 0) {
                loveCause = m_targetMate->getLoveCause();
            }

            // 保存幼体指针（spawnEntity 后 unique_ptr 会被移动）
            Entity* babyPtr = baby.get();

            // 生成到世界中
            world->spawnEntity(std::move(baby));

            // 触发繁殖事件（用于成就触发）
            if (loveCause != 0) {
                world->onBredAnimals(static_cast<PlayerId>(loveCause), babyPtr, m_animal, m_targetMate);
            }

            // 生成爱心粒子效果
            m_animal->spawnHeartParticles();
            m_targetMate->spawnHeartParticles();

            // 生成 1-7 个经验球
            i32 xpCount = 1 + rng.nextInt(7);
            for (i32 i = 0; i < xpCount; ++i) {
                auto xpOrb =
                    std::make_unique<ExperienceOrbEntity>(world, m_animal->x(), m_animal->y(), m_animal->z(), 1);

                // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
                xpOrb->setTypeId(EntityTypeKeys::EXPERIENCE_ORB);

                // 添加随机速度
                f32 vx = (rng.nextFloat() - 0.5f) * 0.2f;
                f32 vy = rng.nextFloat() * 0.2f;
                f32 vz = (rng.nextFloat() - 0.5f) * 0.2f;
                xpOrb->setVelocity(vx, vy, vz);
                world->spawnEntity(std::move(xpOrb));
            }
        }
    }
}

} // namespace mc::entity::ai::goal
