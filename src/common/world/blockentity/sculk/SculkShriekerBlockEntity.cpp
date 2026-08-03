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

#include "SculkShriekerBlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

#include <memory>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::blockentity {

// ============================================================================
// SculkShriekerBlockEntity
// ============================================================================

SculkShriekerBlockEntity::SculkShriekerBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::SculkShrieker, pos)
{}

// ============================================================================
// 序列化
// ============================================================================

bool SculkShriekerBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    // listener: 振动系统数据
    if (data.contains("listener") && data["listener"].is_object()) {
        if (!m_vibrationData.loadFromJson(data["listener"])) {
            m_vibrationData = gameevent::VibrationSystem::Data();
        }
    }

    // warning_level: 警告等级
    if (data.contains("warning_level") && data["warning_level"].is_number_integer()) {
        m_warningLevel = data["warning_level"].get<i32>();
    }

    return true;
}

void SculkShriekerBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    // listener: 振动系统数据
    nlohmann::json listenerData;
    m_vibrationData.saveToJson(listenerData);
    data["listener"] = std::move(listenerData);

    // warning_level: 警告等级
    data["warning_level"] = m_warningLevel;
}

bool SculkShriekerBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    // listener: 振动系统数据
    const nbt::CompoundTag* listenerTag = entity::serialization::nbt_helper::tryGetCompound(tag, "listener");
    if (listenerTag != nullptr) {
        if (!m_vibrationData.loadFromNBT(*listenerTag)) {
            m_vibrationData = gameevent::VibrationSystem::Data();
        }
    }

    // warning_level: 警告等级
    auto warningOpt = entity::serialization::nbt_helper::tryGetInt(tag, "warning_level");
    if (warningOpt.has_value()) {
        m_warningLevel = *warningOpt;
    }

    return true;
}

void SculkShriekerBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    // listener: 振动系统数据
    auto listenerTag = std::make_unique<nbt::CompoundTag>();
    m_vibrationData.saveToNBT(*listenerTag);
    tag.value.emplace("listener", std::move(listenerTag));

    // warning_level: 警告等级
    tag.put("warning_level", static_cast<i32>(m_warningLevel));
}

std::unique_ptr<BlockEntity> SculkShriekerBlockEntity::clone() const
{
    auto copy = std::make_unique<SculkShriekerBlockEntity>(m_pos);
    copy->m_vibrationData = m_vibrationData;
    copy->m_warningLevel = m_warningLevel;
    return copy;
}

} // namespace mc::blockentity
