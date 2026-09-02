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

#include "MiningManager.hpp"
#include "InventoryManager.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/tool/EfficiencyEnchantment.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::interaction {

// ============================================================================
// 常量定义
// ============================================================================

namespace {
/// 正确工具挖掘除数
constexpr f32 CORRECT_TOOL_DIVISOR = 30.0f;
/// 错误工具挖掘除数
constexpr f32 WRONG_TOOL_DIVISOR = 100.0f;
/// 水下挖掘惩罚
constexpr f32 UNDERWATER_PENALTY = 5.0f;
/// 空中挖掘惩罚
constexpr f32 OFF_GROUND_PENALTY = 5.0f;

/// 挖掘疲劳乘数表
/// 等级 0(I) = 0.3, 1(II) = 0.09, 2(III) = 0.0027, 3+(IV+) = 0.00081
constexpr f32 MINING_FATIGUE_MULTIPLIERS[] = {0.3f, 0.09f, 0.0027f, 0.00081f};
constexpr size_t MINING_FATIGUE_MULTIPLIERS_COUNT = 4;
} // namespace

// ============================================================================
// 构造函数
// ============================================================================

MiningManager::MiningManager(core::PlayerManager& playerManager, core::ConnectionManager& connectionManager)
    : m_playerManager(playerManager)
    , m_connectionManager(connectionManager)
{}

// ============================================================================
// 设置依赖
// ============================================================================

void MiningManager::setInventoryManager(InventoryManager* inventoryManager)
{
    m_inventoryManager = inventoryManager;
}

// ============================================================================
// 核心方法
// ============================================================================

void MiningManager::startMining(PlayerId playerId, const BlockPos& pos, EntityInstanceId entityId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Mining,
        "MiningManager::startMining",
        "playerId",
        playerId,
        "pos",
        pos.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    auto& state = m_miningStates[playerId];
    state.position = pos;
    state.progress = 0.0f;
    state.lastStage = 255;
    state.active = true;
    state.startTick = m_gameTicks; // 对齐原版 destroyProgressStart = gameTicks
    state.breakerId = entityId;
    state.hasDelayedDestroy = false;
}

void MiningManager::abortMining(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Mining, "MiningManager::abortMining", "playerId", playerId);

    auto it = m_miningStates.find(playerId);
    if (it != m_miningStates.end()) {
        // 对应 MC 原版: ServerPlayerGameMode.stopDestroyBlock() 中发送 stage=-1 的
        // destroyBlockProgress，通知其他玩家移除破坏动画。
        if (it->second.active && it->second.lastStage != 255) {
            _broadcastBreakAnim(playerId, it->second.position, -1);
        }
        m_miningStates.erase(it);
    }
}

void MiningManager::handleBlockInteraction(
    PlayerId playerId, const BlockPos& pos, network::BlockInteractionAction action)
{
    switch (action) {
        case network::BlockInteractionAction::StartDestroyBlock: {
            // 通过解析器获取正确的 EntityInstanceId（PlayerId != EntityInstanceId）
            EntityInstanceId entityId = 0;
            if (m_entityIdResolver) {
                entityId = m_entityIdResolver(playerId);
            }
            startMining(playerId, pos, entityId);

            // 对齐原版 ServerPlayerGameMode.handleBlockBreakAction START_DESTROY_BLOCK
            // 分支 instabuild 检查（:165-168）：creative 模式 START 时同步立即破坏，
            // 不进入 isDestroyingBlock 流程、不等 tick。此前本项目 creative 仍需等下一
            // tick 的 tick() 才触发完成，与原版同步立即破坏语义不符。
            auto* playerData = m_playerManager.getPlayer(playerId);
            if (playerData != nullptr && playerData->gameMode == GameMode::Creative) {
                if (m_onMiningComplete) {
                    m_onMiningComplete(playerId, pos);
                }
                // creative 立即破坏后清理状态，避免 STOP 到达时 tryCompleteMining 找不到状态
                m_miningStates.erase(playerId);
            }
            break;
        }

        case network::BlockInteractionAction::AbortDestroyBlock:
            abortMining(playerId);
            break;

        case network::BlockInteractionAction::StopDestroyBlock:
            MC_UNUSED(playerId);
            MC_UNUSED(pos);
            break;
    }
}

