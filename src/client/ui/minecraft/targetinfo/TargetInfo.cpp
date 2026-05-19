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

#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

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

    bool capitalizeNext = true;
    for (size_t index = 0; index < identifier.size(); ++index) {
        const char ch = identifier[index];

        if (isSeparator(ch)) {
            if (!result.empty() && result.back() != ' ') {
                result.push_back(' ');
            }
            capitalizeNext = true;
            continue;
        }

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

    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result;
}

std::string humanizeResourceLocation(const ResourceLocation& location)
{
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