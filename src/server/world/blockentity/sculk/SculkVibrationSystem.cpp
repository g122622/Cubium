/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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
 * @file SculkVibrationSystem.cpp
 * @brief 幽匿方块实体的振动系统附件实现
 *
 * 实现 SculkSensorVibrationUser、SculkShriekerVibrationUser 的
 * canReceiveVibration/onReceiveVibration 方法，以及
 * SculkVibrationManager 的注册/注销/tick 逻辑。
 */

#include "SculkVibrationSystem.hpp"
#include "SculkShriekerHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/core/CoordConverter.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/sculk/SculkBlocks.hpp"
#include "common/world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "common/world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/gameevent/GameEventListener.hpp"
#include "common/world/gameevent/GameEventListenerRegistry.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <memory>
#include <utility>

namespace mc::server {

using mc::util::core::CoordConverter;

// ============================================================================
// SculkSensorVibrationUser
// ============================================================================

bool SculkSensorVibrationUser::canReceiveVibration(ServerWorld& world,
    const BlockPos& pos,
    const gameevent::GameEvent& event,
    const gameevent::GameEvent::Context& context) const
{
    // 基本验证：事件频率不能为0
    i32 frequency = gameevent::VibrationSystem::getGameEventFrequency(event);
    if (frequency == 0) {
        return false;
    }

    // 潜行实体产生的事件可被忽略
    if (gameevent::VibrationSystem::isIgnoredBySneaking(event)) {
        if (context.sourceEntity() != nullptr && context.sourceEntity()->isSteppingCarefully()) {
            return false;
        }
    }

    // 只有当前 Phase 为 Inactive 时才能接收振动
    // 活跃或冷却期间不能再次被激活
    // 注意：这里必须读监听器自身位置（m_entity.getPos()，即感测体方块位置）的方块状态，
    // 而非振动源位置 pos（pos 是发事件方块位置，如炼药锅，其方块状态没有 sculk_sensor_phase 属性，
    // 调 canActivate 会触发 StateHolder::get 抛 std::invalid_argument）。
    const BlockPos sensorPos = m_entity.getPos();
    const BlockState* state = world.getBlockState(sensorPos);
    if (state == nullptr) {
        return false;
    }
    if (!blocks::SculkSensorBlock::canActivate(*state)) {
        return false;
    }

    // 拒绝来自自身位置的 BLOCK_DESTROY/BLOCK_PLACE 事件
    // （防止感测体被放置/破坏时自己激活自己，但允许来自其他位置的同类事件）
    if (&event == &gameevent::GameEvents::BLOCK_DESTROY || &event == &gameevent::GameEvents::BLOCK_PLACE) {
        if (pos == m_entity.getPos()) {
            return false;
        }
    }

    return true;
}

void SculkSensorVibrationUser::onReceiveVibration(ServerWorld& world,
    const BlockPos& pos,
    const gameevent::GameEvent& event,
    const Entity* sourceEntity,
    f32 distance)
{
    MC_UNUSED(pos); // pos 是振动源位置；激活读感测体自身位置（m_entity.getPos）

    // 更新最后振动频率
    i32 frequency = gameevent::VibrationSystem::getGameEventFrequency(event);
    m_entity.setLastVibrationFrequency(frequency);

    // 标记方块实体已修改（需要保存）
    m_entity.setChanged();

    // 获取监听器自身位置（感测体方块位置）的方块状态。
    // 注意：不能用振动源位置 pos（pos 是发事件方块位置，如炼药锅，其方块状态没有
    // sculk_sensor_phase/power 属性，activate 内 state.with 会抛 std::invalid_argument）。
    const BlockPos sensorPos = m_entity.getPos();
    const BlockState* state = world.getBlockState(sensorPos);
    if (state == nullptr) {
        return;
    }

    // 根据振动距离计算红石信号强度 (1-15)
    i32 redstoneStrength = gameevent::VibrationSystem::getRedstoneStrengthForDistance(distance, getListenerRadius());

    // 激活幽匿感测体：设置 Active 状态、红石信号、调度 tick、通知邻居、触发共振
    blocks::SculkSensorBlock::activate(sourceEntity, world, sensorPos, *state, redstoneStrength, frequency);
}

// ============================================================================
// SculkShriekerVibrationUser
// ============================================================================

bool SculkShriekerVibrationUser::canReceiveVibration(ServerWorld& world,
    const BlockPos& pos,
    const gameevent::GameEvent& event,
    const gameevent::GameEvent::Context& context) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 幽匿尖啸体只响应 SHRIEK 事件（来自其他尖啸体）
    if (&event != &gameevent::GameEvents::SHRIEK) {
        return false;
    }

    // 潜行实体产生的事件可被忽略
    if (context.sourceEntity() != nullptr && context.sourceEntity()->isSteppingCarefully()) {
        return false;
    }

    return true;
}

