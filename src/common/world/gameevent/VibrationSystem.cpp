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
 * @file VibrationSystem.cpp
 * @brief 振动系统实现
 */

#include "common/world/gameevent/VibrationSystem.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "server/world/ServerWorld.hpp"

#include <cmath>

namespace mc::gameevent {

// ============================================================================
// 振动频率映射表
// ============================================================================

/**
 * @brief 事件ID到振动频率的映射
 *
 * 参考 MC: VibrationSystem.VIBRATION_FREQUENCY_FOR_EVENT
 * 频率0表示不产生振动，1-15对应RESONATE_1到RESONATE_15
 */
static i32 getVibrationFrequencyForEvent(const char* eventId)
{
    // 步行/游泳/振翅 - 频率 1
    if (std::strcmp(eventId, "step") == 0 || std::strcmp(eventId, "swim") == 0 || std::strcmp(eventId, "flap") == 0) {
        return 1;
    }
    // 弹射物落地 - 频率 2
    if (std::strcmp(eventId, "projectile_land") == 0) {
        return 2;
    }
    // 方块失活/引信点燃 - 频率 3
    if (std::strcmp(eventId, "block_deactivate") == 0 || std::strcmp(eventId, "prime_fuse") == 0) {
        return 3;
    }
    // 实体动作/流体拾取/方块脱离 - 频率 4
    if (std::strcmp(eventId, "entity_action") == 0 || std::strcmp(eventId, "fluid_pickup") == 0 ||
        std::strcmp(eventId, "block_detach") == 0) {
        return 4;
    }
    // 实体交互/方块激活/方块放置/流体放置/方块附着 - 频率 5
    if (std::strcmp(eventId, "entity_interact") == 0 || std::strcmp(eventId, "block_activate") == 0 ||
        std::strcmp(eventId, "block_place") == 0 || std::strcmp(eventId, "fluid_place") == 0 ||
        std::strcmp(eventId, "block_attach") == 0) {
        return 5;
    }
    // 实体上坐骑/实体下坐骑 - 频率 6
    if (std::strcmp(eventId, "entity_mount") == 0 || std::strcmp(eventId, "entity_dismount") == 0) {
        return 6;
    }
    // 方块变化/方块打开/方块关闭/容器打开/容器关闭 - 频率 7
    if (std::strcmp(eventId, "block_change") == 0 || std::strcmp(eventId, "block_open") == 0 ||
        std::strcmp(eventId, "block_close") == 0 || std::strcmp(eventId, "container_open") == 0 ||
        std::strcmp(eventId, "container_close") == 0) {
        return 7;
    }
    // 方块销毁/饮用/进食 - 频率 8
    if (std::strcmp(eventId, "block_destroy") == 0 || std::strcmp(eventId, "drink") == 0 ||
        std::strcmp(eventId, "eat") == 0) {
        return 8;
    }
    // 幽匿感测体触须点击 - 频率 9
    if (std::strcmp(eventId, "sculk_sensor_tendrils_clicking") == 0) {
        return 9;
    }
    // 弹射物发射/乐器演奏/音符盒演奏/剪切 - 频率 10
    if (std::strcmp(eventId, "projectile_shoot") == 0 || std::strcmp(eventId, "instrument_play") == 0 ||
        std::strcmp(eventId, "note_block_play") == 0 || std::strcmp(eventId, "shear") == 0) {
        return 10;
    }
    // 实体受伤/实体死亡/装备更换/卸下装备 - 频率 11
    if (std::strcmp(eventId, "entity_damage") == 0 || std::strcmp(eventId, "entity_die") == 0 ||
        std::strcmp(eventId, "equip") == 0 || std::strcmp(eventId, "unequip") == 0) {
        return 11;
    }
    // 落地/溅水 - 频率 12
    if (std::strcmp(eventId, "hit_ground") == 0 || std::strcmp(eventId, "splash") == 0) {
        return 12;
    }
    // 唱片机播放/唱片机停止/物品交互开始/物品交互完成 - 频率 13
    if (std::strcmp(eventId, "jukebox_play") == 0 || std::strcmp(eventId, "jukebox_stop_play") == 0 ||
        std::strcmp(eventId, "item_interact_start") == 0 || std::strcmp(eventId, "item_interact_finish") == 0) {
        return 13;
    }
    // 爆炸/闪电击中 - 频率 14
    if (std::strcmp(eventId, "explode") == 0 || std::strcmp(eventId, "lightning_strike") == 0) {
        return 14;
    }
    // 尖啸 - 频率 15
    if (std::strcmp(eventId, "shriek") == 0) {
        return 15;
    }

    // 共鸣事件：频率等于事件编号
    for (i32 i = 1; i <= 15; ++i) {
        // 共鸣事件ID格式: "resonate_1" 到 "resonate_15"
        if (eventId[0] == 'r' && std::strncmp(eventId, "resonate_", 9) == 0) {
            // 提取数字部分
            const char* numStr = eventId + 9;
            i32 num = 0;
            while (*numStr >= '0' && *numStr <= '9') {
                num = num * 10 + (*numStr - '0');
                ++numStr;
            }
            if (*numStr == '\0' && num >= 1 && num <= 15) {
                return num;
            }
        }
    }

    // 未知事件不产生振动
    return 0;
}

// ============================================================================
// VibrationInfo
// ============================================================================

VibrationInfo::VibrationInfo(const GameEvent& event, f32 dist, const Vector3d& position, const Entity* entity)
    : gameEvent(&event)
    , distance(dist)
    , pos(position)
    , sourceEntityId(entity != nullptr ? entity->id() : 0)
    , hasSourceEntity(entity != nullptr)
{}

// ============================================================================
// VibrationSelector
// ============================================================================

void VibrationSelector::addCandidate(VibrationInfo info, u64 gameTick)
{
    if (shouldReplaceVibration(info, gameTick)) {
        m_currentCandidate = std::make_pair(std::move(info), gameTick);
    }
}

std::optional<VibrationInfo> VibrationSelector::chosenCandidate(u64 currentTick) const
{
    if (!m_currentCandidate.has_value()) {
        return std::nullopt;
    }

    const auto& [info, tickAdded] = m_currentCandidate.value();
    // 只有在添加 tick < 当前 tick 时才返回（确保振动至少延迟1 tick）
    if (tickAdded < currentTick) {
        return info;
    }
    return std::nullopt;
}

void VibrationSelector::startOver()
{
    m_currentCandidate = std::nullopt;
}

bool VibrationSelector::shouldReplaceVibration(const VibrationInfo& info, u64 gameTick) const
{
    if (!m_currentCandidate.has_value()) {
        return true;
    }

    const auto& [existing, tickAdded] = m_currentCandidate.value();

    // 不同 tick 的候选不替换
    if (gameTick != tickAdded) {
        return false;
    }

    // 同一 tick：距离近的优先
    if (info.distance < existing.distance) {
        return true;
    }
    if (info.distance > existing.distance) {
        return false;
    }

    // 距离相同：频率高的优先
    i32 newFreq = (info.gameEvent != nullptr) ? VibrationSystem::getGameEventFrequency(*info.gameEvent) : 0;
    i32 existingFreq =
        (existing.gameEvent != nullptr) ? VibrationSystem::getGameEventFrequency(*existing.gameEvent) : 0;
    return newFreq > existingFreq;
}

// ============================================================================
// VibrationSystem 静态方法
// ============================================================================

i32 VibrationSystem::getGameEventFrequency(const GameEvent& event)
{
    return getVibrationFrequencyForEvent(event.id());
}

const GameEvent* VibrationSystem::getResonanceEventByFrequency(i32 frequency)
{
    switch (frequency) {
        case 1:
            return &GameEvents::RESONATE_1;
        case 2:
            return &GameEvents::RESONATE_2;
        case 3:
            return &GameEvents::RESONATE_3;
        case 4:
            return &GameEvents::RESONATE_4;
        case 5:
            return &GameEvents::RESONATE_5;
        case 6:
            return &GameEvents::RESONATE_6;
        case 7:
            return &GameEvents::RESONATE_7;
        case 8:
            return &GameEvents::RESONATE_8;
        case 9:
            return &GameEvents::RESONATE_9;
        case 10:
            return &GameEvents::RESONATE_10;
        case 11:
            return &GameEvents::RESONATE_11;
        case 12:
            return &GameEvents::RESONATE_12;
        case 13:
            return &GameEvents::RESONATE_13;
        case 14:
            return &GameEvents::RESONATE_14;
        case 15:
            return &GameEvents::RESONATE_15;
        default:
            return nullptr;
    }
}

i32 VibrationSystem::getRedstoneStrengthForDistance(f32 distance, i32 radius)
{
    f64 d0 = 15.0 / static_cast<f64>(radius);
    return std::max(1, 15 - static_cast<i32>(std::floor(d0 * static_cast<f64>(distance))));
}

// ============================================================================
// VibrationSystem::User
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

    // 检查源实体是否在潜行且事件忽略潜行
    // TODO: 当实体潜行系统实现后，添加潜行检查

    // 检查源实体是否阻尼振动（如羊毛覆盖的实体）
    // TODO: 当 dampensVibrations 实现后，添加阻尼检查

    // 检查受影响方块是否阻尼振动（如羊毛）
    // TODO: 当 BlockTags::DAMPENS_VIBRATIONS 实现后，添加方块阻尼检查

    // 源实体不可为旁观者
    if (context.sourceEntity() != nullptr) {
        const auto* player = dynamic_cast<const Player*>(context.sourceEntity());
        if (player != nullptr && player->isSpectator()) {
            return false;
        }
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

    // 检查相邻区块是否正在 tick（如果需要）
    if (user.requiresAdjacentChunksToBeTicking()) {
        // TODO: 当区块 tick 追踪实现后，检查相邻区块是否在 tick
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
