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

#include "common/core/Types.hpp"
#include "common/world/WorldConfig.hpp"
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

namespace mc::world::storage {

/**
 * @brief 世界兼容性状态
 */
enum class WorldCompatibility : u8 {
    Current,            ///< 当前版本，可直接加载
    Older,              ///< 旧版本，建议备份后升级
    Newer,              ///< 未来版本，通常不可加载
    Unknown,            ///< 版本信息缺失或无法识别
    UnsupportedStorage, ///< 不支持的存储格式（如 McRegion）
    Corrupted           ///< level.dat 损坏
};

/**
 * @brief 世界列表条目
 *
 * 用于存档列表 UI 显示和世界选择。包含从 level.dat 摘要读取的所有信息。
 *
 * 使用 Builder 模式创建：
 * @code
 * auto entry = WorldListEntry::builder()
 *     .levelId("MyWorld")
 *     .worldDir("/saves/MyWorld")
 *     .displayName("My World")
 *     .build();
 * @endcode
 */
struct WorldListEntry {
    /// 世界目录名（稳定 ID）
    std::string levelId;

    /// 世界目录绝对路径
    std::filesystem::path worldDir;

    /// 显示名称，优先 level.dat 中的 LevelName，否则使用 levelId
    std::string displayName;

    /// 最后游玩时间（毫秒时间戳），用于排序
    i64 lastPlayedMs = 0;

    /// 世界种子
    u64 seed = 0;

    /// 世界类型
    WorldType worldType = WorldType::Default;

    /// 世界预设资源位置（数据驱动装配查 WorldPresetRegistry，如 "minecraft:normal"）
    resource::ResourceLocation worldPresetId{"minecraft", "normal"};

    /// 游戏模式
    GameMode gameMode = GameMode::Survival;

    /// 难度
    Difficulty difficulty = Difficulty::Normal;

    /// 是否为极限模式
    bool hardcore = false;

    /// 是否允许作弊
    bool allowCommands = false;

    /// 是否被其他进程锁定
    bool locked = false;

    /// 是否需要存储格式转换（如 McRegion -> Anvil）
    bool requiresConversion = false;

    /// 兼容性状态
    WorldCompatibility compatibility = WorldCompatibility::Current;

    /// 版本名称（如 "1.16.5"）
    std::string versionName;

    /// 数据版本号
    i32 dataVersion = 0;

    /// 图标文件路径（可能不存在）
    std::filesystem::path iconPath;

    /// 错误信息（损坏时）
    std::string errorMessage;

    WorldListEntry() = default;

    /**
     * @brief 构造世界列表条目
     */
    WorldListEntry(std::string levelId,
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
        std::string errorMessage);

    /**
     * @brief Builder 模式
     */
    class Builder {
    public:
        Builder& levelId(std::string value)
        {
            m_levelId = std::move(value);
            return *this;
        }
        Builder& worldDir(std::filesystem::path value)
        {
            m_worldDir = std::move(value);
            return *this;
        }
        Builder& displayName(std::string value)
        {
            m_displayName = std::move(value);
            return *this;
        }
        Builder& lastPlayedMs(i64 value)
        {
            m_lastPlayedMs = value;
            return *this;
        }
        Builder& seed(u64 value)
        {
            m_seed = value;
            return *this;
        }
        Builder& worldType(WorldType value)
        {
            m_worldType = value;
            return *this;
        }
        Builder& worldPresetId(resource::ResourceLocation value)
        {
            m_worldPresetId = std::move(value);
            return *this;
        }
        Builder& gameMode(GameMode value)
        {
            m_gameMode = value;
            return *this;
        }
        Builder& difficulty(Difficulty value)
        {
            m_difficulty = value;
            return *this;
        }
        Builder& hardcore(bool value)
        {
            m_hardcore = value;
            return *this;
        }
        Builder& allowCommands(bool value)
        {
            m_allowCommands = value;
            return *this;
        }
        Builder& locked(bool value)
        {
            m_locked = value;
            return *this;
        }
        Builder& requiresConversion(bool value)
        {
            m_requiresConversion = value;
            return *this;
        }
        Builder& compatibility(WorldCompatibility value)
        {
            m_compatibility = value;
            return *this;
        }
        Builder& versionName(std::string value)
        {
            m_versionName = std::move(value);
            return *this;
        }
        Builder& dataVersion(i32 value)
        {
            m_dataVersion = value;
            return *this;
        }
        Builder& iconPath(std::filesystem::path value)
        {
            m_iconPath = std::move(value);
            return *this;
        }
        Builder& errorMessage(std::string value)
        {
            m_errorMessage = std::move(value);
            return *this;
        }

        [[nodiscard]] WorldListEntry build() const noexcept
        {
            return WorldListEntry(m_levelId,
                m_worldDir,
                m_displayName,
                m_lastPlayedMs,
                m_seed,
                m_worldType,
                m_worldPresetId,
                m_gameMode,
                m_difficulty,
                m_hardcore,
                m_allowCommands,
                m_locked,
                m_requiresConversion,
                m_compatibility,
                m_versionName,
                m_dataVersion,
                m_iconPath,
                m_errorMessage);
        }

    private:
        std::string m_levelId;
        std::filesystem::path m_worldDir;
        std::string m_displayName;
        i64 m_lastPlayedMs = 0;
        u64 m_seed = 0;
        WorldType m_worldType = WorldType::Default;
        resource::ResourceLocation m_worldPresetId{"minecraft", "normal"};
        GameMode m_gameMode = GameMode::Survival;
        Difficulty m_difficulty = Difficulty::Normal;
        bool m_hardcore = false;
        bool m_allowCommands = false;
        bool m_locked = false;
        bool m_requiresConversion = false;
        WorldCompatibility m_compatibility = WorldCompatibility::Current;
        std::string m_versionName;
        i32 m_dataVersion = 0;
        std::filesystem::path m_iconPath;
        std::string m_errorMessage;
    };

    /**
     * @brief 创建 Builder
     */
    static Builder builder() { return Builder(); }

    /**
     * @brief 获取排序键
     *
     * 按 lastPlayedMs 降序，levelId 升序。
     */
    [[nodiscard]] bool operator<(const WorldListEntry& other) const noexcept;
};

/**
 * @brief 世界列表排序函数
 *
 * 按 lastPlayedMs 降序、levelId 升序排列。
 */
void sortWorldEntries(std::vector<WorldListEntry>& entries);

/**
 * @brief 过滤世界列表
 *
 * 检查 displayName 或 levelId 是否包含搜索字符串（不区分大小写）。
 */
std::vector<WorldListEntry> filterWorldEntries(
    const std::vector<WorldListEntry>& entries, const std::string& searchQuery);

} // namespace mc::world::storage
