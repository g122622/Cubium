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
 * @brief 振动系统实现 - 不依赖服务端的部分
 *
 * 包含 VibrationInfo、VibrationSelector、VibrationSystem 静态方法和
 * User::calculateTravelTimeInTicks 的实现。
 * 依赖 ServerWorld 的部分（Listener、Ticker、User::isValidVibration）
 * 位于 src/server/world/gameevent/VibrationSystemServer.cpp。
 */

#include "common/world/gameevent/VibrationSystem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::gameevent {

// ============================================================================
// 振动频率映射表
// ============================================================================

/**
 * @brief 事件ID到振动频率的映射
 *
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

bool VibrationSystem::isIgnoredBySneaking(const GameEvent& event)
{
    // 参考: net.minecraft.tags.GameEventTags.IGNORE_VIBRATIONS_SNEAKING
    // 当源实体正在潜行（isSteppingCarefully）时，这些事件不触发振动
    const char* id = event.id();
    return std::strcmp(id, "hit_ground") == 0 || std::strcmp(id, "projectile_shoot") == 0 ||
        std::strcmp(id, "step") == 0 || std::strcmp(id, "swim") == 0 || std::strcmp(id, "item_interact_start") == 0 ||
        std::strcmp(id, "item_interact_finish") == 0;
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
// VibrationInfo 序列化
// ============================================================================

void VibrationInfo::saveToNBT(nbt::CompoundTag& tag) const
{
    // game_event: 事件 ID 字符串
    if (gameEvent != nullptr) {
        tag.put("game_event", std::string(gameEvent->id()));
    }

    // distance: 振动传播距离
    tag.put("distance", static_cast<f32>(distance));

    // pos: 振动源位置 [x, y, z]
    auto posList = std::make_unique<nbt::tags::double_list_tag>();
    posList->value.push_back(pos.x);
    posList->value.push_back(pos.y);
    posList->value.push_back(pos.z);
    tag.value.emplace("pos", std::move(posList));

    // source: 源实体 ID（可选）
    if (hasSourceEntity) {
        tag.put("source", static_cast<i64>(sourceEntityId));
    }
}

bool VibrationInfo::loadFromNBT(const nbt::CompoundTag& tag)
{
    // game_event: 事件 ID 字符串
    auto eventId = entity::serialization::nbt_helper::tryGetString(tag, "game_event");
    if (eventId.has_value()) {
        gameEvent = GameEvents::getGameEventById(*eventId);
        // 如果事件 ID 无法识别，gameEvent 将为 nullptr，但仍视为成功加载
        // （存档中可能包含未来版本的事件）
    } else {
        // 缺少 game_event 字段，振动信息无效
        return false;
    }

    // distance: 振动传播距离
    auto distOpt = entity::serialization::nbt_helper::tryGetFloat(tag, "distance");
    if (distOpt.has_value()) {
        distance = *distOpt;
    }

    // pos: 振动源位置 [x, y, z]
    auto* posList = entity::serialization::nbt_helper::tryGetList(tag, "pos");
    if (posList != nullptr && posList->element_id() == nbt::TagId::Double) {
        const auto& doubles = dynamic_cast<const nbt::tags::double_list_tag&>(*posList);
        if (doubles.value.size() >= 3) {
            pos.x = doubles.value[0];
            pos.y = doubles.value[1];
            pos.z = doubles.value[2];
        }
    }

    // source: 源实体 ID（可选）
    auto sourceOpt = entity::serialization::nbt_helper::tryGetLong(tag, "source");
    if (sourceOpt.has_value()) {
        sourceEntityId = static_cast<u64>(*sourceOpt);
        hasSourceEntity = true;
    } else {
        sourceEntityId = 0;
        hasSourceEntity = false;
    }

    return true;
}

void VibrationInfo::saveToJson(nlohmann::json& data) const
{
    if (gameEvent != nullptr) {
        data["game_event"] = gameEvent->id();
    }
    data["distance"] = distance;
    data["pos"] = {pos.x, pos.y, pos.z};
    if (hasSourceEntity) {
        data["source"] = static_cast<i64>(sourceEntityId);
    }
}

bool VibrationInfo::loadFromJson(const nlohmann::json& data)
{
    // game_event
    if (data.contains("game_event") && data["game_event"].is_string()) {
        gameEvent = GameEvents::getGameEventById(data["game_event"].get<std::string>());
    } else {
        return false;
    }

    // distance
    if (data.contains("distance") && data["distance"].is_number()) {
        distance = data["distance"].get<f32>();
    }

    // pos
    if (data.contains("pos") && data["pos"].is_array() && data["pos"].size() >= 3) {
        pos.x = data["pos"][0].get<f64>();
        pos.y = data["pos"][1].get<f64>();
        pos.z = data["pos"][2].get<f64>();
    }

    // source
    if (data.contains("source") && data["source"].is_number_integer()) {
        sourceEntityId = static_cast<u64>(data["source"].get<i64>());
        hasSourceEntity = true;
    } else {
        sourceEntityId = 0;
        hasSourceEntity = false;
    }

    return true;
}

// ============================================================================
// VibrationSelector 序列化
// ============================================================================

void VibrationSelector::saveToNBT(nbt::CompoundTag& tag) const
{
    if (m_currentCandidate.has_value()) {
        const auto& [info, tick] = m_currentCandidate.value();

        // event: 候选振动信息
        auto eventTag = std::make_unique<nbt::CompoundTag>();
        info.saveToNBT(*eventTag);
        tag.value.emplace("event", std::move(eventTag));

        // tick: 候选被添加时的游戏 tick
        tag.put("tick", static_cast<i64>(tick));
    } else {
        // 无候选时，tick 为 -1
        tag.put("tick", static_cast<i64>(-1));
    }
}

bool VibrationSelector::loadFromNBT(const nbt::CompoundTag& tag)
{
    // tick: 候选被添加时的游戏 tick
    auto tickOpt = entity::serialization::nbt_helper::tryGetLong(tag, "tick");
    i64 tick = tickOpt.value_or(-1);

    // event: 候选振动信息（可选）
    const nbt::CompoundTag* eventTag = entity::serialization::nbt_helper::tryGetCompound(tag, "event");
    if (eventTag != nullptr && tick >= 0) {
        VibrationInfo info;
        if (info.loadFromNBT(*eventTag)) {
            m_currentCandidate = std::make_pair(std::move(info), static_cast<u64>(tick));
        } else {
            m_currentCandidate = std::nullopt;
        }
    } else {
        m_currentCandidate = std::nullopt;
    }

    return true;
}

void VibrationSelector::saveToJson(nlohmann::json& data) const
{
    if (m_currentCandidate.has_value()) {
        const auto& [info, tick] = m_currentCandidate.value();
        nlohmann::json eventData;
        info.saveToJson(eventData);
        data["event"] = std::move(eventData);
        data["tick"] = tick;
    } else {
        data["tick"] = -1;
    }
}

bool VibrationSelector::loadFromJson(const nlohmann::json& data)
{
    i64 tick = -1;
    if (data.contains("tick") && data["tick"].is_number_integer()) {
        tick = data["tick"].get<i64>();
    }

    if (data.contains("event") && data["event"].is_object() && tick >= 0) {
        VibrationInfo info;
        if (info.loadFromJson(data["event"])) {
            m_currentCandidate = std::make_pair(std::move(info), static_cast<u64>(tick));
        } else {
            m_currentCandidate = std::nullopt;
        }
    } else {
        m_currentCandidate = std::nullopt;
    }

    return true;
}

// ============================================================================
// VibrationSystem::Data 序列化
// ============================================================================

void VibrationSystem::Data::saveToNBT(nbt::CompoundTag& tag) const
{
    // event: 当前正在传播的振动（可选）
    if (m_currentVibration.has_value()) {
        auto eventTag = std::make_unique<nbt::CompoundTag>();
        m_currentVibration.value().saveToNBT(*eventTag);
        tag.value.emplace("event", std::move(eventTag));
    }

    // selector: 振动选择器
    auto selectorTag = std::make_unique<nbt::CompoundTag>();
    m_selectionStrategy.saveToNBT(*selectorTag);
    tag.value.emplace("selector", std::move(selectorTag));

    // event_delay: 传播剩余时间（tick）
    tag.put("event_delay", static_cast<i32>(m_travelTimeInTicks));

    // 注意：reloadVibrationParticle 不序列化到 NBT，反序列化时始终设为 true
}

bool VibrationSystem::Data::loadFromNBT(const nbt::CompoundTag& tag)
{
    // event: 当前正在传播的振动（可选，对齐 MC 原版 lenientOptionalFieldOf）
    const nbt::CompoundTag* eventTag = entity::serialization::nbt_helper::tryGetCompound(tag, "event");
    if (eventTag != nullptr) {
        VibrationInfo info;
        if (info.loadFromNBT(*eventTag)) {
            m_currentVibration = std::move(info);
        } else {
            m_currentVibration = std::nullopt;
        }
    } else {
        m_currentVibration = std::nullopt;
    }

    // selector: 振动选择器
    const nbt::CompoundTag* selectorTag = entity::serialization::nbt_helper::tryGetCompound(tag, "selector");
    if (selectorTag != nullptr) {
        (void)m_selectionStrategy.loadFromNBT(*selectorTag);
    } else {
        m_selectionStrategy.startOver();
    }

    // event_delay: 传播剩余时间（tick），默认 0
    auto delayOpt = entity::serialization::nbt_helper::tryGetInt(tag, "event_delay");
    m_travelTimeInTicks = delayOpt.value_or(0);
    if (m_travelTimeInTicks < 0) {
        m_travelTimeInTicks = 0;
    }

    // 对齐 MC 原版 CODEC 行为：从存档加载时 reloadVibrationParticle = true
    m_reloadVibrationParticle = true;

    return true;
}

void VibrationSystem::Data::saveToJson(nlohmann::json& data) const
{
    // event: 当前正在传播的振动
    if (m_currentVibration.has_value()) {
        nlohmann::json eventData;
        m_currentVibration.value().saveToJson(eventData);
        data["event"] = std::move(eventData);
    }

    // selector: 振动选择器
    nlohmann::json selectorData;
    m_selectionStrategy.saveToJson(selectorData);
    data["selector"] = std::move(selectorData);

    // event_delay: 传播剩余时间
    data["event_delay"] = m_travelTimeInTicks;
}

bool VibrationSystem::Data::loadFromJson(const nlohmann::json& data)
{
    // event
    if (data.contains("event") && data["event"].is_object()) {
        VibrationInfo info;
        if (info.loadFromJson(data["event"])) {
            m_currentVibration = std::move(info);
        } else {
            m_currentVibration = std::nullopt;
        }
    } else {
        m_currentVibration = std::nullopt;
    }

    // selector
    if (data.contains("selector") && data["selector"].is_object()) {
        (void)m_selectionStrategy.loadFromJson(data["selector"]);
    } else {
        m_selectionStrategy.startOver();
    }

    // event_delay
    if (data.contains("event_delay") && data["event_delay"].is_number_integer()) {
        m_travelTimeInTicks = std::max(0, data["event_delay"].get<i32>());
    } else {
        m_travelTimeInTicks = 0;
    }

    // 对齐 MC 原版行为：从存档加载时 reloadVibrationParticle = true
    m_reloadVibrationParticle = true;

    return true;
}

} // namespace mc::gameevent
