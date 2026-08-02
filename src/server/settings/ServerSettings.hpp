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
#include "common/core/settings/SettingsBase.hpp"
#include "common/core/settings/SettingsTypes.hpp"

namespace mc::server {

/**
 * @brief 服务端设置类
 *
 * 管理服务端所有设置项，包括网络、世界、游戏和安全设置。
 * 设置自动保存到文件，支持热重载。
 *
 * 使用示例:
 * @code
 * ServerSettings settings;
 * settings.load("server_options.json");
 *
 * // 访问设置
 * u16 port = settings.serverPort.get();
 * settings.maxPlayers.set(50);
 *
 * // 设置变更回调
 * settings.maxPlayers.onChange([](int value) {
 *     spdlog::info("Max players: {}", value);
 * });
 * @endcode
 */
class ServerSettings : public SettingsBase {
public:
    ServerSettings();
    ~ServerSettings() override = default;

    // 禁止拷贝
    ServerSettings(const ServerSettings&) = delete;
    ServerSettings& operator=(const ServerSettings&) = delete;

    // 允许移动
    ServerSettings(ServerSettings&&) noexcept = default;
    ServerSettings& operator=(ServerSettings&&) noexcept = default;

    // ========================================================================
    // 网络设置
    // ========================================================================

    /// 服务器端口
    RangeOption serverPort;

    /// 绑定地址
    StringOption bindAddress;

    /// 最大玩家数
    RangeOption maxPlayers;

    /// 在线模式（验证玩家账户）
    BooleanOption onlineMode;

    /// 服务器描述（MOTD）
    StringOption motd;

    /// 是否启用 P2P 连接
    BooleanOption p2pEnabled;

    // ========================================================================
    // 世界设置
    // ========================================================================

    /// 世界名称
    StringOption worldName;

    /// 关卡名称
    StringOption levelName;

    /// 世界种子（空字符串表示随机）
    StringOption levelSeed;

    /// 世界类型
    EnumOption<u8> levelType;

    /// 是否生成结构
    BooleanOption generateStructures;

    /// 是否启用命令方块
    BooleanOption enableCommandBlock;

    // ========================================================================
    // 游戏设置
    // ========================================================================

    /// 默认游戏模式
    EnumOption<u8> defaultGameMode;

    /// 难度
    EnumOption<u8> difficulty;

    /// 硬核模式
    BooleanOption hardcore;

    /// 是否允许 PVP
    BooleanOption pvpEnabled;

    /// 是否允许飞行
    BooleanOption allowFlight;

    /// 玩家闲置超时（分钟）
    RangeOption playerIdleTimeout;

    /// 游戏刻率（TPS）
    RangeOption tickRate;

    // ========================================================================
    // 性能设置
    // ========================================================================

    /// 视距（区块）
    RangeOption viewDistance;

    /// 模拟距离（区块）
    RangeOption simulationDistance;

    /// 最大实体数每区块
    RangeOption maxEntitiesPerChunk;

    /// 区块加载速率
    RangeOption chunkLoadRate;

    // ========================================================================
    // 安全设置
    // ========================================================================

    /// 是否启用白名单
    BooleanOption whiteList;

    /// 是否启用黑名单
    BooleanOption blackList;

    /// 刻超时时间（毫秒）
    RangeOption maxTickTime;

    /// 最大数据包大小（字节）
    RangeOption maxPacketSize;

    // ========================================================================
    // 日志设置
    // ========================================================================

    /// 日志级别
    StringOption logLevel;

    /// 是否记录到文件
    BooleanOption logToFile;

    /// 日志文件名
    StringOption logFile;

    /// 是否记录调试信息
    BooleanOption debugLogging;

    // ========================================================================
    // 便捷方法
    // ========================================================================

    /**
     * @brief 解析种子字符串为 u64
     *
     * 空字符串返回 0（随机种子）。
     * 纯数字字符串直接转换为 u64。
     * 非数字字符串使用哈希值作为种子。
     */
    [[nodiscard]] u64 parseSeed() const;

    /**
     * @brief 生成默认服务端配置文件
     *
     * 将所有选项重置为默认值并保存到指定路径。
     * 当配置文件不存在时由 loadOrGenerate() 调用。
     *
     * @param path 配置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> generateDefaultConfig(const std::filesystem::path& path) override;

    // ========================================================================
    // 加载/保存
    // ========================================================================

    /**
     * @brief 加载服务端设置
     * @param path 设置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadSettings(const std::filesystem::path& path);

    /**
     * @brief 保存服务端设置
     * @param path 设置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> saveSettings(const std::filesystem::path& path);
};

// ============================================================================
// 枚举值常量
// ============================================================================

/// 世界类型值
namespace LevelType {
constexpr u8 Default = 0;
constexpr u8 Flat = 1;
constexpr u8 LargeBiomes = 2;
constexpr u8 Amplified = 3;
constexpr u8 Debug = 4; ///< 调试世界类型
} // namespace LevelType

/// 游戏模式值（与 Types.hpp 中的 GameMode 枚举对应）
namespace GameModeValue {
constexpr u8 Survival = 0;
constexpr u8 Creative = 1;
constexpr u8 Adventure = 2;
constexpr u8 Spectator = 3;
} // namespace GameModeValue

/// 难度值（与 Types.hpp 中的 Difficulty 枚举对应）
namespace DifficultyValue {
constexpr u8 Peaceful = 0;
constexpr u8 Easy = 1;
constexpr u8 Normal = 2;
constexpr u8 Hard = 3;
} // namespace DifficultyValue

} // namespace mc::server
