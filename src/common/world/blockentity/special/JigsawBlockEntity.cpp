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

#include "JigsawBlockEntity.hpp"

#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"

#include <string>

namespace mc::blockentity {

namespace {

using world::gen::jigsaw::JigsawJointType;

/// 连接类型 NBT 名称（对齐 vanilla JointType 序列化名）
[[nodiscard]] const char* _jointName(JigsawJointType joint)
{
    return joint == JigsawJointType::Aligned ? "aligned" : "rollable";
}

[[nodiscard]] JigsawJointType _parseJoint(const std::string& name)
{
    return name == "aligned" ? JigsawJointType::Aligned : JigsawJointType::Rollable;
}

} // namespace

JigsawBlockEntity::JigsawBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::JigsawBlock, pos)
{}

Direction JigsawBlockEntity::frontFacing() const
{
    const BlockState* state = getBlockState();
    if (state == nullptr || !state->hasProperty(BlockStateProperties::ORIENTATION())) {
        return Direction::North;
    }
    return world::gen::jigsaw::JigsawOrientations::getFacing(state->get(BlockStateProperties::ORIENTATION()));
}

JigsawJointType JigsawBlockEntity::defaultJointType() const
{
    // 对齐 vanilla StructureTemplate.getDefaultJointType：
    //   front 方向为水平 → ALIGNED；竖直 → ROLLABLE。
    const Direction front = frontFacing();
    switch (front) {
        case Direction::North:
        case Direction::South:
        case Direction::East:
        case Direction::West:
            return JigsawJointType::Aligned;
        default:
            return JigsawJointType::Rollable;
    }
}

// ========== NBT ==========

bool JigsawBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    namespace nh = mc::entity::serialization::nbt_helper;

    m_name = nh::tryGetString(tag, "name").value_or(EMPTY_ID);
    m_target = nh::tryGetString(tag, "target").value_or(EMPTY_ID);
    m_pool = nh::tryGetString(tag, "pool").value_or("minecraft:empty");
    m_finalState = nh::tryGetString(tag, "final_state").value_or(DEFAULT_FINAL_STATE);
    // 缺失 joint 时按前方朝向推断默认值（对齐 vanilla loadAdditional）。
    if (auto jointName = nh::tryGetString(tag, "joint")) {
        m_joint = _parseJoint(*jointName);
    } else {
        m_joint = defaultJointType();
    }
    m_placementPriority = nh::tryGetInt(tag, "placement_priority").value_or(0);
    m_selectionPriority = nh::tryGetInt(tag, "selection_priority").value_or(0);
    return true;
}

void JigsawBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    tag.put("name", m_name);
    tag.put("target", m_target);
    tag.put("pool", m_pool);
    tag.put("final_state", m_finalState);
    tag.put("joint", std::string(_jointName(m_joint)));
    tag.put("placement_priority", m_placementPriority);
    tag.put("selection_priority", m_selectionPriority);
}

void JigsawBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["name"] = m_name;
    data["target"] = m_target;
    data["pool"] = m_pool;
    data["final_state"] = m_finalState;
    data["joint"] = _jointName(m_joint);
    data["placement_priority"] = m_placementPriority;
    data["selection_priority"] = m_selectionPriority;
}

std::unique_ptr<BlockEntity> JigsawBlockEntity::clone() const
{
    auto copy = std::make_unique<JigsawBlockEntity>(m_pos);
    copy->m_name = m_name;
    copy->m_target = m_target;
    copy->m_pool = m_pool;
    copy->m_finalState = m_finalState;
    copy->m_joint = m_joint;
    copy->m_placementPriority = m_placementPriority;
    copy->m_selectionPriority = m_selectionPriority;
    return copy;
}

} // namespace mc::blockentity