void MiningManager::tick(ServerWorld& world)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Mining, "MiningManager::tick", "activeCount", m_miningStates.size());

    ++m_gameTicks; // 对齐原版 ServerPlayerGameMode.tick()(:109) gameTicks++

    for (auto& [playerId, state] : m_miningStates) {
        // 1. delayed-destroy 优先处理（对齐原版 tick :110-120）
        if (state.hasDelayedDestroy) {
            const BlockState* delayedState =
                world.getBlockState(state.delayedDestroyPos.x, state.delayedDestroyPos.y, state.delayedDestroyPos.z);
            if (!delayedState || delayedState->isAir()) {
                // 方块已被其他方式破坏，取消延迟
                state.hasDelayedDestroy = false;
            } else {
                // 对齐原版 incrementDestroyProgress(delayedState, delayedDestroyPos, delayedTickStart)
                f32 speed = _calculateMiningSpeed(world, state.delayedDestroyPos, playerId);
                f32 f = speed * static_cast<f32>(m_gameTicks - state.delayedTickStart + 1);
                u8 stage = static_cast<u8>(std::min(f * 10.0f, 9.0f));
                if (stage != state.lastStage) {
                    _broadcastBreakAnim(playerId, state.delayedDestroyPos, static_cast<i8>(stage));
                    state.lastStage = stage;
                }
                if (f >= 1.0f) {
                    state.hasDelayedDestroy = false;
                    if (m_onMiningComplete) {
                        m_onMiningComplete(playerId, state.delayedDestroyPos);
                    }
                }
            }
            continue;
        }

        if (!state.active) {
            continue;
        }

        // 检查玩家是否仍然有效
        auto* playerData = m_playerManager.getPlayer(playerId);
        if (!playerData || !playerData->loggedIn) {
            state.active = false;
            continue;
        }

        // 距离复检：玩家走远后中止挖掘（对齐 vanilla ServerPlayerGameMode 持续校验
        // isWithinBlockInteractionRange）。MiningManager 无 Player 实体访问，此处用
        // PlayerData 眼位到方块中心距离做保底；宽松阈值 8 格（>创造最大交互距离 5.0
        // + padding），入口 ServerPlayHandler 已做精确属性校验，此处仅防走远。
        const f64 eyeX = playerData->x;
        const f64 eyeY = playerData->y + static_cast<f64>(game::PLAYER_EYE_HEIGHT);
        const f64 eyeZ = playerData->z;
        const f64 dx = (state.position.x + 0.5) - eyeX;
        const f64 dy = (state.position.y + 0.5) - eyeY;
        const f64 dz = (state.position.z + 0.5) - eyeZ;
        constexpr f64 MAX_MINING_DISTANCE_SQ = 64.0; // 8 * 8
        if (dx * dx + dy * dy + dz * dz > MAX_MINING_DISTANCE_SQ) {
            state.active = false;
            continue;
        }

        // 检查方块是否仍然存在
        const BlockState* blockState = world.getBlockState(state.position.x, state.position.y, state.position.z);

        if (!blockState || blockState->isAir()) {
            // 方块已被破坏（可能是其他玩家），对齐原版 :123-126 清除破坏阶段并停止
            if (state.lastStage != 255) {
                _broadcastBreakAnim(playerId, state.position, -1);
                state.lastStage = 255;
            }
            state.active = false;
            continue;
        }

        // 绝对进度模型（对齐原版 incrementDestroyProgress :133-143）：
        // f = 单tick进度 × (已过tick数 + 1)，是绝对进度而非累加。
        f32 speed = _calculateMiningSpeed(world, state.position, playerId);
        state.progress = speed * static_cast<f32>(m_gameTicks - state.startTick + 1);

        // 计算动画阶段 (0-9)，对齐原版 j = (int)(f * 10.0F)
        u8 stage = static_cast<u8>(std::min(state.progress * 10.0f, 9.0f));

        // 广播动画（阶段变化时），对齐原版 :137-139
        if (stage != state.lastStage) {
            _broadcastBreakAnim(playerId, state.position, static_cast<i8>(stage));
            state.lastStage = stage;
        }

        // 检查是否完成（对齐原版 :116-119，f >= 1.0F 即破坏）
        if (state.progress >= 1.0f) {
            MC_TRACE_INSTANT_EVENT(TraceEvents.Server.Mining,
                "MiningManager::miningComplete",
                "playerId",
                playerId,
                "pos",
                state.position.toString(),
                [flow = ::perfetto::Flow::ProcessScoped(state.position.toId())](
                    ::perfetto::EventContext ctx) { flow(ctx); });

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

bool MiningManager::tryCompleteMining(PlayerId playerId, const BlockPos& pos, ServerWorld& world)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Mining,
        "MiningManager::tryCompleteMining",
        "playerId",
        playerId,
        "pos",
        pos.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 容错：状态不存在直接返回 false（对齐原版 :210 equals(destroyPos) 守卫）。
    // 此前为 MC_ASSERT_RELEASE(it != end())，STOP 乱序/重复/creative 已 erase 后到达会触发断言崩溃，
    // 改为宽容返回消除崩溃风险。
    auto it = m_miningStates.find(playerId);
    if (it == m_miningStates.end()) {
        return false;
    }

    MiningState& state = it->second;
    // 对齐原版 :210：仅处理 pos.equals(destroyPos) 的情况
    if (!state.active || state.position != pos) {
        return false;
    }

    // 对齐原版 :211-214：f1 = getDestroyProgress * (gameTicks - destroyProgressStart + 1)
    // 注：原版公式是 (gameTicks - destroyProgressStart)，但 incrementDestroyProgress 用 (i+1)，
    // 此处统一用 (gameTicks - startTick + 1) 与 tick 中的绝对进度计算保持一致。
    f32 speed = _calculateMiningSpeed(world, pos, playerId);
    f32 f1 = speed * static_cast<f32>(m_gameTicks - state.startTick + 1);

    if (f1 >= 0.7f) {
        // 对齐原版 :215-219：进度达 0.7 阈值，清除破坏阶段并立即破坏。
        // destroyAndAck 内部调 destroyBlock 真正破坏方块，此处由 onMiningComplete 回调
        // 触发 handleBlockBreak 完成破坏（与 tick 中 progress>=1.0 的完成路径一致）。
        if (state.lastStage != 255) {
            _broadcastBreakAnim(playerId, pos, -1);
        }
        m_miningStates.erase(it);
        if (m_onMiningComplete) {
            m_onMiningComplete(playerId, pos);
        }
        return true;
    }

    // 对齐原版 :222-226：进度不足 0.7，转入 delayed-destroy 状态机，由 tick 续挖
    if (!state.hasDelayedDestroy) {
        state.active = false;
        state.hasDelayedDestroy = true;
        state.delayedDestroyPos = pos;
        state.delayedTickStart = state.startTick;
    }
    return false;
}

void MiningManager::setOnBreakAnimBroadcast(std::function<void(PlayerId, i32, i32, i32, i8)> callback)
{
    m_onBreakAnimBroadcast = std::move(callback);
}

void MiningManager::setEntityIdResolver(std::function<EntityInstanceId(PlayerId)> resolver)
{
    m_entityIdResolver = std::move(resolver);
}

void MiningManager::setOnMiningComplete(std::function<void(PlayerId, const BlockPos&)> callback)
{
    m_onMiningComplete = std::move(callback);
}

// ============================================================================
// 挖掘速度计算
// ============================================================================

f32 MiningManager::_calculateMiningSpeed(ServerWorld& world, const BlockPos& pos, PlayerId playerId) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Mining,
        "MiningManager::calculateMiningSpeed",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 1. 获取方块状态
    const BlockState* state = world.getBlockState(pos);
    if (!state || state->isAir()) {
        return 1.0f; // 方块不存在，快速完成
    }

    // 2. 获取方块硬度
    f32 hardness = state->hardness();
    if (hardness < 0.0f) {
        return 0.0f; // 不可破坏（基岩等）
    }

    // 3. 获取玩家数据
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData) {
        return 0.0f;
    }

    // 4. 创造模式瞬间破坏
    if (playerData->gameMode == GameMode::Creative) {
        return 1.0f; // 创造模式一 tick 破坏任何方块
    }

    // 5. 硬度为 0 的方块瞬间破坏
    if (hardness == 0.0f) {
        return 1.0f;
    }

    // 6. 获取手持物品
    ItemStack heldItem;
    if (m_inventoryManager) {
        heldItem = m_inventoryManager->getHeldItem(playerId);
    }

    // 7. 计算挖掘速度倍率
    f32 digSpeed = _calculateDigSpeedMultiplier(world, heldItem, *state, *playerData);

    // 8. 检查是否可以使用正确工具
    bool canHarvest = false;
    if (!heldItem.isEmpty()) {
        canHarvest = heldItem.canHarvestBlock(*state);
    } else {
        // 空手只能采集不需要工具的方块
        canHarvest = !state->requiresTool();
    }

    // 9. 计算工具除数
    f32 divisor = canHarvest ? CORRECT_TOOL_DIVISOR : WRONG_TOOL_DIVISOR;

    // 10. 计算最终挖掘进度
    // 方块相对硬度 = digSpeed / hardness / divisor
    // 挖掘时间（tick）= 1 / 相对硬度
    // 每tick进度 = 相对硬度
    f32 relativeHardness = digSpeed / hardness / divisor;

    return relativeHardness;
}

