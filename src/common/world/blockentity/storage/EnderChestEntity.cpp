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

#include "world/blockentity/storage/EnderChestEntity.hpp"
#include "common/core/Types.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/gameevent/GameEvents.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

namespace {

/// 末影箱开盖动画每 tick 变化量。
constexpr f32 ENDER_CHEST_LID_SPEED = 0.1f;
/// 末影箱状态同步计数间隔（tick）。
constexpr i32 ENDER_CHEST_SYNC_INTERVAL = 80;
/// 玩家访问最大距离的平方（64格）
constexpr f32 MAX_ACCESS_DISTANCE_SQ = 64.0f * 64.0f;

} // namespace

// ========== EnderChestEntity 实现 ==========

EnderChestEntity::EnderChestEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::EnderChest, pos)
{}

EnderChestEntity::~EnderChestEntity() = default;

bool EnderChestEntity::openContainer(Player* player)
{
    if (player == nullptr) {
        return false;
    }

    // 检查玩家是否在访问范围内
    if (!canPlayerAccess(player)) {
        return false;
    }

    // 末影箱内容由玩家侧末影箱背包提供，这里仅维护方块实体动画与开关计数。
    m_openCount++;

    // 打开时播放音效和游戏事件
    // 当从 0 变为 1 时播放打开音效
    if (m_openCount == 1) {
        IWorld* world = player->world();
        if (world != nullptr && !world->isClientSide()) {
            world->playSound(
                SoundEvents::BLOCK_ENDER_CHEST_OPEN, sound::SoundCategory::Blocks, m_pos.center(), 0.5f, 1.0f);
            world->gameEvent(gameevent::GameEvents::CONTAINER_OPEN, m_pos, gameevent::GameEvent::Context::of(player));
        }
    }

    // 同步开合动画到客户端
    if (m_world != nullptr && !m_world->isClientSide()) {
        const BlockState* state = m_world->getBlockState(m_pos);
        if (state != nullptr) {
            m_world->blockEvent(m_pos, state->getBlock(), 1, m_openCount);
        }
    }

    setChanged();
    return true;
}

void EnderChestEntity::closeContainer(Player* player)
{
    MC_UNUSED(player);

    if (m_openCount > 0) {
        m_openCount--;

        // 同步开合动画到客户端
        if (m_world != nullptr && !m_world->isClientSide()) {
            const BlockState* state = m_world->getBlockState(m_pos);
            if (state != nullptr) {
                m_world->blockEvent(m_pos, state->getBlock(), 1, m_openCount);
            }
        }

        setChanged();
    }
}

bool EnderChestEntity::triggerEvent(i32 id, i32 type)
{
    if (id == 1) {
        // 末影箱开合动画事件：type > 0 表示打开，type == 0 表示关闭
        m_openCount = type;
        return true;
    }
    return false;
}

bool EnderChestEntity::canPlayerAccess(Player* player) const
{
    if (player == nullptr) {
        return false;
    }

    // 检查玩家是否在 64 格以内
    return player->position().distanceSquared(m_pos.center()) <= MAX_ACCESS_DISTANCE_SQ;
}

void EnderChestEntity::updateLidAnimation(f32 partialTick)
{
    MC_UNUSED(partialTick);

    // 更新盖子动画
    m_prevLidAngle = m_lidAngle;

    if (m_openCount > 0 && m_lidAngle < 1.0f) {
        m_lidAngle += ENDER_CHEST_LID_SPEED;
        if (m_lidAngle > 1.0f) {
            m_lidAngle = 1.0f;
        }
    } else if (m_openCount == 0 && m_lidAngle > 0.0f) {
        m_lidAngle -= ENDER_CHEST_LID_SPEED;
        if (m_lidAngle < 0.0f) {
            m_lidAngle = 0.0f;
        }
    }
}

f32 EnderChestEntity::getLidAngle(f32 partialTick) const
{
    return m_prevLidAngle + (m_lidAngle - m_prevLidAngle) * partialTick;
}

void EnderChestEntity::tick(IWorld& world)
{
    m_ticksSinceSync++;

    // 计数到阈值后重置，具体网络同步由上层容器/网络系统处理。
    if (m_ticksSinceSync >= ENDER_CHEST_SYNC_INTERVAL) {
        m_ticksSinceSync = 0;
    }

    // 记录上一帧的开门数，用于检测关门时机
    const i32 prevOpenCount = m_openCount;

    // 更新动画
    updateLidAnimation(0.0f);

    // 关门时播放音效和游戏事件
    // 当盖子从 >0.5 变为 <=0.5 时播放关闭音效
    if (!world.isClientSide() && prevOpenCount > 0 && m_openCount == 0 && m_prevLidAngle > 0.5f && m_lidAngle <= 0.5f) {
        world.playSound(SoundEvents::BLOCK_ENDER_CHEST_CLOSE, sound::SoundCategory::Blocks, m_pos.center(), 0.5f, 1.0f);
        world.gameEvent(gameevent::GameEvents::CONTAINER_CLOSE, m_pos, nullptr);
    }
}

bool EnderChestEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 末影箱不存储物品，物品在玩家数据中
    if (data.contains("open_count")) {
        m_openCount = data["open_count"].get<i32>();
    }

    return true;
}

void EnderChestEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["open_count"] = m_openCount;
}

std::unique_ptr<BlockEntity> EnderChestEntity::clone() const
{
    auto clone = std::make_unique<EnderChestEntity>(m_pos);
    clone->m_openCount = m_openCount;
    clone->m_lidAngle = m_lidAngle;
    clone->m_prevLidAngle = m_prevLidAngle;
    return clone;
}

} // namespace blockentity
} // namespace mc
