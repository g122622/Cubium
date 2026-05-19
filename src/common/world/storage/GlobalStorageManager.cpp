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

#include "GlobalStorageManager.hpp"

namespace mc::world::storage {

GlobalStorageManager::GlobalStorageManager()
    : GlobalStorageManager(WorldStoragePaths::defaultPaths())
{}

GlobalStorageManager::GlobalStorageManager(WorldStoragePaths paths)
    : m_paths(std::move(paths))
    , m_worldListService(m_paths)
{}

Result<std::vector<WorldListEntry>> GlobalStorageManager::listWorlds()
{
    return m_worldListService.listWorlds();
}

Result<WorldListEntry> GlobalStorageManager::getWorldSummary(const std::string& levelId)
{
    return m_worldListService.getWorldSummary(levelId);
}

bool GlobalStorageManager::worldExists(const std::string& levelId)
{
    return m_worldListService.worldExists(levelId);
}

Result<std::string> GlobalStorageManager::createWorld(const CreateWorldRequest& request)
{
    return m_worldListService.createWorld(request);
}

Result<void> GlobalStorageManager::deleteWorld(const std::string& levelId)
{
    return m_worldListService.deleteWorld(levelId);
}

Result<void> GlobalStorageManager::renameWorld(const std::string& levelId, const std::string& newDisplayName)
{
    return m_worldListService.renameWorld(levelId, newDisplayName);
}

Result<void> GlobalStorageManager::updateLastPlayed(const std::string& levelId, i64 lastPlayedMs)
{
    return m_worldListService.updateLastPlayed(levelId, lastPlayedMs);
}

Result<BackupWorldResult> GlobalStorageManager::backupWorld(const BackupWorldRequest& request)
{
    return m_worldListService.backupWorld(request);
}

Result<std::unique_ptr<SingleLevelStorageManager>> GlobalStorageManager::openLevel(
    const std::string& levelId, const SingleLevelStorageConfig& config)
{
    auto storage = std::make_unique<SingleLevelStorageManager>();
    auto openResult = storage->open(resolveWorldPath(levelId), config);
    if (openResult.failed()) {
        return openResult.error();
    }
    return storage;
}

std::filesystem::path GlobalStorageManager::resolveWorldPath(const std::string& levelId) const
{
    return m_paths.worldDir(levelId);
}

const std::filesystem::path& GlobalStorageManager::savesDirectory() const noexcept
{
    return m_paths.savesDir();
}

const std::filesystem::path& GlobalStorageManager::backupsDirectory() const noexcept
{
    return m_paths.backupsDir();
}

const WorldStoragePaths& GlobalStorageManager::paths() const noexcept
{
    return m_paths;
}

} // namespace mc::world::storage
