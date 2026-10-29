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

#include "PanelStats.hpp"

#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockState.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <sstream>
#include <utility>

namespace mc::server::console {

namespace {

/// 百分比格式：整数部分 + 一位小数，0 与 100 也保持一致的小数位。
std::string _formatPercent(i64 value, i64 total)
{
    if (total <= 0) {
        return "";
    }
    const f64 ratio = static_cast<f64>(value) / static_cast<f64>(total) * 100.0;
    return fmt::format(" ({:.1f}%)", ratio);
}

} // namespace

std::string formatThousands(i64 value)
{
    const bool negative = value < 0;
    // 取绝对值时避免 i64 最小值溢出。
    const u64 magnitude = negative ? static_cast<u64>(-(value + 1)) + 1u : static_cast<u64>(value);

    std::string digits = std::to_string(magnitude);
    std::string result;
    result.reserve(digits.size() + digits.size() / 3 + 1);
    if (negative) {
        result.push_back('-');
    }
    for (usize index = 0; index < digits.size(); ++index) {
        if (index > 0 && (digits.size() - index) % 3 == 0) {
            result.push_back(',');
        }
        result.push_back(digits[index]);
    }
    return result;
}

std::string stripNamespace(std::string_view id)
{
    constexpr std::string_view NAMESPACE_PREFIX = "minecraft:";
    if (id.starts_with(NAMESPACE_PREFIX)) {
        return std::string(id.substr(NAMESPACE_PREFIX.size()));
    }
    return std::string(id);
}

std::vector<std::string> formatRankedCounts(
    std::vector<std::pair<std::string, i64>> counts, usize maxEntries, i64 total)
{
    std::vector<std::string> lines;
    if (counts.empty() || maxEntries == 0) {
        if (counts.empty()) {
            lines.emplace_back(" (none)");
        }
        return lines;
    }

    // 数量降序；数量相同时按名称升序，保证刷新之间顺序稳定。
    std::sort(counts.begin(), counts.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second != rhs.second) {
            return lhs.second > rhs.second;
        }
        return lhs.first < rhs.first;
    });

    const usize shown = std::min(counts.size(), maxEntries);
    for (usize index = 0; index < shown; ++index) {
        const auto& [name, count] = counts[index];
        lines.push_back(fmt::format(" {} {}{}", name, formatThousands(count), _formatPercent(count, total)));
    }
    if (counts.size() > shown) {
        lines.push_back(fmt::format(" +{} more", counts.size() - shown));
    }
    return lines;
}

std::vector<std::string> formatChunkStatusLines(const ChunkStatusStats& stats)
{
    std::vector<std::string> lines;
    lines.push_back(" loaded  " + formatThousands(static_cast<i64>(stats.loadedChunks)));
    lines.push_back(" ticking " + formatThousands(static_cast<i64>(stats.tickingChunks)) +
        _formatPercent(static_cast<i64>(stats.tickingChunks), static_cast<i64>(stats.loadedChunks)));
    lines.push_back(" postpr  " + formatThousands(static_cast<i64>(stats.postProcessedChunks)) +
        _formatPercent(static_cast<i64>(stats.postProcessedChunks), static_cast<i64>(stats.loadedChunks)));
    lines.push_back(" entities(block-loaded) " + formatThousands(static_cast<i64>(stats.loadedEntities)));
    return lines;
}

std::string describeBlockState(const BlockState* state)
{
    if (state == nullptr) {
        return "unknown";
    }
    std::string result = stripNamespace(state->blockLocation().toString());
    const std::string modelKey = state->toModelKey();
    if (!modelKey.empty()) {
        result += "[";
        result += modelKey;
        result += "]";
    }
    return result;
}

std::string describeNbt(const ::mc::nbt::tags::tag& tag, usize maxLength)
{
    // operator<< 在 mc::nbt 命名空间内，经 ADL 找不到（形参是 std::ostream 与 mc::nbt::tags 的
    // 类型，后者位于更深一层命名空间）；显式引入后再配合 Contexts::mojangson 输出 SNBT。
    using ::mc::nbt::operator<<;

    std::ostringstream stream;
    stream << ::mc::nbt::contexts::mojangson << tag;
    std::string text = stream.str();

    // 面板一行放不下长标签：截断并给出省略标记，避免把整段 NBT 铺满屏幕。
    if (text.size() > maxLength && maxLength >= 3) {
        text.resize(maxLength - 3);
        text += "...";
    }
    return text;
}

} // namespace mc::server::console
