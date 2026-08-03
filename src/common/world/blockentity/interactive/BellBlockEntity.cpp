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

#include "world/blockentity/interactive/BellBlockEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/brain/Brain.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/effect/EffectType.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

// ============================================================================
// 构造与析构
// ============================================================================

BellBlockEntity::BellBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Bell, pos)
    , m_clickDirection(Direction::North)
{}

BellBlockEntity::~BellBlockEntity() noexcept = default;

// ============================================================================
// 敲击钟
// ============================================================================

void BellBlockEntity::onHit(IWorld& world, Direction direction)
{
    m_clickDirection = direction;

    if (m_shaking) {
        // 已经在摇晃，重置计数
        m_ticks = 0;
    } else {
        m_shaking = true;
    }

    // 通过 blockEvent 同步动画到客户端
    // 事件 ID=1，事件数据=Direction 的 3D 数据值
    const BlockState* state = world.getBlockState(m_pos);
    if (state != nullptr) {
        world.blockEvent(m_pos, state->getBlock(), 1, static_cast<i32>(direction));
    }
}

// ============================================================================
// Tick 更新
// ============================================================================

void BellBlockEntity::tick(IWorld& world)
{
    // 摇晃计时
    if (m_shaking) {
        ++m_ticks;
    }

    // 摇晃动画结束
    if (m_ticks >= DURATION) {
        m_shaking = false;
        m_ticks = 0;
    }

    // 检测共振触发条件：摇晃达到 TICKS_BEFORE_RESONATION 且未共振且附近有灾厄村民
    if (m_ticks >= TICKS_BEFORE_RESONATION && m_resonationTicks == 0 && _areRaidersNearby()) {
        m_resonating = true;
        world.playSound(SoundEvents::BLOCK_BELL_RESONATE, sound::SoundCategory::Blocks, m_pos.center(), 1.0f, 1.0f);
    }

    // 共振计时与到期处理
    if (m_resonating) {
        if (m_resonationTicks < MAX_RESONATION_TICKS) {
            ++m_resonationTicks;
        } else {
            // 共振到期：执行端动作并退出共振状态
            if (world.isClientSide()) {
                _showBellParticles(world);
            } else {
                _makeRaidersGlow(world);
            }
            m_resonating = false;
        }
    }
}

bool BellBlockEntity::needsTick() const noexcept
{
    // 仅在摇晃或共振时需要 tick
    return m_shaking || m_resonating;
}

// ============================================================================
// 客户端方块事件
// ============================================================================

bool BellBlockEntity::triggerEvent(i32 id, i32 type)
{
    if (id == 1) {
        // 敲击事件：重新搜索附近实体、重置共振、设置敲击方向、启动摇晃
        if (m_world != nullptr) {
            _updateEntities(*m_world);
        }
        m_resonationTicks = 0;
        // Direction 枚举值与 MC Java 的 get3DDataValue 完全一致：
        // Down=0, Up=1, North=2, South=3, West=4, East=5
        m_clickDirection = static_cast<Direction>(type);
        m_ticks = 0;
        m_shaking = true;
        return true;
    }
    return false;
}

// ============================================================================
// 序列化
// ============================================================================

bool BellBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 空 JSON（默认构造或 null）使用默认值
    if (data.is_null() || !data.is_object()) {
        m_nearbyEntities.clear();
        return true;
    }

    m_ticks = data.value("ticks", 0);
    m_shaking = data.value("shaking", false);
    m_resonating = data.value("resonating", false);
    m_resonationTicks = data.value("resonation_ticks", 0);
    m_lastRingTimestamp = data.value("last_ring_timestamp", static_cast<i64>(0));

    if (data.contains("click_direction") && data["click_direction"].is_number_integer()) {
        const i32 raw = data["click_direction"].get<i32>();
        if (raw >= 0 && raw <= 5) {
            m_clickDirection = static_cast<Direction>(raw);
        }
    }

    // nearbyEntities 不需要序列化，下次 tick 会重新搜索
    m_nearbyEntities.clear();

    return true;
}

void BellBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["ticks"] = m_ticks;
    data["shaking"] = m_shaking;
    data["resonating"] = m_resonating;
    data["resonation_ticks"] = m_resonationTicks;
    data["last_ring_timestamp"] = m_lastRingTimestamp;
    data["click_direction"] = static_cast<i32>(m_clickDirection);
}

