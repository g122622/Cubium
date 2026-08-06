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

#include "GameDirectory.hpp"
#include "common/core/Result.hpp"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // 禁用 getenv 不安全警告
#endif

namespace fs = std::filesystem;

namespace mc {

namespace {

/**
 * @brief 展开用户主目录路径
 *
 * 将路径开头的 ~ 展开为用户主目录。
 * - Unix: $HOME
 * - Windows: %USERPROFILE%
 *
 * @param path 可能包含 ~ 的路径
 * @return 展开后的路径
 */
fs::path expandHomePath(const fs::path& path)
{
    std::string pathStr = path.string();
    if (pathStr.empty() || pathStr[0] != '~') {
        return path;
    }

    std::string homeDir;

#ifdef _WIN32
    // Windows: 使用 USERPROFILE 环境变量
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile != nullptr) {
        homeDir = userProfile;
    } else {
        // 回退：组合 HOMEDRIVE 和 HOMEPATH
        const char* homeDrive = std::getenv("HOMEDRIVE");
        const char* homePath = std::getenv("HOMEPATH");
        if (homeDrive != nullptr && homePath != nullptr) {
            homeDir = std::string(homeDrive) + homePath;
        }
    }
#else
    // Unix: 使用 HOME 环境变量
    const char* home = std::getenv("HOME");
    if (home != nullptr) {
        homeDir = home;
    }
#endif

    if (homeDir.empty()) {
        spdlog::warn("Could not determine home directory, using current path for ~ expansion");
        return path;
    }

    // 替换 ~ 为 homeDir
    return fs::path(homeDir) / pathStr.substr(1);
}

} // namespace

// ============================================================================
// 工厂方法
// ============================================================================

GameDirectory GameDirectory::fromConfigPath(const std::filesystem::path& configPath)
{
    if (configPath.empty()) {
        return defaultDirectory();
    }

    // 展开可能的 ~ 路径
    fs::path expandedPath = expandHomePath(configPath);

    // 配置文件的父目录即为游戏目录
    fs::path root = expandedPath.parent_path();

    if (root.empty()) {
        root = fs::current_path();
    }

    return GameDirectory(fs::weakly_canonical(root));
}

GameDirectory GameDirectory::defaultDirectory()
{
    // 默认路径：~/minecraft_reborn/
    fs::path homePath = expandHomePath("~");
    fs::path root = homePath / "minecraft_reborn";

    return GameDirectory(fs::weakly_canonical(root));
}

GameDirectory GameDirectory::fromRoot(std::filesystem::path root)
{
    if (root.empty()) {
        return defaultDirectory();
    }

    fs::path expandedRoot = expandHomePath(root);

    return GameDirectory(fs::weakly_canonical(expandedRoot));
}

GameDirectory::GameDirectory(std::filesystem::path root)
    : m_root(std::move(root))
{}

// ============================================================================
// 路径访问器
// ============================================================================

std::filesystem::path GameDirectory::clientOptionsPath() const
{
    return m_root / "client_options.json";
}

std::filesystem::path GameDirectory::serverOptionsPath() const
{
    return m_root / "server_options.json";
}

std::filesystem::path GameDirectory::resourcePacksDir() const
{
    return m_root / "resourcepacks";
}

std::filesystem::path GameDirectory::dataPacksDir() const
{
    return m_root / "datapacks";
}

std::filesystem::path GameDirectory::savesDir() const
{
    return m_root / "saves";
}

std::filesystem::path GameDirectory::backupsDir() const
{
    return m_root / "backups";
}

std::filesystem::path GameDirectory::logsDir() const
{
    return m_root / "logs";
}

std::filesystem::path GameDirectory::cacheDir() const
{
    return m_root / "cache";
}

std::filesystem::path GameDirectory::builtinPackDir() const
{
    // 内置包位于可执行文件旁的 resources/builtin/
    // 这不是游戏根目录下的子目录，而是随可执行文件分发的
    fs::path exeDir = fs::current_path();
    return exeDir / "resources" / "builtin";
}

// ============================================================================
// 目录操作
// ============================================================================

Result<void> GameDirectory::ensureDirectoriesExist() const
{
    if (m_root.empty()) {
        return Error(ErrorCode::InvalidArgument, "Game directory root is empty");
    }

    // 需要创建的目录列表
    std::vector<fs::path> dirs = {
        m_root,
        resourcePacksDir(),
        dataPacksDir(),
        savesDir(),
        backupsDir(),
        logsDir(),
        cacheDir(),
    };

    for (const auto& dir : dirs) {
        if (!fs::exists(dir)) {
            std::error_code ec;
            if (!fs::create_directories(dir, ec)) {
                return Error(
                    ErrorCode::OperationFailed, "Failed to create directory: " + dir.string() + " - " + ec.message());
            }
            spdlog::info("Created game directory: {}", dir.string());
        }
    }

    return Result<void>::ok();
}

} // namespace mc

#ifdef _MSC_VER
#pragma warning(pop)
#endif