f32 MiningManager::_calculateDigSpeedMultiplier(ServerWorld& world,
    const ItemStack& heldItem,
    const BlockState& blockState,
    const ServerPlayerData& playerData) const
{
    // 1. 获取工具基础挖掘速度
    f32 speed = heldItem.isEmpty() ? 1.0f : heldItem.getDestroySpeed(blockState);

    // 2. 效率附魔加成（仅当工具对当前方块有效时）
    if (speed > 1.0f) {
        i32 efficiencyLevel = item::enchant::EnchantmentHelper::getEfficiencyLevel(heldItem);
        if (efficiencyLevel > 0) {
            // 效率附魔加成公式: level^2 + 1
            // I: 2, II: 5, III: 10, IV: 17, V: 26
            speed += static_cast<f32>(item::enchant::EfficiencyEnchantment::getMiningSpeedBonus(efficiencyLevel));
        }
    }

    // 3. 急迫效果和潮涌能量加成
    f32 hasteMultiplier = _calculateHasteMultiplier(playerData);
    speed *= hasteMultiplier;

    // 4. 挖掘疲劳惩罚
    f32 fatigueMultiplier = _calculateMiningFatigueMultiplier(playerData);
    speed *= fatigueMultiplier;

    // 5. 水下挖掘惩罚（仅当眼睛在水中且没有水下速掘附魔）
    if (_areEyesInWater(world, playerData) && !_hasAquaAffinity(playerData)) {
        speed /= UNDERWATER_PENALTY;
    }

    // 6. 空中挖掘惩罚
    if (!playerData.onGround) {
        speed /= OFF_GROUND_PENALTY;
    }

    return speed;
}

