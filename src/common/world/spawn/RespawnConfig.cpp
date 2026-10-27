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

#include "RespawnConfig.hpp"

#include "common/entity/serialization/NbtHelper.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include <string>

namespace mc {

namespace {

constexpr const char* KEY_FORCED = "forced";

} // namespace

DimensionId RespawnConfig::getDimensionOrDefault(const RespawnConfig* config)
{
    return config != nullptr ? config->respawnData.dimension() : DimensionManager::OVERWORLD;
}

bool RespawnConfig::isSamePosition(const std::optional<RespawnConfig>& other) const noexcept
{
    return other.has_value() && respawnData.isSamePosition(other->respawnData);
}

void RespawnConfig::toNbt(nbt::tags::compound_tag& tag) const
{
    respawnData.toNbt(tag);
    // forced 在 NBT 中按可选字段编码：false 时不写，读取时缺省为 false。
    if (forced) {
        tag.put(KEY_FORCED, static_cast<i8>(1));
    }
}

std::optional<RespawnConfig> RespawnConfig::fromNbt(const nbt::tags::compound_tag& tag)
{
    const auto respawnData = RespawnData::fromNbt(tag);
    if (!respawnData.has_value()) {
        return std::nullopt;
    }

    RespawnConfig config;
    config.respawnData = *respawnData;
    config.forced = entity::serialization::nbt_helper::tryGetBool(tag, KEY_FORCED).value_or(false);
    return config;
}

} // namespace mc
