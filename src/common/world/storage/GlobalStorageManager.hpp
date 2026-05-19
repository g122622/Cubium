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

#pragma once

#include "common/core/Result.hpp"
#include "world/storage/SingleLevelStorageManager.hpp"
#include "world/storage/core/WorldStoragePaths.hpp"
#include "world/storage/list/WorldListEntry.hpp"
#include "world/storage/list/WorldListService.hpp"
#include "world/storage/request/WorldRequests.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace mc::world::storage {

class GlobalStorageManager {
public:
    GlobalStorageManager();
    explicit GlobalStorageManager(WorldStoragePaths paths);

    [[nodiscard]] Result<std::vector<WorldListEntry>> listWorlds();
    [[nodiscard]] Result<WorldListEntry> getWorldSummary(const std::string& levelId);
    [[nodiscard]] bool worldExists(const std::string& levelId);
    [[nodiscard]] Result<std::string> createWorld(const CreateWorldRequest& request);
    Result<void> deleteWorld(const std::string& levelId);
    Result<void> renameWorld(const std::string& levelId, const std::string& newDisplayName);
    Result<void> updateLastPlayed(const std::string& levelId, i64 lastPlayedMs);
    [[nodiscard]] Result<BackupWorldResult> backupWorld(const BackupWorldRequest& request);
    [[nodiscard]] Result<std::unique_ptr<SingleLevelStorageManager>> openLevel(
        const std::string& levelId, const SingleLevelStorageConfig& config);

    [[nodiscard]] std::filesystem::path resolveWorldPath(const std::string& levelId) const;
    [[nodiscard]] const std::filesystem::path& savesDirectory() const noexcept;
    [[nodiscard]] const std::filesystem::path& backupsDirectory() const noexcept;
    [[nodiscard]] const WorldStoragePaths& paths() const noexcept;

private:
    WorldStoragePaths m_paths;
    WorldListService m_worldListService;
};

} // namespace mc::world::storage
 