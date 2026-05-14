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
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../core/AgeableEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../entities/orb/ExperienceOrbEntity.hpp"
#include "../../../entities/passive/basic/AnimalEntity.hpp"
#include "../../controller/LookController.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../GoalConstants.hpp"
#include <cmath>

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

    // MC 1.16.5: 检查是否处于爱心状态
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

    // MC 1.16.5: 检查配偶是否存活且仍处于爱心状态，且未超时
    if (!m_targetMate->isAlive()) return false;
    if (!m_targetMate->isInLove()) return false;

    // MC 1.16.5: spawnBabyDelay < 60
    return m_spawnBabyDelay < SPAWN_BABY_DELAY;
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

    // MC 1.16.5: 使用 LookController 看向配偶
    // setLookPositionWithEntity(targetMate, 10.0F, getVerticalFaceSpeed())
    if (auto* lookCtrl = m_animal->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_targetMate, 10.0f, m_animal->getVerticalFaceSpeed());
    }

    // MC 1.16.5: 使用 navigator.tryMoveToEntityLiving(targetMate, moveSpeed)
    if (auto* nav = m_animal->navigator()) {
        static_cast<void>(nav->moveTo(*m_targetMate, m_speed));
    }

    m_spawnBabyDelay++;

    // MC 1.16.5: spawnBabyDelay >= 60 && distanceSq < 9.0D
    f64 distSq = m_animal->distanceSqTo(*m_targetMate);
    if (m_spawnBabyDelay >= SPAWN_BABY_DELAY && distSq < BREED_DISTANCE_SQ) {
        spawnBaby();
    }
}

AnimalEntity* BreedGoal::findNearbyMate()
{
    if (!m_animal || !m_animal->world()) return nullptr;

    // MC 1.16.5: 在 8 格范围内寻找配偶，使用 EntityPredicate
    // EntityPredicate.setDistance(8.0D).allowInvulnerable().allowFriendlyFire().setLineOfSiteRequired()
    return EntityUtils::findClosestEntity<AnimalEntity>(
        m_animal->world(), m_animal->position(), BREED_DETECTION_RANGE, m_animal, [this](AnimalEntity* animal) {
            return m_animal->canMateWith(*animal);
        });
}

void BreedGoal::spawnBaby()
{
    if (!m_animal || !m_targetMate) return;

    // MC 1.16.5: 重置爱心状态
    m_animal->resetInLove();
    m_targetMate->resetInLove();

    // MC 1.16.5: 设置繁殖冷却 (6000 ticks = 5分钟)
    m_animal->setGrowingAge(AgeableEntity::BREEDING_COOLDOWN);
    m_targetMate->setGrowingAge(AgeableEntity::BREEDING_COOLDOWN);

    // 生成幼体
    auto baby = m_animal->spawnBaby(*m_targetMate);
    if (baby) {
        baby->setTypeId(m_animal->getTypeId());

        IWorld* world = m_animal->world();
        if (world) {
            // 设置幼体位置（在父母之间随机偏移）
            mc::math::Random rng = m_animal->getRandom();
            f32 babyX = static_cast<f32>(m_animal->x() + (rng.nextDouble() - 0.5) * 2.0);
            f32 babyY = static_cast<f32>(m_animal->y());
            f32 babyZ = static_cast<f32>(m_animal->z() + (rng.nextDouble() - 0.5) * 2.0);
            baby->setPosition(babyX, babyY, babyZ);

            // 设置幼体年龄
            baby->setGrowingAge(AgeableEntity::BABY_AGE);

            // 生成到世界中
            world->spawnEntity(std::move(baby));

            // MC 1.16.5: 生成爱心粒子效果
            m_animal->spawnHeartParticles();
            m_targetMate->spawnHeartParticles();

            // MC 1.16.5: 生成 1-7 个经验球
            i32 xpCount = 1 + rng.nextInt(7);
            for (i32 i = 0; i < xpCount; ++i) {
                auto xpOrb =
                    std::make_unique<ExperienceOrbEntity>(world, m_animal->x(), m_animal->y(), m_animal->z(), 1);
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
