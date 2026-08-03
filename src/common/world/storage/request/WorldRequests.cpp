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

#include "WorldRequests.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/WorldConfig.hpp"
#include <filesystem>
#include <string>
#include <utility>

namespace mc::world::storage {

CreateWorldRequest::CreateWorldRequest(std::string displayName,
    std::string requestedLevelId,
    u64 seed,
    WorldType worldType,
    resource::ResourceLocation worldPresetId,
    GameMode gameMode,
    Difficulty difficulty,
    bool hardcore,
    bool allowCommands,
    i32 viewDistance)
    : displayName(std::move(displayName))
    , requestedLevelId(std::move(requestedLevelId))
    , seed(seed)
    , worldType(worldType)
    , worldPresetId(std::move(worldPresetId))
    , gameMode(gameMode)
    , difficulty(difficulty)
    , hardcore(hardcore)
    , allowCommands(allowCommands)
    , viewDistance(viewDistance)
{}

LoadWorldRequest::LoadWorldRequest(
    std::string levelId, bool allowFutureVersion, bool createBackupBeforeUpgrade, bool allowStorageConversion)
    : levelId(std::move(levelId))
    , allowFutureVersion(allowFutureVersion)
    , createBackupBeforeUpgrade(createBackupBeforeUpgrade)
    , allowStorageConversion(allowStorageConversion)
{}

RenameWorldRequest::RenameWorldRequest(std::string levelId, std::string newDisplayName)
    : levelId(std::move(levelId))
    , newDisplayName(std::move(newDisplayName))
{}

DeleteWorldRequest::DeleteWorldRequest(std::string levelId)
    : levelId(std::move(levelId))
{}

BackupWorldRequest::BackupWorldRequest(std::string levelId, std::string reason)
    : levelId(std::move(levelId))
    , reason(std::move(reason))
{}

BackupWorldResult::BackupWorldResult(std::filesystem::path zipPath, u64 sizeBytes)
    : zipPath(std::move(zipPath))
    , sizeBytes(sizeBytes)
{}

} // namespace mc::world::storage
