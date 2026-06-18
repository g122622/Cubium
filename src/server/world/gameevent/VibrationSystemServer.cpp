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

#include "common/entity/entities/player/Player.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/chunk/load/ChunkLoadLevel.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc::gameevent {

// ============================================================================
// VibrationSystem::User - 依赖服务端的方法
// ============================================================================

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
        // 参考: net.minecraft.tags.GameEventTags.IGNORE_VIBRATIONS_SNEAKING
        // 当实体潜行时，HIT_GROUND/PROJECTILE_SHOOT/STEP/SWIM/
        // ITEM_INTERACT_START/ITEM_INTERACT_FINISH 不触发振动
        if (sourceEntity->isSteppingCarefully() && isIgnoredBySneaking(event)) {
            // 如果监听器支持规避振动成就触发且源实体是服务端玩家，触发 AVOID_VIBRATION 进度
            // TODO: 当 AVOID_VIBRATION 成就触发器实现后，在此触发 ServerPlayer 进度
            return false;
        }

        // 源实体阻尼振动（如监守者、羊毛物品）
        if (sourceEntity->dampensVibrations()) {
            return false;
        }
    }

    // 受影响方块阻尼振动（如羊毛方块/地毯）
    // 参考: net.minecraft.tags.BlockTags.DAMPENS_VIBRATIONS
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

        // TODO: 当粒子系统实现后，发送振动粒子效果
        // world.sendParticles(VibrationParticleOption(user.getPositionSource(), travelTime),
        //     info.pos.x, info.pos.y, info.pos.z, 1, 0, 0, 0, 0);

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

    // TODO: 当前仅检查源区块本身的加载级别是否为 EntityTicking，
    // MC 原版检查的是源位置周围 3x3 区块范围是否均在 EntityTicking 级别，
    // 未来需扩展为遍历 (chunkX-1..chunkX+1, chunkZ-1..chunkZ+1) 全部9个区块进行检查，
    // 以防止振动穿透区块边界的问题。参见 README 中"容易踩的坑"第4点。
    if (user.requiresAdjacentChunksToBeTicking()) {
        auto* chunkManager = world.chunkManager();
        if (chunkManager != nullptr) {
            using mc::world::chunk::ChunkLoadLevel;
            i32 chunkX = mc::world::toChunkCoord(sourceBlockPos.x);
            i32 chunkZ = mc::world::toChunkCoord(sourceBlockPos.z);
            i32 level = chunkManager->ticketManager().getChunkLevel(chunkX, chunkZ);
            if (level > static_cast<i32>(ChunkLoadLevel::EntityTicking)) {
                // 源区块不在 EntityTicking 级别，拒绝振动
                data.clearCurrentVibration();
                return false;
            }
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

} // namespace mc::gameevent
