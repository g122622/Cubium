#include "MiningManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "common/world/block/Block.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include <spdlog/spdlog.h>
#include "common/perfetto/TraceEvents.hpp"

namespace mc::server::interaction {

MiningManager::MiningManager(core::PlayerManager& playerManager,
                             core::ConnectionManager& connectionManager)
    : m_playerManager(playerManager)
    , m_connectionManager(connectionManager)
{
}

void MiningManager::startMining(PlayerId playerId, const BlockPos& pos, EntityId entityId)
{
    auto& state = m_miningStates[playerId];
    state.position = pos;
    state.progress = 0.0f;
    state.lastStage = 255;
    state.active = true;
    state.startTick = 0;  // 将在 tick 中设置
    state.breakerId = entityId;

    spdlog::debug("[Mining] Player {} started mining at ({}, {}, {})",
                  playerId, pos.x, pos.y, pos.z);
}

void MiningManager::abortMining(PlayerId playerId)
{
    auto it = m_miningStates.find(playerId);
    if (it != m_miningStates.end()) {
        if (it->second.active) {
            spdlog::debug("[Mining] Player {} aborted mining at ({}, {}, {})",
                          playerId, it->second.position.x, it->second.position.y, it->second.position.z);
        }
        m_miningStates.erase(it);
    }
}

void MiningManager::handleBlockInteraction(PlayerId playerId, const BlockPos& pos,
                                           network::BlockInteractionAction action)
{
    switch (action) {
        case network::BlockInteractionAction::StartDestroyBlock:
            startMining(playerId, pos, static_cast<EntityId>(playerId));
            break;

        case network::BlockInteractionAction::AbortDestroyBlock:
            abortMining(playerId);
            break;

        case network::BlockInteractionAction::StopDestroyBlock:
            // 停止挖掘状态
            abortMining(playerId);
            break;
    }
}

void MiningManager::tick(ServerWorld& world)
{
    MC_TRACE_EVENT("server.world.mining", "MiningManager::tick", "phase", "tick");

    for (auto& [playerId, state] : m_miningStates) {
        if (!state.active) {
            continue;
        }

        // 检查玩家是否仍然有效
        auto* playerData = m_playerManager.getPlayer(playerId);
        if (!playerData || !playerData->loggedIn) {
            state.active = false;
            continue;
        }

        // 检查方块是否仍然存在
        const BlockState* blockState = world.getBlockState(
            state.position.x, state.position.y, state.position.z);

        if (!blockState || blockState->isAir()) {
            // 方块已被破坏（可能是其他玩家）
            state.active = false;
            continue;
        }

        // 计算挖掘速度
        f32 speed = calculateMiningSpeed(world, state.position, playerId);
        state.progress += speed;

        // 计算动画阶段 (0-9)
        u8 stage = static_cast<u8>(std::min(state.progress * 10.0f, 9.0f));

        // 广播动画（阶段变化时）
        if (stage != state.lastStage) {
            broadcastBreakAnim(playerId, state.position, static_cast<i8>(stage));
            state.lastStage = stage;
        }

        // 检查是否完成
        if (state.progress >= 1.0f) {
            // 调用挖掘完成回调
            if (m_onMiningComplete) {
                m_onMiningComplete(playerId, state.position);
            }
            state.active = false;
        }
    }
}

f32 MiningManager::getMiningProgress(PlayerId playerId) const
{
    auto it = m_miningStates.find(playerId);
    if (it != m_miningStates.end() && it->second.active) {
        return it->second.progress;
    }
    return 0.0f;
}

bool MiningManager::isMining(PlayerId playerId) const
{
    auto it = m_miningStates.find(playerId);
    return it != m_miningStates.end() && it->second.active;
}

std::optional<BlockPos> MiningManager::getMiningPosition(PlayerId playerId) const
{
    auto it = m_miningStates.find(playerId);
    if (it != m_miningStates.end() && it->second.active) {
        return it->second.position;
    }
    return std::nullopt;
}

void MiningManager::setOnBreakAnimBroadcast(
    std::function<void(PlayerId, i32, i32, i32, i8)> callback)
{
    m_onBreakAnimBroadcast = std::move(callback);
}

void MiningManager::setOnMiningComplete(
    std::function<void(PlayerId, const BlockPos&)> callback)
{
    m_onMiningComplete = std::move(callback);
}

f32 MiningManager::calculateMiningSpeed(ServerWorld& world,
                                         const BlockPos& pos,
                                         PlayerId playerId) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state || state->isAir()) {
        return 1.0f;  // 方块不存在，快速完成
    }

    // 获取方块硬度
    f32 hardness = state->hardness();
    if (hardness < 0.0f) {
        return 0.0f;  // 不可破坏
    }

    // TODO: 考虑工具、附魔、药水效果等
    // 简化版本：基础速度 = 1 / (hardness * 30)
    // 创造模式：更快
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (playerData && playerData->gameMode == GameMode::Creative) {
        return 1.0f;  // 创造模式瞬间破坏
    }

    if (hardness == 0.0f) {
        return 1.0f;  // 瞬间破坏
    }

    // 基础挖掘速度
    f32 baseSpeed = 1.0f / (hardness * 30.0f);
    return baseSpeed;
}

void MiningManager::broadcastBreakAnim(PlayerId playerId, const BlockPos& pos, i8 stage)
{
    if (m_onBreakAnimBroadcast) {
        m_onBreakAnimBroadcast(playerId, pos.x, pos.y, pos.z, stage);
    }
}

} // namespace mc::server::interaction