std::unique_ptr<BlockEntity> BellBlockEntity::clone() const
{
    auto cloned = std::make_unique<BellBlockEntity>(m_pos);
    cloned->m_ticks = m_ticks;
    cloned->m_shaking = m_shaking;
    cloned->m_clickDirection = m_clickDirection;
    cloned->m_resonating = m_resonating;
    cloned->m_resonationTicks = m_resonationTicks;
    cloned->m_lastRingTimestamp = m_lastRingTimestamp;
    // nearbyEntities 不克隆，新实体不在世界中
    return cloned;
}

// ============================================================================
// 内部实现
// ============================================================================

void BellBlockEntity::_updateEntities(IWorld& world)
{
    const i64 currentGameTime = static_cast<i64>(world.getGameTime());

    // 距离上次搜索超过 MIN_TICKS_BETWEEN_SEARCHES tick 时重新搜索
    if (currentGameTime > m_lastRingTimestamp + MIN_TICKS_BETWEEN_SEARCHES || m_nearbyEntities.empty()) {
        m_lastRingTimestamp = currentGameTime;

        // 以钟为中心、半径 SEARCH_RADIUS 的 AABB
        const AxisAlignedBB searchBox = AxisAlignedBB::fromBlock(m_pos.x, m_pos.y, m_pos.z).grow(SEARCH_RADIUS);
        const std::vector<Entity*> entities = world.getEntitiesInAABB(searchBox, nullptr);

        // 筛选 LivingEntity
        m_nearbyEntities.clear();
        m_nearbyEntities.reserve(entities.size());
        for (Entity* entity : entities) {
            if (entity == nullptr) {
                continue;
            }
            auto* living = dynamic_cast<LivingEntity*>(entity);
            if (living != nullptr) {
                m_nearbyEntities.push_back(living);
            }
        }
    }

    // 仅服务端：给范围内的村民写入 HEARD_BELL_TIME 记忆
    if (!world.isClientSide()) {
        const Vector3 center = m_pos.center();
        const f32 hearRadiusSq = HEAR_BELL_RADIUS * HEAR_BELL_RADIUS;

        for (LivingEntity* living : m_nearbyEntities) {
            if (living == nullptr || !living->isAlive()) {
                continue;
            }

            // 距离检查（closerToCenterThan）
            const Vector3 entityPos = living->position();
            const f32 dx = entityPos.x - center.x;
            const f32 dy = entityPos.y - center.y;
            const f32 dz = entityPos.z - center.z;
            const f32 distSq = dx * dx + dy * dy + dz * dz;
            if (distSq > hearRadiusSq) {
                continue;
            }

            // 仅村民有 Brain 和 HEARD_BELL_TIME 记忆
            auto* villager = dynamic_cast<entity::VillagerEntity*>(living);
            if (villager == nullptr) {
                continue;
            }

            villager->brain().setMemory<i64>(
                entity::ai::brain::memory::MemoryModuleTypes::HEARD_BELL_TIME, currentGameTime);
        }
    }
}

bool BellBlockEntity::_areRaidersNearby() const
{
    const Vector3 center = m_pos.center();
    const f32 hearRadiusSq = HEAR_BELL_RADIUS * HEAR_BELL_RADIUS;

    for (LivingEntity* living : m_nearbyEntities) {
        if (living == nullptr || !living->isAlive()) {
            continue;
        }

        // 距离检查
        const Vector3 entityPos = living->position();
        const f32 dx = entityPos.x - center.x;
        const f32 dy = entityPos.y - center.y;
        const f32 dz = entityPos.z - center.z;
        const f32 distSq = dx * dx + dy * dy + dz * dz;
        if (distSq > hearRadiusSq) {
            continue;
        }

        // RAIDERS 标签检查
        if (EntityTypeTags::isInitialized()) {
            const std::string& typeId = living->getTypeId();
            if (EntityTypeTags::RAIDERS().contains(typeId)) {
                return true;
            }
        }
    }

    return false;
}

void BellBlockEntity::_makeRaidersGlow(IWorld& world)
{
    MC_UNUSED(world);

    for (LivingEntity* living : m_nearbyEntities) {
        if (living == nullptr) {
            continue;
        }
        if (_isRaiderWithinRange(*living)) {
            _glow(*living);
        }
    }
}

