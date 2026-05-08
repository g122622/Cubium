#include "TargetInfo.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

namespace mc::client::ui::minecraft::targetinfo {

TargetInfoSnapshot::TargetInfoSnapshot(TargetInfoKind kind, std::string title, std::vector<std::string> details, u32 accentColor)
    : m_kind(kind)
    , m_title(std::move(title))
    , m_details(std::move(details))
    , m_accentColor(accentColor)
{
}

TargetInfoSnapshot TargetInfoSnapshot::none() {
    return TargetInfoSnapshot(TargetInfoKind::None, std::string{}, {}, 0);
}

namespace {

[[nodiscard]] bool isSeparator(char ch) {
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

std::string humanizeIdentifier(std::string_view identifier) {
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

        const bool shouldInsertSpace =
            !result.empty() &&
            !capitalizeNext &&
            std::isupper(static_cast<unsigned char>(ch)) &&
            index + 1 < identifier.size() &&
            std::islower(static_cast<unsigned char>(identifier[index + 1])) &&
            result.back() != ' ';

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

std::string humanizeResourceLocation(const ResourceLocation& location) {
    if (!location.path().empty()) {
        return humanizeIdentifier(location.path());
    }

    return humanizeIdentifier(location.namespace_());
}

std::string formatDistance(f32 distance) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << distance << " m";
    return stream.str();
}

std::string formatBlockPos(const BlockPos& pos) {
    return std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z);
}

std::string formatDirection(Direction direction) {
    switch (direction) {
        case Direction::Down: return "Down";
        case Direction::Up: return "Up";
        case Direction::North: return "North";
        case Direction::South: return "South";
        case Direction::West: return "West";
        case Direction::East: return "East";
        case Direction::None:
        default:
            return "Unknown";
    }
}

} // namespace mc::client::ui::minecraft::targetinfo