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

/**
 * @file VibrationSystemServer.cpp
 * @brief 振动系统实现 - 依赖服务端的部分
 *
 * 包含 VibrationSystem::User::isValidVibration、Listener 和 Ticker 的实现。
 * 不依赖 ServerWorld 的纯逻辑部分位于 src/common/world/gameevent/VibrationSystem.cpp。
 */

#include "common/world/gameevent/VibrationSystem.hpp"

#include <cmath>
#include <utility>

#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/AvoidVibrationTrigger.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/chunk/load/ChunkLoadLevel.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "server/advancement/TriggerInstantiation.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc::gameevent {
namespace {

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 检查振动信号是否被遮挡（被遮挡方块完全包围）
 *
 * 对振动源方块中心的 6 个方向各偏移微小距离后，
 * 向监听器方块中心发射射线。如果所有 6 个方向的射线都命中了
 * OCCLUDES_VIBRATION_SIGNALS 标签的方块，则振动被遮挡。
 *
 * @param world 世界
 * @param sourcePos 振动源位置（世界坐标）
 * @param listenerPos 监听器位置（世界坐标）
 * @return 如果振动被遮挡返回 true
 */
[[nodiscard]] bool _isOccluded(server::ServerWorld& world, const Vector3d& sourcePos, const Vector3d& listenerPos)
{
    // 将源位置和监听器位置对齐到方块中心
    const Vector3d sourceCenter(
        std::floor(sourcePos.x) + 0.5, std::floor(sourcePos.y) + 0.5, std::floor(sourcePos.z) + 0.5);
    const Vector3d listenerCenter(
        std::floor(listenerPos.x) + 0.5, std::floor(listenerPos.y) + 0.5, std::floor(listenerPos.z) + 0.5);

    // 缓存标签引用，避免每个方向重复查询
    auto& occludesTag = BlockTags::OCCLUDES_VIBRATION_SIGNALS();

    // 检查所有 6 个方向
    for (Direction dir : Directions::all()) {
        // 从源方块中心沿当前方向偏移微小距离（1e-5），避免射线起点正好在方块边界上
        const f64 offsetX = static_cast<f64>(Directions::xOffset(dir)) * 1.0e-5;
        const f64 offsetY = static_cast<f64>(Directions::yOffset(dir)) * 1.0e-5;
        const f64 offsetZ = static_cast<f64>(Directions::zOffset(dir)) * 1.0e-5;

        Vector3d rayStart(sourceCenter.x + offsetX, sourceCenter.y + offsetY, sourceCenter.z + offsetZ);

        // 使用 isBlockInLine 检查从偏移起点到监听器中心的射线上是否有遮挡方块
        bool hitOccludingBlock = world.isBlockInLine(
            rayStart, listenerCenter, [&occludesTag](const BlockState& state) { return occludesTag.contains(state); });

        // 如果某个方向的射线没有命中遮挡方块，说明振动信号可以从这个方向逸出，
        // 因此振动未被完全遮挡
        if (!hitOccludingBlock) {
            return false;
        }
    }

    // 所有 6 个方向的射线都命中了遮挡方块，振动被完全遮挡
    return true;
}

/**
 * @brief 检查监听器位置周围 3x3 区块范围是否全部处于 BlockTicking 级别
 *
 * 检查监听器所在区块及其 8 个相邻区块是否全部满足两个条件：
 * 1. 区块加载级别 <= BlockTicking
 * 2. 区块已在内存中
 *
 * @param world 服务端世界
 * @param pos 监听器位置（方块坐标）
 * @return 如果 3x3 区块全部处于 BlockTicking 级别且已加载返回 true
 */
[[nodiscard]] bool _areAdjacentChunksTicking(server::ServerWorld& world, const BlockPos& pos)
{
    auto* chunkManager = world.chunkManager();
    if (chunkManager == nullptr) {
        return false;
    }

    using mc::world::chunk::ChunkLoadLevel;
    constexpr i32 blockTickingLevel = static_cast<i32>(ChunkLoadLevel::BlockTicking);

    i32 centerChunkX = mc::world::toChunkCoord(pos.x);
    i32 centerChunkZ = mc::world::toChunkCoord(pos.z);

    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            i32 cx = centerChunkX + dx;
            i32 cz = centerChunkZ + dz;

            // 条件1：区块加载级别 <= BlockTicking
            i32 level = chunkManager->ticketManager().getChunkLevel(cx, cz);
            if (level > blockTickingLevel) {
                return false;
            }

            // 条件2：区块已在内存中
            if (!world.hasChunk(cx, cz)) {
                return false;
            }
        }
    }

    return true;
}

} // namespace

