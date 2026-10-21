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

#include "StructureBlockEntity.hpp"

#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"

#include <algorithm>
#include <string>

namespace mc::blockentity {

namespace {

using StructureMode = BlockStateProperties::StructureMode;

/// 模式 NBT 名称（对齐 vanilla StructureMode.LEGACY_CODEC 的序列化名）
[[nodiscard]] const char* _modeName(StructureMode mode)
{
    switch (mode) {
        case StructureMode::Save:
            return "SAVE";
        case StructureMode::Load:
            return "LOAD";
        case StructureMode::Corner:
            return "CORNER";
        case StructureMode::Data:
            return "DATA";
    }
    return "DATA";
}

/// 由 NBT 名称解析模式（无法识别时回退 DATA，对齐 vanilla LEGACY_CODEC 的默认值）
[[nodiscard]] StructureMode _parseMode(const std::string& name)
{
    if (name == "SAVE") {
        return StructureMode::Save;
    }
    if (name == "LOAD") {
        return StructureMode::Load;
    }
    if (name == "CORNER") {
        return StructureMode::Corner;
    }
    return StructureMode::Data;
}

/// 旋转 NBT 名称（对齐 vanilla Rotation.LEGACY_CODEC）
[[nodiscard]] const char* _rotationName(Rotation rotation)
{
    switch (rotation) {
        case Rotation::None:
            return "none";
        case Rotation::Clockwise90:
            return "clockwise_90";
        case Rotation::Clockwise180:
            return "180";
        case Rotation::CounterClockwise90:
            return "counterclockwise_90";
    }
    return "none";
}

[[nodiscard]] Rotation _parseRotation(const std::string& name)
{
    if (name == "clockwise_90") {
        return Rotation::Clockwise90;
    }
    if (name == "180") {
        return Rotation::Clockwise180;
    }
    if (name == "counterclockwise_90") {
        return Rotation::CounterClockwise90;
    }
    return Rotation::None;
}

/// 镜像 NBT 名称（对齐 vanilla Mirror.LEGACY_CODEC）
[[nodiscard]] const char* _mirrorName(Mirror mirror)
{
    switch (mirror) {
        case Mirror::None:
            return "none";
        case Mirror::LeftRight:
            return "left_right";
        case Mirror::FrontBack:
            return "front_back";
    }
    return "none";
}

[[nodiscard]] Mirror _parseMirror(const std::string& name)
{
    if (name == "left_right") {
        return Mirror::LeftRight;
    }
    if (name == "front_back") {
        return Mirror::FrontBack;
    }
    return Mirror::None;
}

} // namespace

StructureBlockEntity::StructureBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::StructureBlock, pos)
{}

StructureMode StructureBlockEntity::_mode() const
{
    // 模式以方块状态的 STRUCTURE_MODE 属性为权威来源（对齐 vanilla：模式同时写入状态与实体，
    // 状态在方块被替换/放置时由放置逻辑设置，实体仅镜像）。
    const BlockState* state = getBlockState();
    if (state == nullptr) {
        return StructureMode::Data;
    }
    return state->get(BlockStateProperties::STRUCTURE_MODE());
}

void StructureBlockEntity::setStructureName(const std::string& name)
{
    m_structureName = name;
    setChanged();
}

void StructureBlockEntity::setStructurePos(const BlockPos& pos)
{
    // 对齐 vanilla：每轴偏移钳制在 [-48, 48]。
    m_structurePos = BlockPos(std::clamp(pos.x, -MAX_OFFSET_PER_AXIS, MAX_OFFSET_PER_AXIS),
        std::clamp(pos.y, -MAX_OFFSET_PER_AXIS, MAX_OFFSET_PER_AXIS),
        std::clamp(pos.z, -MAX_OFFSET_PER_AXIS, MAX_OFFSET_PER_AXIS));
    setChanged();
}

void StructureBlockEntity::setStructureSize(const BlockPos& size)
{
    // 对齐 vanilla：每轴尺寸钳制在 [0, 48]。
    m_structureSize = BlockPos(std::clamp(size.x, 0, MAX_SIZE_PER_AXIS),
        std::clamp(size.y, 0, MAX_SIZE_PER_AXIS),
        std::clamp(size.z, 0, MAX_SIZE_PER_AXIS));
    setChanged();
}

void StructureBlockEntity::setIntegrity(f32 integrity)
{
    m_integrity = std::clamp(integrity, 0.0f, 1.0f);
    setChanged();
}

// ========== NBT ==========

