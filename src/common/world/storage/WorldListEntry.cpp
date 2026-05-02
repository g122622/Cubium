#include "WorldListEntry.hpp"
#include "common/util/StringUtils.hpp"
#include <algorithm>

namespace mc::world::storage {

WorldListEntry::WorldListEntry(
    std::string levelId,
    std::filesystem::path worldDir,
    std::string displayName,
    i64 lastPlayedMs,
    u64 seed,
    WorldType worldType,
    GameMode gameMode,
    Difficulty difficulty,
    bool hardcore,
    bool allowCommands,
    bool locked,
    bool requiresConversion,
    WorldCompatibility compatibility,
    std::string versionName,
    i32 dataVersion,
    std::filesystem::path iconPath,
    std::string errorMessage
)
    : levelId(std::move(levelId))
    , worldDir(std::move(worldDir))
    , displayName(std::move(displayName))
    , lastPlayedMs(lastPlayedMs)
    , seed(seed)
    , worldType(worldType)
    , gameMode(gameMode)
    , difficulty(difficulty)
    , hardcore(hardcore)
    , allowCommands(allowCommands)
    , locked(locked)
    , requiresConversion(requiresConversion)
    , compatibility(compatibility)
    , versionName(std::move(versionName))
    , dataVersion(dataVersion)
    , iconPath(std::move(iconPath))
    , errorMessage(std::move(errorMessage))
{
}

bool WorldListEntry::operator<(const WorldListEntry& other) const noexcept
{
    // 首先按最后游玩时间降序
    if (lastPlayedMs != other.lastPlayedMs) {
        return lastPlayedMs > other.lastPlayedMs;
    }
    // 时间相同则按 levelId 升序
    return levelId < other.levelId;
}

void sortWorldEntries(std::vector<WorldListEntry>& entries)
{
    std::sort(entries.begin(), entries.end());
}

std::vector<WorldListEntry> filterWorldEntries(
    const std::vector<WorldListEntry>& entries,
    const std::string& searchQuery
)
{
    if (searchQuery.empty()) {
        return entries;
    }

    // 不区分大小写的搜索
    String lowerQuery = util::toLowerAscii(searchQuery);

    std::vector<WorldListEntry> result;
    result.reserve(entries.size());

    for (const auto& entry : entries) {
        String lowerDisplayName = util::toLowerAscii(entry.displayName);
        String lowerLevelId = util::toLowerAscii(entry.levelId);

        if (lowerDisplayName.find(lowerQuery) != String::npos ||
            lowerLevelId.find(lowerQuery) != String::npos) {
            result.push_back(entry);
        }
    }

    return result;
}

} // namespace mc::world::storage
