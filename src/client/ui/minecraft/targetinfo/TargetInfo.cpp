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

#include "TargetInfo.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"

#include <cctype>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::client::ui::minecraft::targetinfo {

TargetInfoSnapshot::TargetInfoSnapshot(
    TargetInfoKind kind, std::string title, std::vector<std::string> details, u32 accentColor)
    : m_kind(kind)
    , m_title(std::move(title))
    , m_details(std::move(details))
    , m_accentColor(accentColor)
{}

TargetInfoSnapshot TargetInfoSnapshot::none()
{
    return TargetInfoSnapshot(TargetInfoKind::None, std::string{}, {}, 0);
}

namespace {

/** @brief 判断字符是否为标识符中的分隔符（下划线、短横线、斜杠、冒号、点号） */
[[nodiscard]] bool isSeparator(char ch)
{
    switch (ch) {
        case '_':
        case '-':
        case '/':
        case ':':
        case '.':
            return true;
        default:
            return false;
    }
}

} // namespace

std::string humanizeIdentifier(std::string_view identifier)
{
    std::string result;
    result.reserve(identifier.size());

    // 标记下一个字母是否需要大写（句首或分隔符后）
    bool capitalizeNext = true;
    for (size_t index = 0; index < identifier.size(); ++index) {
        const char ch = identifier[index];

        // 分隔符替换为空格，并标记下一个字母需要大写
        if (isSeparator(ch)) {
            if (!result.empty() && result.back() != ' ') {
                result.push_back(' ');
            }
            capitalizeNext = true;
            continue;
        }

        // 检测驼峰命名边界：当前大写、下一个小写，说明这是新单词的开始
        // 例如 "ironIngot" 中的 'I' 需要在前面插入空格
        const bool shouldInsertSpace = !result.empty() && !capitalizeNext &&
            std::isupper(static_cast<unsigned char>(ch)) && index + 1 < identifier.size() &&
            std::islower(static_cast<unsigned char>(identifier[index + 1])) && result.back() != ' ';

        if (shouldInsertSpace) {
            result.push_back(' ');
            capitalizeNext = true;
        }

        if (capitalizeNext && std::isalpha(static_cast<unsigned char>(ch))) {
            result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        } else {
            result.push_back(ch);
        }
        capitalizeNext = false;
    }

    // 移除末尾可能残留的空格
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result;
}

std::string humanizeResourceLocation(const ResourceLocation& location)
{
    // 优先使用路径部分（冒号后面的内容），回退到命名空间
    if (!location.path().empty()) {
        return humanizeIdentifier(location.path());
    }

    return humanizeIdentifier(location.namespace_());
}

std::string formatDistance(f32 distance)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << distance << " m";
    return stream.str();
}

std::string formatBlockPos(const BlockPos& pos)
{
    return std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z);
}

std::string formatDirection(Direction direction)
{
    switch (direction) {
        case Direction::Down:
            return "Down";
        case Direction::Up:
            return "Up";
        case Direction::North:
            return "North";
        case Direction::South:
            return "South";
        case Direction::West:
            return "West";
        case Direction::East:
            return "East";
        case Direction::None:
        default:
            return "Unknown";
    }
}

} // namespace mc::client::ui::minecraft::targetinfo