// ============================================================================
// VibrationSystem::User - 实现定义在服务端的方法
//
// User 的 vtable 在本 TU 生成（calculateTravelTimeInTicks 与 isValidVibration 均在此定义），
// 避免在 common 库生成 vtable 而引用仅存在于 server 库的 isValidVibration 符号。
// ============================================================================

i32 VibrationSystem::User::calculateTravelTimeInTicks(f32 distance) const
{
    return static_cast<i32>(std::floor(static_cast<f64>(distance)));
}

bool VibrationSystem::User::isValidVibration(const GameEvent& event, const GameEvent::Context& context) const
{
    // 检查事件是否有有效频率
    i32 frequency = getGameEventFrequency(event);
    if (frequency == 0) {
        return false;
    }

    // 检查源实体
    const Entity* sourceEntity = context.sourceEntity();
    if (sourceEntity != nullptr) {
        // 源实体不可为旁观者
        const auto* player = dynamic_cast<const Player*>(sourceEntity);
        if (player != nullptr && player->isSpectator()) {
            return false;
        }

        // 源实体正在潜行且事件可被潜行忽略
        // 当实体潜行时，HIT_GROUND/PROJECTILE_SHOOT/STEP/SWIM/
        // ITEM_INTERACT_START/ITEM_INTERACT_FINISH 不触发振动
        if (sourceEntity->isSteppingCarefully() && isIgnoredBySneaking(event)) {
            // 如果监听器支持规避振动成就触发且源实体是服务端玩家，触发 AVOID_VIBRATION 进度
            if (canTriggerAvoidVibration()) {
                // const_cast 是安全的：触达成就是游戏逻辑副作用，不修改实体状态
                auto* nonConstEntity = const_cast<Entity*>(sourceEntity);
                auto* nonConstPlayer = dynamic_cast<Player*>(nonConstEntity);
                if (nonConstPlayer != nullptr) {
                    auto* serverPlayer = nonConstPlayer->asServerPlayer();
                    if (serverPlayer != nullptr) {
                        auto* advancements = serverPlayer->getAdvancements();
                        if (advancements != nullptr) {
                            auto* trigger = advancement::CriterionTriggers::instance()
                                                .getTrigger<advancement::AvoidVibrationTrigger>();
                            if (trigger != nullptr) {
                                // 使用基类的 trigger 模板方法，匹配所有监听此触发器的进度实例
                                trigger->AbstractCriterionTrigger<advancement::AvoidVibrationTriggerInstance>::trigger(
                                    *advancements, [](const advancement::AvoidVibrationTriggerInstance& /*instance*/) {
                                        return true;
                                    });
                            }
                        }
                    }
                }
            }
            return false;
        }

        // 源实体阻尼振动（如监守者、羊毛物品）
        if (sourceEntity->dampensVibrations()) {
            return false;
        }
    }

    // 受影响方块阻尼振动（如羊毛方块/地毯）
    if (context.affectedState() != nullptr && BlockTags::DAMPENS_VIBRATIONS().contains(*context.affectedState())) {
        return false;
    }

    return true;
}

// ============================================================================
// VibrationSystem::Listener
// ============================================================================

