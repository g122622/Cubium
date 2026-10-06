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

#include "SkullBlockEntity.hpp"

#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <utility>

namespace mc {
namespace blockentity {

SkullBlockEntity::SkullBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Skull, pos)
{}

void SkullBlockEntity::setOwnerProfile(const skin::GameProfile& profile)
{
    m_owner = profile;
    setChanged();
}

void SkullBlockEntity::setCustomName(const std::string& name)
{
    m_customName = name;
    setChanged();
}

void SkullBlockEntity::tick(IWorld& world)
{
    MC_UNUSED(world);

    // 对齐 vanilla SkullBlockEntity.animation：POWERED 为真时累加动画计数并标记动画中
    const BlockState* state = world.getBlockState(m_pos);
    if (state != nullptr && state->get(BlockStateProperties::POWERED())) {
        m_isAnimating = true;
        ++m_animationTickCount;
    } else {
        m_isAnimating = false;
    }
}

bool SkullBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    if (data.contains("custom_name") && data["custom_name"].is_string()) {
        m_customName = data["custom_name"].get<std::string>();
    }

    // 所有者档案（JSON 内联对象）
    if (data.contains("profile") && data["profile"].is_object()) {
        auto result = skin::GameProfile::fromJson(data["profile"]);
        if (result.success()) {
            m_owner = result.value();
        }
    }

    return true;
}

void SkullBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    if (m_owner.has_value()) {
        data["profile"] = m_owner->toJson();
    }
    if (!m_customName.empty()) {
        data["custom_name"] = m_customName;
    }
}

bool SkullBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    auto nameOpt = entity::serialization::nbt_helper::tryGetString(tag, "CustomName");
    if (nameOpt.has_value()) {
        m_customName = nameOpt.value();
    }

    // 所有者档案：NBT 中以 CompoundTag 形式存储（UUID/Name/Properties）。
    // TODO: 结构模板中的头颅所有者档案反序列化待 GameProfile 的 NBT 复合标签映射接入后补全，
    //   当前 NBT 路径仅恢复自定义名称（JSON 存档路径 load/save 已完整支持档案往返）。
    const auto* profileTag = entity::serialization::nbt_helper::tryGetCompound(tag, "profile");
    if (profileTag != nullptr) {
        auto uuidStr = entity::serialization::nbt_helper::tryGetString(*profileTag, "Id");
        auto nameStr = entity::serialization::nbt_helper::tryGetString(*profileTag, "Name");
        if (uuidStr.has_value()) {
            skin::GameProfile profile;
            profile.setUUID(skin::GameProfile::parseUUID(uuidStr.value()));
            profile.setName(nameStr.value_or(""));
            m_owner = profile;
        }
    }

    return true;
}

void SkullBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    if (!m_customName.empty()) {
        tag.put("CustomName", m_customName);
    }

    if (m_owner.has_value()) {
        auto profileTag = std::make_unique<nbt::tags::compound_tag>();
        profileTag->put("Id", m_owner->uuidToString());
        profileTag->put("Name", m_owner->name());
        tag.value.emplace("profile", std::move(profileTag));
    }
}

std::unique_ptr<BlockEntity> SkullBlockEntity::clone() const
{
    auto copy = std::make_unique<SkullBlockEntity>(m_pos);
    copy->m_owner = m_owner;
    copy->m_customName = m_customName;
    copy->m_animationTickCount = m_animationTickCount;
    copy->m_isAnimating = m_isAnimating;
    return copy;
}

} // namespace blockentity
} // namespace mc
