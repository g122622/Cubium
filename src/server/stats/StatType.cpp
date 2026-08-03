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

#include "server/stats/StatType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace mc {
namespace server {
namespace stats {

std::optional<StatType> parseStatType(std::string_view prefix) noexcept
{
    if (prefix == "mined") {
        return StatType::Mined;
    } else if (prefix == "crafted") {
        return StatType::Crafted;
    } else if (prefix == "used") {
        return StatType::Used;
    } else if (prefix == "broken") {
        return StatType::Broken;
    } else if (prefix == "picked_up") {
        return StatType::PickedUp;
    } else if (prefix == "dropped") {
        return StatType::Dropped;
    } else if (prefix == "killed") {
        return StatType::Killed;
    } else if (prefix == "killed_by") {
        return StatType::KilledBy;
    } else if (prefix == "custom") {
        return StatType::Custom;
    }
    return std::nullopt;
}

ResourceLocation buildStatLocation(StatType type, const ResourceLocation& id)
{
    std::string prefix(getStatTypePrefix(type));
    std::string fullId = "minecraft." + prefix + ":" + id.toString();
    return ResourceLocation(fullId);
}

} // namespace stats
} // namespace server
} // namespace mc
