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

#include "RespawnData.hpp"

#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

namespace {

constexpr const char* KEY_DIMENSION = "dimension";
constexpr const char* KEY_POS = "pos";
constexpr const char* KEY_YAW = "yaw";
constexpr const char* KEY_PITCH = "pitch";

} // namespace

RespawnData RespawnData::of(DimensionId dimension, const BlockPos& pos, f32 yaw, f32 pitch)
{
    RespawnData data;
    data.globalPos = GlobalPos(dimension, pos);
    data.yaw = math::wrapDegrees(yaw);
    data.pitch = math::clamp(pitch, -90.0f, 90.0f);
    return data;
}

void RespawnData::toNbt(nbt::tags::compound_tag& tag) const
{
    tag.put(KEY_DIMENSION, std::string(dimensionIdToString(globalPos.getDimensionId())));

    // 与原版 Codec 一致：BlockPos 以三元素 int 数组编码（兼容 int 列表形式的读取）。
    tag.value.insert_or_assign(KEY_POS,
        std::make_unique<nbt::tags::intarray_tag>(std::vector<i32>{globalPos.x(), globalPos.y(), globalPos.z()}));
    tag.put(KEY_YAW, yaw);
    tag.put(KEY_PITCH, pitch);
}

std::optional<RespawnData> RespawnData::fromNbt(const nbt::tags::compound_tag& tag)
{
    const auto dimensionName = entity::serialization::nbt_helper::tryGetString(tag, KEY_DIMENSION);
    if (!dimensionName.has_value()) {
        return std::nullopt;
    }

    i32 xyz[3] = {0, 0, 0};
    bool hasPos = false;
    if (const auto posIt = tag.value.find(KEY_POS); posIt != tag.value.end()) {
        if (posIt->second->id() == nbt::TagId::IntArray) {
            const auto& values = static_cast<const nbt::tags::intarray_tag&>(*posIt->second).value;
            if (values.size() >= 3) {
                xyz[0] = values[0];
                xyz[1] = values[1];
                xyz[2] = values[2];
                hasPos = true;
            }
        } else if (posIt->second->id() == nbt::TagId::List) {
            const auto values = entity::serialization::nbt_helper::getIntList(tag, KEY_POS);
            if (values.size() >= 3) {
                xyz[0] = values[0];
                xyz[1] = values[1];
                xyz[2] = values[2];
                hasPos = true;
            }
        }
    }
    if (!hasPos) {
        return std::nullopt;
    }

    // 非有限值（NaN/Inf）会让归一化产生 NaN，按缺失处理回退到 0。
    const auto finiteOr = [](std::optional<f32> value) -> f32 {
        return value.has_value() && std::isfinite(*value) ? *value : 0.0f;
    };
    const f32 yaw = finiteOr(entity::serialization::nbt_helper::tryGetFloat(tag, KEY_YAW));
    const f32 pitch = finiteOr(entity::serialization::nbt_helper::tryGetFloat(tag, KEY_PITCH));
    return RespawnData::of(dimensionNameToId(*dimensionName), BlockPos(xyz[0], xyz[1], xyz[2]), yaw, pitch);
}

namespace world::storage {

const RespawnData& LevelRespawnData::get() const
{
    return m_data;
}

void LevelRespawnData::set(const RespawnData& data)
{
    m_data = data;
}

} // namespace world::storage

} // namespace mc
