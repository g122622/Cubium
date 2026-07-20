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

#include "VillagerBreedGoal.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

using namespace constants;

// ============================================================================
// VillagerBreedGoal - 村民繁殖目标
// ============================================================================

VillagerBreedGoal::VillagerBreedGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_partnerId(0)
    , m_breedTicks(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool VillagerBreedGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 检查是否愿意繁殖
    if (!_isWillingToBreed()) return false;

    // MC原版 VillagerMakeLove 不在开始时检查床位，
    // 而是在 _spawnChild 时检查。如果无空床位，双方显示愤怒粒子。
    // 保留床位检查作为优化：如果完全无床位则不启动繁殖流程
    // 但在 _spawnChild 中会再次检查，以处理繁殖过程中床位被占用的情况

    // 寻找配偶
    _findPartner();
    return m_partnerId != 0;
}

bool VillagerBreedGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 配偶消失或不再愿意繁殖
    if (m_partnerId == 0) return false;

    // 超时
    return m_breedTicks < BREED_TICKS;
}

void VillagerBreedGoal::startExecuting()
{
    m_breedTicks = 0;
}

void VillagerBreedGoal::resetTask()
{
    m_partnerId = 0;
    m_breedTicks = 0;

    if (m_villager) {
        m_villager->clearNavigation();
        // 重置繁殖意愿
        m_villager->resetBreedWillingness();
    }
}

void VillagerBreedGoal::tick()
{
    if (!m_villager || m_partnerId == 0) return;

    m_breedTicks++;

    // 移动到配偶
    _moveToPartner();

    // 检查配偶是否仍然有效
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_partnerId) : nullptr;
    if (!entity) {
        m_partnerId = 0;
        return;
    }

    VillagerEntity* partner = dynamic_cast<VillagerEntity*>(entity);
    if (!partner || !partner->isAlive()) {
        m_partnerId = 0;
        return;
    }

    // MC原版 VillagerMakeLove.tick: 繁殖过程中双方每隔约35tick随机显示爱心粒子
    if (m_villager->world() != nullptr && m_villager->getRandom().nextInt(35) == 0) {
        m_villager->world()->broadcastEntityStatus(
            m_villager->id(), static_cast<u8>(network::EntityStatusPacket::Status::VillagerHeart));
        m_villager->world()->broadcastEntityStatus(
            partner->id(), static_cast<u8>(network::EntityStatusPacket::Status::VillagerHeart));
    }

    // 检查距离，足够接近时繁殖
    f32 distSq = m_villager->distanceSqTo(*partner);
    if (distSq <= BREED_DISTANCE * BREED_DISTANCE && m_breedTicks >= BREED_TICKS) {
        _spawnChild();
    }
}

bool VillagerBreedGoal::_hasEnoughBeds() const
{
    if (!m_villager) return false;

    // 检查村庄中是否有足够的床位
    // 通过VillageManager获取POI存储，统计可用床位数
    auto* villageManager = m_villager->world()->villageManager();
    if (!villageManager) {
        // 没有VillageManager时，默认允许繁殖
        return true;
    }

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(
        static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));

    // 搜索48格范围内的所有床
    using namespace world::village::poi;
    constexpr f32 BED_SEARCH_RANGE = 48.0f;
    i32 availableBeds = 0;

    // 遍历所有床类型（BedRed 到 BedYellow）
    for (i32 bedType = static_cast<i32>(PointOfInterestType::BedRed);
        bedType <= static_cast<i32>(PointOfInterestType::BedYellow);
        ++bedType) {

        PointOfInterestType poiType = static_cast<PointOfInterestType>(bedType);
        auto pois = poiStorage.findAllInRange(villagerPos, BED_SEARCH_RANGE, poiType);

        for (const auto* poi : pois) {
            if (poi && !poi->isOccupied()) {
                availableBeds++;
            }
        }
    }

    // 需要至少有1个可用床位才能繁殖
    return availableBeds > 0;
}

bool VillagerBreedGoal::_isWillingToBreed() const
{
    if (!m_villager) return false;

    return m_villager->isWillingToBreed();
}

void VillagerBreedGoal::_findPartner()
{
    if (!m_villager || !m_villager->world()) {
        m_partnerId = 0;
        return;
    }

    m_partnerId = 0;

    // 搜索附近愿意繁殖的村民
    static constexpr f32 PARTNER_SEARCH_RANGE = 8.0f;

    VillagerEntity* partner = EntityUtils::findClosestEntity<VillagerEntity>(
        m_villager->world(), m_villager->position(), PARTNER_SEARCH_RANGE, m_villager, [](VillagerEntity* candidate) {
            if (!candidate || !candidate->isAlive()) return false;

            // 检查对方是否也愿意繁殖
            if (!candidate->isWillingToBreed()) return false;

            // 检查是否是成年村民（不是幼年）
            if (candidate->isChild()) return false;

            return true;
        });

    if (partner) {
        m_partnerId = partner->id();
    }
}

void VillagerBreedGoal::_moveToPartner()
{
    if (!m_villager || m_partnerId == 0) return;

    // 获取配偶实体
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_partnerId) : nullptr;
    if (!entity) {
        m_partnerId = 0;
        return;
    }

    VillagerEntity* partner = dynamic_cast<VillagerEntity*>(entity);
    if (!partner || !partner->isAlive()) {
        m_partnerId = 0;
        return;
    }

    // 移动到配偶位置
    static constexpr f32 BREED_SPEED = 0.5f;
    m_villager->tryMoveTo(partner->x(), partner->y(), partner->z(), BREED_SPEED);
}

void VillagerBreedGoal::_spawnChild()
{
    if (!m_villager) return;

    // MC原版 VillagerMakeLove.tryToGiveBirth: 尝试寻找空床位
    // 如果找不到空床位，双方村民显示愤怒粒子
    if (!_hasEnoughBeds()) {
        // 无空床位，繁殖失败，双方显示愤怒粒子
        if (m_villager->world()) {
            m_villager->world()->broadcastEntityStatus(
                m_villager->id(), static_cast<u8>(network::EntityStatusPacket::Status::VillagerAngry));

            Entity* entity = m_villager->world()->getEntity(m_partnerId);
            if (entity != nullptr && entity->isAlive()) {
                m_villager->world()->broadcastEntityStatus(
                    entity->id(), static_cast<u8>(network::EntityStatusPacket::Status::VillagerAngry));

                // MC原版在tryToGiveBirth之前就对双方调用eatAndDigestFood()消耗食物意愿。
                // 本项目使用布尔标志m_willingToBreed替代食物点数系统，
                // 因此需要同时重置配偶的繁殖意愿，防止配偶立即再次尝试繁殖。
                VillagerEntity* partnerVillager = dynamic_cast<VillagerEntity*>(entity);
                if (partnerVillager != nullptr) {
                    partnerVillager->resetBreedWillingness();
                }
            }
        }
    } else {
        // 生成幼年村民
        auto child = m_villager->createChild();
        if (child && m_villager->world()) {
            child->setPosition(m_villager->x(), m_villager->y(), m_villager->z());
            // spawnEntity 返回服务端分配的 EntityInstanceId，可能不同于移动前的 id
            EntityInstanceId childId = m_villager->world()->spawnEntity(std::move(child));

            // MC原版 VillagerMakeLove.breed: 幼年村民出生后对其广播爱心粒子 (byte)12
            m_villager->world()->broadcastEntityStatus(
                childId, static_cast<u8>(network::EntityStatusPacket::Status::VillagerHeart));
        }
    }

    // 重置
    m_partnerId = 0;
    m_breedTicks = 0;
    m_villager->resetBreedWillingness();
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
