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

#include "TestInstanceBlockEntity.hpp"

#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <utility>

namespace mc {
namespace blockentity {

namespace {

/// 状态名（对齐 vanilla Status.getSerializedName）
const char* _statusName(TestInstanceBlockEntity::Status status)
{
    switch (status) {
        case TestInstanceBlockEntity::Status::Cleared:
            return "cleared";
        case TestInstanceBlockEntity::Status::Running:
            return "running";
        case TestInstanceBlockEntity::Status::Finished:
            return "finished";
        default:
            return "cleared";
    }
}

/// 字符串转状态（对齐 vanilla Status.CODEC）
TestInstanceBlockEntity::Status _statusFromName(const std::string& name)
{
    if (name == "running") {
        return TestInstanceBlockEntity::Status::Running;
    }
    if (name == "finished") {
        return TestInstanceBlockEntity::Status::Finished;
    }
    return TestInstanceBlockEntity::Status::Cleared;
}

} // namespace

TestInstanceBlockEntity::TestInstanceBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::TestInstanceBlock, pos)
{}

BlockPos TestInstanceBlockEntity::getStructurePos() const
{
    return BlockPos(m_pos.x + STRUCTURE_OFFSET_X, m_pos.y + STRUCTURE_OFFSET_Y, m_pos.z + STRUCTURE_OFFSET_Z);
}

void TestInstanceBlockEntity::setSuccess()
{
    m_status = Status::Finished;
    m_errorMessage.clear();
    setChanged();
}

void TestInstanceBlockEntity::setRunning()
{
    m_status = Status::Running;
    m_errorMessage.clear();
    setChanged();
}

void TestInstanceBlockEntity::markError(const BlockPos& pos, const std::string& text)
{
    m_errorMarkers.push_back(ErrorMarker{pos, text});
    setChanged();
}

void TestInstanceBlockEntity::clearErrorMarkers()
{
    if (!m_errorMarkers.empty()) {
        m_errorMarkers.clear();
        setChanged();
    }
}

bool TestInstanceBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    if (data.contains("test") && data["test"].is_string()) {
        m_testId = data["test"].get<std::string>();
    }
    if (data.contains("size") && data["size"].is_array() && data["size"].size() == 3) {
        m_sizeX = data["size"][0].get<i32>();
        m_sizeY = data["size"][1].get<i32>();
        m_sizeZ = data["size"][2].get<i32>();
    }
    if (data.contains("rotation") && data["rotation"].is_string()) {
        // 结构旋转值名（none/clockwise_90/180/counterclockwise_90）与方向名不同，单独解析
        const std::string rotName = data["rotation"].get<std::string>();
        if (rotName == "clockwise_90") {
            m_rotation = Rotation::Clockwise90;
        } else if (rotName == "180") {
            m_rotation = Rotation::Clockwise180;
        } else if (rotName == "counterclockwise_90") {
            m_rotation = Rotation::CounterClockwise90;
        } else {
            m_rotation = Rotation::None;
        }
    }
    if (data.contains("ignore_entities") && data["ignore_entities"].is_boolean()) {
        m_ignoreEntities = data["ignore_entities"].get<bool>();
    }
    if (data.contains("status") && data["status"].is_string()) {
        m_status = _statusFromName(data["status"].get<std::string>());
    }
    if (data.contains("error_message") && data["error_message"].is_string()) {
        m_errorMessage = data["error_message"].get<std::string>();
    }

    m_errorMarkers.clear();
    if (data.contains("errors") && data["errors"].is_array()) {
        for (const auto& entry : data["errors"]) {
            if (!entry.is_object() || !entry.contains("pos") || !entry.contains("text")) {
                continue;
            }
            const auto& posJson = entry["pos"];
            if (!posJson.is_array() || posJson.size() != 3) {
                continue;
            }
            ErrorMarker marker;
            marker.pos = BlockPos(posJson[0].get<i32>(), posJson[1].get<i32>(), posJson[2].get<i32>());
            marker.text = entry["text"].get<std::string>();
            m_errorMarkers.push_back(std::move(marker));
        }
    }

    return true;
}

void TestInstanceBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["test"] = m_testId;
    data["size"] = nlohmann::json::array({m_sizeX, m_sizeY, m_sizeZ});
    switch (m_rotation) {
        case Rotation::Clockwise90:
            data["rotation"] = "clockwise_90";
            break;
        case Rotation::Clockwise180:
            data["rotation"] = "180";
            break;
        case Rotation::CounterClockwise90:
            data["rotation"] = "counterclockwise_90";
            break;
        default:
            data["rotation"] = "none";
            break;
    }
    data["ignore_entities"] = m_ignoreEntities;
    data["status"] = _statusName(m_status);
    if (!m_errorMessage.empty()) {
        data["error_message"] = m_errorMessage;
    }

    if (!m_errorMarkers.empty()) {
        nlohmann::json errors = nlohmann::json::array();
        for (const auto& marker : m_errorMarkers) {
            errors.push_back(nlohmann::json{{"pos", {marker.pos.x, marker.pos.y, marker.pos.z}}, {"text", marker.text}});
        }
        data["errors"] = std::move(errors);
    }
}

bool TestInstanceBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    auto testOpt = entity::serialization::nbt_helper::tryGetString(tag, "test");
    if (testOpt.has_value()) {
        m_testId = testOpt.value();
    }
    auto statusOpt = entity::serialization::nbt_helper::tryGetString(tag, "status");
    if (statusOpt.has_value()) {
        m_status = _statusFromName(statusOpt.value());
    }
    auto errorOpt = entity::serialization::nbt_helper::tryGetString(tag, "error_message");
    if (errorOpt.has_value()) {
        m_errorMessage = errorOpt.value();
    }
    auto ignoreOpt = entity::serialization::nbt_helper::tryGetBool(tag, "ignore_entities");
    if (ignoreOpt.has_value()) {
        m_ignoreEntities = ignoreOpt.value();
    }
    // 结构尺寸与旋转以独立标量键存储（与 saveToNBT 对称），尺寸缺失时保留默认 0
    m_sizeX = entity::serialization::nbt_helper::tryGetInt(tag, "size_x").value_or(m_sizeX);
    m_sizeY = entity::serialization::nbt_helper::tryGetInt(tag, "size_y").value_or(m_sizeY);
    m_sizeZ = entity::serialization::nbt_helper::tryGetInt(tag, "size_z").value_or(m_sizeZ);
    auto rotationOpt = entity::serialization::nbt_helper::tryGetString(tag, "rotation");
    if (rotationOpt.has_value()) {
        const std::string& rotName = rotationOpt.value();
        if (rotName == "clockwise_90") {
            m_rotation = Rotation::Clockwise90;
        } else if (rotName == "180") {
            m_rotation = Rotation::Clockwise180;
        } else if (rotName == "counterclockwise_90") {
            m_rotation = Rotation::CounterClockwise90;
        } else {
            m_rotation = Rotation::None;
        }
    }
    return true;
}

void TestInstanceBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    tag.put("test", m_testId);
    tag.put("size_x", m_sizeX);
    tag.put("size_y", m_sizeY);
    tag.put("size_z", m_sizeZ);
    switch (m_rotation) {
        case Rotation::Clockwise90:
            tag.put("rotation", std::string("clockwise_90"));
            break;
        case Rotation::Clockwise180:
            tag.put("rotation", std::string("180"));
            break;
        case Rotation::CounterClockwise90:
            tag.put("rotation", std::string("counterclockwise_90"));
            break;
        default:
            tag.put("rotation", std::string("none"));
            break;
    }
    tag.put("ignore_entities", static_cast<i8>(m_ignoreEntities ? 1 : 0));
    tag.put("status", std::string(_statusName(m_status)));
    if (!m_errorMessage.empty()) {
        tag.put("error_message", m_errorMessage);
    }
}

std::unique_ptr<BlockEntity> TestInstanceBlockEntity::clone() const
{
    auto copy = std::make_unique<TestInstanceBlockEntity>(m_pos);
    copy->m_testId = m_testId;
    copy->m_sizeX = m_sizeX;
    copy->m_sizeY = m_sizeY;
    copy->m_sizeZ = m_sizeZ;
    copy->m_rotation = m_rotation;
    copy->m_ignoreEntities = m_ignoreEntities;
    copy->m_status = m_status;
    copy->m_errorMessage = m_errorMessage;
    copy->m_errorMarkers = m_errorMarkers;
    return copy;
}

} // namespace blockentity
} // namespace mc