bool StructureBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    namespace nh = mc::entity::serialization::nbt_helper;

    if (auto v = nh::tryGetString(tag, "name")) {
        setStructureName(*v);
    }
    if (auto v = nh::tryGetString(tag, "author")) {
        m_author = *v;
    }
    if (auto v = nh::tryGetString(tag, "metadata")) {
        m_metaData = *v;
    }

    const i32 posX = nh::tryGetInt(tag, "posX").value_or(m_structurePos.x);
    const i32 posY = nh::tryGetInt(tag, "posY").value_or(m_structurePos.y);
    const i32 posZ = nh::tryGetInt(tag, "posZ").value_or(m_structurePos.z);
    setStructurePos(BlockPos(posX, posY, posZ));

    const i32 sizeX = nh::tryGetInt(tag, "sizeX").value_or(m_structureSize.x);
    const i32 sizeY = nh::tryGetInt(tag, "sizeY").value_or(m_structureSize.y);
    const i32 sizeZ = nh::tryGetInt(tag, "sizeZ").value_or(m_structureSize.z);
    setStructureSize(BlockPos(sizeX, sizeY, sizeZ));

    if (auto v = nh::tryGetString(tag, "rotation")) {
        m_rotation = _parseRotation(*v);
    }
    if (auto v = nh::tryGetString(tag, "mirror")) {
        m_mirror = _parseMirror(*v);
    }
    m_ignoreEntities = nh::tryGetBool(tag, "ignoreEntities").value_or(true);
    m_strict = nh::tryGetBool(tag, "strict").value_or(false);
    m_showAir = nh::tryGetBool(tag, "showair").value_or(false);
    m_showBoundingBox = nh::tryGetBool(tag, "showboundingbox").value_or(true);
    m_integrity = std::clamp(nh::tryGetFloat(tag, "integrity").value_or(1.0f), 0.0f, 1.0f);
    m_seed = nh::tryGetLong(tag, "seed").value_or(0);

    // 模式来自 NBT 时同步写回方块状态（对齐 vanilla loadAdditional → updateBlockState）。
    // 结构方块模式与状态属性必须一致，故直接按 NBT 值更新方块状态。
    const StructureMode mode = _parseMode(nh::tryGetString(tag, "mode").value_or("DATA"));
    if (IWorld* worldPtr = getWorld(); worldPtr != nullptr) {
        if (const BlockState* state = worldPtr->getBlockState(m_pos); state != nullptr &&
            state->hasProperty(BlockStateProperties::STRUCTURE_MODE()) &&
            state->get(BlockStateProperties::STRUCTURE_MODE()) != mode) {
            worldPtr->setBlockState(m_pos, &state->with(BlockStateProperties::STRUCTURE_MODE(), mode));
        }
    }
    return true;
}

void StructureBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    tag.put("name", m_structureName);
    tag.put("author", m_author);
    tag.put("metadata", m_metaData);
    tag.put("posX", m_structurePos.x);
    tag.put("posY", m_structurePos.y);
    tag.put("posZ", m_structurePos.z);
    tag.put("sizeX", m_structureSize.x);
    tag.put("sizeY", m_structureSize.y);
    tag.put("sizeZ", m_structureSize.z);
    tag.put("rotation", std::string(_rotationName(m_rotation)));
    tag.put("mirror", std::string(_mirrorName(m_mirror)));
    tag.put("mode", std::string(_modeName(_mode())));
    tag.put("ignoreEntities", static_cast<i8>(m_ignoreEntities ? 1 : 0));
    tag.put("strict", static_cast<i8>(m_strict ? 1 : 0));
    tag.put("showair", static_cast<i8>(m_showAir ? 1 : 0));
    tag.put("showboundingbox", static_cast<i8>(m_showBoundingBox ? 1 : 0));
    tag.put("integrity", m_integrity);
    tag.put("seed", m_seed);
}

void StructureBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["name"] = m_structureName;
    data["author"] = m_author;
    data["metadata"] = m_metaData;
    data["posX"] = m_structurePos.x;
    data["posY"] = m_structurePos.y;
    data["posZ"] = m_structurePos.z;
    data["sizeX"] = m_structureSize.x;
    data["sizeY"] = m_structureSize.y;
    data["sizeZ"] = m_structureSize.z;
    data["rotation"] = _rotationName(m_rotation);
    data["mirror"] = _mirrorName(m_mirror);
    data["mode"] = _modeName(_mode());
    data["ignoreEntities"] = m_ignoreEntities;
    data["strict"] = m_strict;
    data["showair"] = m_showAir;
    data["showboundingbox"] = m_showBoundingBox;
    data["integrity"] = m_integrity;
    data["seed"] = m_seed;
}

std::unique_ptr<BlockEntity> StructureBlockEntity::clone() const
{
    auto copy = std::make_unique<StructureBlockEntity>(m_pos);
    copy->m_structureName = m_structureName;
    copy->m_author = m_author;
    copy->m_metaData = m_metaData;
    copy->m_structurePos = m_structurePos;
    copy->m_structureSize = m_structureSize;
    copy->m_mirror = m_mirror;
    copy->m_rotation = m_rotation;
    copy->m_ignoreEntities = m_ignoreEntities;
    copy->m_strict = m_strict;
    copy->m_showAir = m_showAir;
    copy->m_showBoundingBox = m_showBoundingBox;
    copy->m_integrity = m_integrity;
    copy->m_seed = m_seed;
    return copy;
}

} // namespace mc::blockentity