f32 MiningManager::_calculateHasteMultiplier(const ServerPlayerData& playerData) const
{
    // 急迫和潮涌能量都可以增加挖掘速度，取最大值
    i32 hasteLevel = -1; // -1 表示没有效果
    i32 conduitLevel = -1;

    // 检查急迫效果
    const auto* hasteEffect = playerData.getEffect(entity::effect::EffectType::Haste);
    if (hasteEffect) {
        hasteLevel = hasteEffect->amplifier(); // amplifier = level - 1
    }

    // 检查潮涌能量效果
    const auto* conduitEffect = playerData.getEffect(entity::effect::EffectType::ConduitPower);
    if (conduitEffect) {
        conduitLevel = conduitEffect->amplifier();
    }

    // 如果两个效果都没有，返回 1.0
    i32 maxLevel = std::max(hasteLevel, conduitLevel);
    if (maxLevel < 0) {
        return 1.0f;
    }

    // 计算乘数: 1.0 + (amplifier + 1) * 0.2
    // I级: 1.2, II级: 1.4, III级: 1.6, ...
    return 1.0f + static_cast<f32>(maxLevel + 1) * 0.2f;
}

f32 MiningManager::_calculateMiningFatigueMultiplier(const ServerPlayerData& playerData) const
{
    const auto* fatigueEffect = playerData.getEffect(entity::effect::EffectType::MiningFatigue);
    if (!fatigueEffect) {
        return 1.0f;
    }

    i32 amplifier = fatigueEffect->amplifier();

    // 使用预定义的乘数表
    if (amplifier >= 0 && static_cast<size_t>(amplifier) < MINING_FATIGUE_MULTIPLIERS_COUNT) {
        return MINING_FATIGUE_MULTIPLIERS[amplifier];
    }

    // 超出范围使用最小值
    return MINING_FATIGUE_MULTIPLIERS[MINING_FATIGUE_MULTIPLIERS_COUNT - 1];
}