void BellBlockEntity::_showBellParticles(IWorld& world)
{
    // 对应 MC Java 原版 BellBlockEntity.showBellParticles：
    //   MutableInt mutableint = new MutableInt(16700985);
    //   int i = nearbyEntities.filter(closerThan 48).count();
    //   for each raider within range:
    //       int j = Mth.clamp((i - 21) / -2, 3, 15);
    //       double d1 = pos.x + 0.5 + (1/dist) * (entity.x - pos.x);
    //       double d2 = pos.z + 0.5 + (1/dist) * (entity.z - pos.z);
    //       for (int k = 0; k < j; k++) {
    //           int l = mutableint.addAndGet(5);
    //           world.addParticle(ColorParticleOption.create(ENTITY_EFFECT, l),
    //                             d1, pos.y + 0.5, d2, 0, 0, 0);
    //       }
    const Vector3 center = m_pos.center();
    const f32 highlightRadiusSq = HIGHLIGHT_RAIDERS_RADIUS * HIGHLIGHT_RAIDERS_RADIUS;

    // 统计 48 格内的实体数量（参考 MC 原版 i = nearbyEntities.filter(closerThan 48).count）
    i32 nearbyCount = 0;
    for (LivingEntity* living : m_nearbyEntities) {
        if (living == nullptr) {
            continue;
        }
        const Vector3 entityPos = living->position();
        const f32 dx = entityPos.x - center.x;
        const f32 dy = entityPos.y - center.y;
        const f32 dz = entityPos.z - center.z;
        const f32 distSq = dx * dx + dy * dy + dz * dz;
        if (distSq <= highlightRadiusSq) {
            ++nearbyCount;
        }
    }

    // 统计范围内的灾厄村民数量，若无则直接返回
    bool hasRaider = false;
    for (LivingEntity* living : m_nearbyEntities) {
        if (living != nullptr && _isRaiderWithinRange(*living)) {
            hasRaider = true;
            break;
        }
    }
    if (!hasRaider) {
        return;
    }

    // 粒子数量（参考 MC 原版 j = Mth.clamp((i - 21) / -2, 3, 15)）
    const i32 particleCount = std::max(3, std::min(15, (nearbyCount - 21) / -2));

    // 颜色计数器：初始 16700985，每个粒子发射前递增 5（对应 MC 原版 MutableInt.addAndGet(5)）
    i32 colorCounter = 16700985;

    for (LivingEntity* living : m_nearbyEntities) {
        if (living == nullptr || !_isRaiderWithinRange(*living)) {
            continue;
        }

        const Vector3 entityPos = living->position();
        const f32 dx = entityPos.x - static_cast<f32>(m_pos.x);
        const f32 dz = entityPos.z - static_cast<f32>(m_pos.z);
        const f32 dist = std::sqrt(dx * dx + dz * dz);
        if (dist < 0.001f) {
            continue;
        }

        // 计算粒子发射位置（参考 MC 原版公式）
        const f32 d1 = static_cast<f32>(m_pos.x) + 0.5f + (1.0f / dist) * dx;
        const f32 d2 = static_cast<f32>(m_pos.z) + 0.5f + (1.0f / dist) * dz;
        const Vector3 particlePos(d1, static_cast<f32>(m_pos.y) + 0.5f, d2);

        // 为每个粒子生成递增颜色（addAndGet 语义：先加后返回）
        for (i32 k = 0; k < particleCount; ++k) {
            colorCounter += 5;
            const u32 color = static_cast<u32>(colorCounter);
            // 通过 IWorld::addEntityEffectParticle 走粒子数据管线
            // 客户端：经 ClientWorld 创建带 EntityEffectParticleData 的粒子
            // 服务端：经 ServerWorld 广播给附近玩家
            world.addEntityEffectParticle(particlePos, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), 1, color);
        }
    }
}

bool BellBlockEntity::_isRaiderWithinRange(const LivingEntity& entity) const
{
    if (!entity.isAlive()) {
        return false;
    }

    // 距离检查（HIGHLIGHT_RAIDERS_RADIUS）
    const Vector3 center = m_pos.center();
    const Vector3 entityPos = entity.position();
    const f32 dx = entityPos.x - center.x;
    const f32 dy = entityPos.y - center.y;
    const f32 dz = entityPos.z - center.z;
    const f32 distSq = dx * dx + dy * dy + dz * dz;
    const f32 highlightRadiusSq = HIGHLIGHT_RAIDERS_RADIUS * HIGHLIGHT_RAIDERS_RADIUS;
    if (distSq > highlightRadiusSq) {
        return false;
    }

    // RAIDERS 标签检查
    if (!EntityTypeTags::isInitialized()) {
        return false;
    }
    const std::string& typeId = entity.getTypeId();
    return EntityTypeTags::RAIDERS().contains(typeId);
}

void BellBlockEntity::_glow(LivingEntity& entity)
{
    const entity::effect::EffectInstance glowEffect(
        entity::effect::EffectType::Glowing, GLOW_DURATION, 0, false, true, true);
    entity.addEffect(glowEffect);
}

} // namespace blockentity
} // namespace mc
