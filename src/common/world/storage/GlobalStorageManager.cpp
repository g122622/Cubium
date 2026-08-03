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
#include "common/core/GameDirectory.hpp"
#include "common/core/Result.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "common/world/storage/core/WorldStoragePaths.hpp"
#include "common/world/storage/list/WorldListEntry.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

GlobalStorageManager::GlobalStorageManager() = default;

GlobalStorageManager::GlobalStorageManager(WorldStoragePaths paths)
    : m_paths(std::move(paths))
    , m_worldListService(m_paths)
{}

GlobalStorageManager::GlobalStorageManager(const GameDirectory& gameDirectory)
    : GlobalStorageManager(WorldStoragePaths::fromGameDirectory(gameDirectory))
{}

Result<std::vector<WorldListEntry>> GlobalStorageManager::listWorlds()
{
    return m_worldListService.listWorlds();
}

Result<std::unique_ptr<SingleLevelStorageManager>> GlobalStorageManager::openLevel(
    const std::string& levelId, const SingleLevelStorageConfig& config)
{
    auto storage = std::make_unique<SingleLevelStorageManager>();

    // 先尝试标准路径（saves/levelId）
    std::filesystem::path worldDir = m_paths.worldDir(levelId);
    if (!std::filesystem::exists(worldDir)) {
        // 将 levelId 视为绝对路径尝试
        std::filesystem::path absolutePath(levelId);
        std::error_code ec;
        if (std::filesystem::exists(absolutePath, ec)) {
            spdlog::info("GlobalStorageManager: Using absolute path for level: {}", levelId);
            worldDir = absolutePath;
        } else {
            return Error(ErrorCode::WorldNotFound,
                fmt::format("World not found: '{}' (tried as both level ID and absolute path)", levelId));
        }
    }

    auto openResult = storage->open(worldDir, config);
    if (openResult.failed()) {
        return openResult.error();
    }
    return storage;
}

Result<void> GlobalStorageManager::deleteWorld(const std::string& levelId)
{
    return m_worldListService.deleteWorld(levelId);
}

const std::filesystem::path& GlobalStorageManager::savesDirectory() const noexcept
{
    return m_paths.savesDir();
}

} // namespace mc::world::storage