bool VibrationSystem::Listener::handleGameEvent(
    server::ServerWorld& world, const GameEvent& event, const GameEvent::Context& context, const Vector3d& pos)
{
    Data& data = m_system.getVibrationData();
    User& user = m_system.getVibrationUser();

    // 已有正在传播的振动，拒绝新振动
    if (data.currentVibration() != nullptr) {
        return false;
    }

    // 验证振动是否有效
    if (!user.isValidVibration(event, context)) {
        return false;
    }

    // 获取监听器位置
    auto listenerPos = user.getPositionSource().getPosition(world);
    if (!listenerPos.has_value()) {
        return false;
    }

    // 验证是否可以接收此振动
    BlockPos sourceBlockPos(
        static_cast<i32>(std::floor(pos.x)), static_cast<i32>(std::floor(pos.y)), static_cast<i32>(std::floor(pos.z)));
    if (!user.canReceiveVibration(world, sourceBlockPos, event, context)) {
        return false;
    }

    // 检查振动信号是否被遮挡方块阻挡
    // 如果振动源被 OCCLUDES_VIBRATION_SIGNALS 方块从所有6个方向完全包围，
    // 则振动信号无法传播到监听器
    if (_isOccluded(world, pos, listenerPos.value())) {
        return false;
    }

    // 调度振动
    scheduleVibration(world, event, context, pos, listenerPos.value());
    return true;
}

void VibrationSystem::Listener::forceScheduleVibration(
    server::ServerWorld& world, const GameEvent& event, const GameEvent::Context& context, const Vector3d& pos)
{
    User& user = m_system.getVibrationUser();
    auto listenerPos = user.getPositionSource().getPosition(world);
    if (listenerPos.has_value()) {
        scheduleVibration(world, event, context, pos, listenerPos.value());
    }
}

void VibrationSystem::Listener::scheduleVibration(server::ServerWorld& world,
    const GameEvent& event,
    const GameEvent::Context& context,
    const Vector3d& pos,
    const Vector3d& listenerPos)
{
    Data& data = m_system.getVibrationData();

    f32 distance = static_cast<f32>(pos.distance(listenerPos));
    const Entity* sourceEntity = context.sourceEntity();

    VibrationInfo info(event, distance, pos, sourceEntity);
    u64 gameTick = world.currentTick();

    data.selectionStrategy().addCandidate(std::move(info), gameTick);
}

// ============================================================================
// VibrationSystem::Ticker
// ============================================================================

void VibrationSystem::Ticker::tick(server::ServerWorld& world, Data& data, User& user)
{
    // 区块重新加载后重发振动粒子
    tryReloadVibrationParticle(world, data, user);

    // 如果没有当前振动，尝试选择候选
    if (data.currentVibration() == nullptr) {
        trySelectAndScheduleVibration(world, data, user);
    }

    // 如果有当前振动，处理传播
    if (data.currentVibration() != nullptr) {
        bool wasTraveling = data.travelTimeInTicks() > 0;

        // 递减传播时间
        data.decrementTravelTime();

        // 传播完成
        if (data.travelTimeInTicks() <= 0) {
            (void)receiveVibration(world, data, user, *data.currentVibration());
        }

        if (wasTraveling) {
            user.onDataChanged();
        }
    }
}

void VibrationSystem::Ticker::trySelectAndScheduleVibration(server::ServerWorld& world, Data& data, User& user)
{
    u64 currentTick = world.currentTick();

    auto candidate = data.selectionStrategy().chosenCandidate(currentTick);
    if (candidate.has_value()) {
        VibrationInfo& info = candidate.value();
        data.setCurrentVibration(info);

        // 计算传播时间
        i32 travelTime = user.calculateTravelTimeInTicks(info.distance);
        data.setTravelTimeInTicks(travelTime);

        // 发送振动粒子效果，从振动源位置飞向监听器位置
        // 粒子类型为 Vibration，携带目标位置来源和到达时间
        Vector3 particlePos(static_cast<f32>(info.pos.x), static_cast<f32>(info.pos.y), static_cast<f32>(info.pos.z));
        auto listenerPosOpt = user.getPositionSource().getPosition(world);
        if (listenerPosOpt.has_value()) {
            world.addVibrationParticle(particlePos, user.getPositionSource(), travelTime);
        } else {
            // 无法获取监听器位置时，使用简化的静态粒子效果（向后兼容）
            world.addParticle(particle::ParticleTypeId::Vibration,
                particlePos,
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(0.0f, 0.0f, 0.0f),
                1);
        }

        user.onDataChanged();
        data.selectionStrategy().startOver();
    }
}

