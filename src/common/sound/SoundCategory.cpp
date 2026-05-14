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

#include "common/sound/SoundCategory.hpp"

#include <algorithm>
#include <cctype>

namespace mc::sound {

namespace {

/**
 * @brief 声音类别名称表
 *
 * 索引与 SoundCategory 枚举值对应。
 * 名称与 MC Java 一致。
 */
constexpr std::string_view s_categoryNames[] = {
    "master",  // Master
    "music",   // Music
    "record",  // Records (注意: MC Java 使用单数形式 "record")
    "weather", // Weather
    "block",   // Blocks (注意: MC Java 使用单数形式 "block")
    "hostile", // Hostile
    "neutral", // Neutral
    "player",  // Players (注意: MC Java 使用单数形式 "player")
    "ambient", // Ambient
    "voice"    // Voice
};

static_assert(std::size(s_categoryNames) == static_cast<size_t>(SoundCategory::Count),
    "Category names array size must match SoundCategory::Count");

} // anonymous namespace

std::string_view getSoundCategoryName(SoundCategory category) noexcept
{
    const size_t index = static_cast<size_t>(category);
    if (index >= static_cast<size_t>(SoundCategory::Count)) {
        return "master"; // 默认返回 master
    }
    return s_categoryNames[index];
}

std::optional<SoundCategory> parseSoundCategory(std::string_view name) noexcept
{
    if (name.empty()) {
        return std::nullopt;
    }

    // 转换为小写进行比较
    auto toLower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };

    // 比较函数：不区分大小写
    auto equalsIgnoreCase = [toLower](std::string_view a, std::string_view b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (toLower(a[i]) != toLower(b[i])) return false;
        }
        return true;
    };

    // 遍历所有类别名称进行匹配
    for (size_t i = 0; i < std::size(s_categoryNames); ++i) {
        if (equalsIgnoreCase(name, s_categoryNames[i])) {
            return static_cast<SoundCategory>(i);
        }
    }

    // 特殊处理：支持复数形式的别名
    // MC Java 在某些地方使用复数，如 "blocks", "players", "records"
    if (equalsIgnoreCase(name, "blocks")) {
        return SoundCategory::Blocks;
    }
    if (equalsIgnoreCase(name, "players")) {
        return SoundCategory::Players;
    }
    if (equalsIgnoreCase(name, "records")) {
        return SoundCategory::Records;
    }

    return std::nullopt;
}

} // namespace mc::sound