bool MiningManager::_hasAquaAffinity(const ServerPlayerData& playerData) const
{
    // 检查头盔是否有水下速掘附魔
    if (!m_inventoryManager) {
        return false;
    }

    // 获取玩家物品栏
    const PlayerInventory* inventory = m_inventoryManager->getInventory(playerData.playerId);
    if (!inventory) {
        return false;
    }

    // 获取头盔
    ItemStack helmet = inventory->getHelmet();
    if (helmet.isEmpty()) {
        return false;
    }

    // 检查是否有水下速掘附魔
    return item::enchant::EnchantmentHelper::hasAquaAffinity(helmet);
}

bool MiningManager::_areEyesInWater(ServerWorld& world, const ServerPlayerData& playerData) const
{
    // 检测玩家眼睛是否在水中

    // 1. 计算眼睛检测点 Y 坐标
    // 眼睛位置向下偏移约 0.11 格，避免边界精度问题
    constexpr f64 EYE_OFFSET = 0.11111111;
    const f64 eyeY = static_cast<f64>(playerData.y) + static_cast<f64>(game::PLAYER_EYE_HEIGHT) - EYE_OFFSET;

    // 2. 获取检测点坐标（玩家脚下位置）
    const i32 eyeBlockX = static_cast<i32>(std::floor(playerData.x));
    const i32 eyeBlockY = static_cast<i32>(std::floor(eyeY));
    const i32 eyeBlockZ = static_cast<i32>(std::floor(playerData.z));

    // 3. 获取该位置的流体状态
    const fluid::FluidState* fluidState = world.getFluidState(eyeBlockX, eyeBlockY, eyeBlockZ);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 4. 检查流体是否为水
    const fluid::Fluid& fluid = fluidState->getFluid();
    if (!fluid.isIn(fluid::FluidTags::WATER())) {
        return false;
    }

    // 5. 计算流体表面高度
    const BlockPos pos(eyeBlockX, eyeBlockY, eyeBlockZ);
    const f32 fluidHeight = fluidState->getHeight();
    const f64 fluidSurfaceY = static_cast<f64>(eyeBlockY) + static_cast<f64>(fluidHeight);

    // 6. 如果流体表面高度 > 检测点高度，则眼睛在水中
    return fluidSurfaceY > eyeY;
}

void MiningManager::_broadcastBreakAnim(PlayerId playerId, const BlockPos& pos, i8 stage)
{
    if (m_onBreakAnimBroadcast) {
        m_onBreakAnimBroadcast(playerId, pos.x, pos.y, pos.z, stage);
    }
}

} // namespace mc::server::interaction