void SculkShriekerVibrationUser::onReceiveVibration(ServerWorld& world,
    const BlockPos& pos,
    const gameevent::GameEvent& event,
    const Entity* sourceEntity,
    f32 distance)
{
    MC_UNUSED(pos); // pos 是振动源位置；tryShriek 操作尖啸体自身位置（m_entity.getPos）
    MC_UNUSED(event);
    MC_UNUSED(distance);

    // 振动到达时，触发尖啸体的 tryShriek 逻辑
    // tryShriek 内部会检查 SHRIEKING 状态、解析玩家、检查条件、递增警告等级、播放效果
    // 注意：tryShriek 的 pos 必须是尖啸体自身位置（监听器位置），而非振动源位置。
    SculkShriekerHelper::tryShriek(world, m_entity.getPos(), sourceEntity);

    // 标记方块实体已修改（需要保存）
    m_entity.setChanged();
}

// ============================================================================
// SculkVibrationManager
// ============================================================================

void SculkVibrationManager::registerSculkSensor(blockentity::SculkSensorBlockEntity& entity)
{
    const BlockPos& pos = entity.getPos();

    // 避免重复注册
    if (m_vibrationSystems.find(pos) != m_vibrationSystems.end()) {
        return;
    }

    // 创建振动系统附件
    auto system = std::make_unique<SculkVibrationSystem>(entity);
    auto& listener = system->getVibrationListener();

    // 注册 Listener 到区块的 GameEventListenerRegistry
    registerListenerInChunk(pos, listener);

    m_vibrationSystems.emplace(pos, std::move(system));
}

void SculkVibrationManager::registerSculkShrieker(blockentity::SculkShriekerBlockEntity& entity)
{
    const BlockPos& pos = entity.getPos();

    // 避免重复注册
    if (m_vibrationSystems.find(pos) != m_vibrationSystems.end()) {
        return;
    }

    // 创建振动系统附件
    auto system = std::make_unique<SculkVibrationSystem>(entity);
    auto& listener = system->getVibrationListener();

    // 注册 Listener 到区块的 GameEventListenerRegistry
    registerListenerInChunk(pos, listener);

    m_vibrationSystems.emplace(pos, std::move(system));
}

void SculkVibrationManager::unregisterSculkBlockEntity(const BlockPos& pos)
{
    auto it = m_vibrationSystems.find(pos);
    if (it == m_vibrationSystems.end()) {
        return;
    }

    auto& system = it->second;
    auto& listener = system->getVibrationListener();

    // 如果是尖啸体且尖啸已结束，在注销前先处理响应逻辑
    // 否则注销后 tickAll 不再遍历此尖啸体，tryRespond 永远不会被调用
    if (system->getVibrationUser().isSculkShrieker()) {
        SculkShriekerHelper::checkShriekingFinished(*m_world, pos);
    }

    // 从区块的 GameEventListenerRegistry 注销 Listener
    unregisterListenerFromChunk(pos, listener);

    m_vibrationSystems.erase(it);
}

void SculkVibrationManager::tickAll()
{
    for (auto& [pos, system] : m_vibrationSystems) {
        gameevent::VibrationSystem::Ticker::tick(*m_world, system->getVibrationData(), system->getVibrationUser());
    }

    // 检查幽匿尖啸体的 SHRIEKING 结束标志
    for (auto& [pos, system] : m_vibrationSystems) {
        // 仅检查尖啸体（感测体没有 SHRIEKING 机制）
        if (!system->getVibrationUser().isSculkShrieker()) {
            continue;
        }
        SculkShriekerHelper::checkShriekingFinished(*m_world, pos);
    }
}

void SculkVibrationManager::registerListenerInChunk(const BlockPos& pos, gameevent::GameEventListener& listener)
{
    // 计算方块所在的区块和段落坐标
    ChunkCoord chunkX = CoordConverter::blockToChunk(pos.x);
    ChunkCoord chunkZ = CoordConverter::blockToChunk(pos.z);
    i32 sectionY = pos.sectionCoord();

    ChunkData* chunk = m_world->chunkManager()->tryToGetChunkInMem(chunkX, chunkZ);
    if (chunk == nullptr) {
        return;
    }

    // 创建 OnEmptyAction 回调：当注册表为空时自动从 ChunkData 中移除
    ChunkData* chunkPtr = chunk;
    auto factory = [this, chunkPtr](i32 sy) -> std::unique_ptr<gameevent::EuclideanGameEventListenerRegistry> {
        return std::make_unique<gameevent::EuclideanGameEventListenerRegistry>(*m_world,
            sy,
            [chunkPtr, sy](i32 emptySectionY) { chunkPtr->removeGameEventListenerRegistry(emptySectionY); });
    };

    gameevent::GameEventListenerRegistry& registry = chunk->getOrCreateGameEventListenerRegistry(sectionY, factory);
    registry.registerListener(listener);
}

void SculkVibrationManager::unregisterListenerFromChunk(const BlockPos& pos, gameevent::GameEventListener& listener)
{
    ChunkCoord chunkX = CoordConverter::blockToChunk(pos.x);
    ChunkCoord chunkZ = CoordConverter::blockToChunk(pos.z);
    i32 sectionY = pos.sectionCoord();

    ChunkData* chunk = m_world->chunkManager()->tryToGetChunkInMem(chunkX, chunkZ);
    if (chunk == nullptr) {
        return;
    }

    gameevent::GameEventListenerRegistry* registry = chunk->getGameEventListenerRegistry(sectionY);
    if (registry != nullptr) {
        registry->unregisterListener(listener);
    }
}

} // namespace mc::server
