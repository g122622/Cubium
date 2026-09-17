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

#include "WorldListEntry.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/StringUtils.hpp"
#include "common/world/WorldConfig.hpp"
#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace mc::world::storage {

WorldListEntry::WorldListEntry(std::string levelId,
    std::filesystem::path worldDir,
    std::string displayName,
    i64 lastPlayedMs,
    u64 seed,
    WorldType worldType,
    resource::ResourceLocation worldPresetId,
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
    std::string errorMessage)
    : levelId(std::move(levelId))
    , worldDir(std::move(worldDir))
    , displayName(std::move(displayName))
    , lastPlayedMs(lastPlayedMs)
    , seed(seed)
    , worldType(worldType)
    , worldPresetId(std::move(worldPresetId))
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
{}

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
    const std::vector<WorldListEntry>& entries, const std::string& searchQuery)
{
    if (searchQuery.empty()) {
        return entries;
    }

    // 不区分大小写的搜索
    const std::string lowerQuery = util::toLowerAscii(searchQuery);

    std::vector<WorldListEntry> result;
    result.reserve(entries.size());

    for (const auto& entry : entries) {
        const std::string lowerDisplayName = util::toLowerAscii(entry.displayName);
        const std::string lowerLevelId = util::toLowerAscii(entry.levelId);

        if (lowerDisplayName.find(lowerQuery) != std::string::npos ||
            lowerLevelId.find(lowerQuery) != std::string::npos) {
            result.push_back(entry);
        }
    }

    return result;
}

} // namespace mc::world::storage