bool VibrationSystem::Ticker::receiveVibration(
    server::ServerWorld& world, Data& data, User& user, const VibrationInfo& info)
{
    if (info.gameEvent == nullptr) {
        data.clearCurrentVibration();
        return false;
    }

    BlockPos sourceBlockPos(static_cast<i32>(std::floor(info.pos.x)),
        static_cast<i32>(std::floor(info.pos.y)),
        static_cast<i32>(std::floor(info.pos.z)));

    // 获取监听器位置
    auto listenerPos = user.getPositionSource().getPosition(world);
    BlockPos listenerBlockPos = listenerPos.has_value() ? BlockPos(static_cast<i32>(std::floor(listenerPos->x)),
                                                              static_cast<i32>(std::floor(listenerPos->y)),
                                                              static_cast<i32>(std::floor(listenerPos->z)))
                                                        : sourceBlockPos;

    if (user.requiresAdjacentChunksToBeTicking()) {
        auto* chunkManager = world.chunkManager();
        if (chunkManager != nullptr && !_areAdjacentChunksTicking(world, listenerBlockPos)) {
            // 监听器周围 3x3 区块未全部处于 BlockTicking 级别，暂不接收振动（下次 tick 重试）
            return false;
        }
    }

    // 查找源实体
    const Entity* sourceEntity = nullptr;
    if (info.hasSourceEntity) {
        sourceEntity = world.getEntity(info.sourceEntityId);
    }

    // 触发接收回调
    user.onReceiveVibration(world, sourceBlockPos, *info.gameEvent, sourceEntity, info.distance);

    // 清除当前振动
    data.clearCurrentVibration();
    return true;
}

void VibrationSystem::Ticker::tryReloadVibrationParticle(server::ServerWorld& world, Data& data, User& user)
{
    if (!data.shouldReloadVibrationParticle()) {
        return;
    }

    // 如果没有正在传播的振动，清除重载标志
    if (data.currentVibration() == nullptr) {
        data.setReloadVibrationParticle(false);
        return;
    }

    const VibrationInfo& info = *data.currentVibration();

    // 获取监听器位置
    auto listenerPos = user.getPositionSource().getPosition(world);
    if (!listenerPos.has_value()) {
        return;
    }

    // 计算当前传播进度：已传播时间 / 总传播时间
    i32 remainingTicks = data.travelTimeInTicks();
    i32 totalTicks = user.calculateTravelTimeInTicks(info.distance);
    if (totalTicks <= 0) {
        data.setReloadVibrationParticle(false);
        return;
    }

    // 从振动源位置到监听器位置插值计算粒子当前位置
    f64 progress = 1.0 - static_cast<f64>(remainingTicks) / static_cast<f64>(totalTicks);
    f32 particleX = static_cast<f32>(info.pos.x + (listenerPos->x - info.pos.x) * progress);
    f32 particleY = static_cast<f32>(info.pos.y + (listenerPos->y - info.pos.y) * progress);
    f32 particleZ = static_cast<f32>(info.pos.z + (listenerPos->z - info.pos.z) * progress);

    // 在插值位置发送振动粒子，携带监听器目标位置来源和剩余传播时间
    Vector3 particlePos(particleX, particleY, particleZ);
    world.addVibrationParticle(particlePos, user.getPositionSource(), remainingTicks);

    // 重载完成，清除标志
    data.setReloadVibrationParticle(false);
}

} // namespace mc::gameevent
