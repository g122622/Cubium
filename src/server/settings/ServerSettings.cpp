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

#include "ServerSettings.hpp"

#include "common/core/DefaultValues.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <spdlog/spdlog.h>

namespace mc::server {

ServerSettings::ServerSettings()
    // 网络设置
    : serverPort("serverPort", 1, 65535, defaults::server::serverPort)
    , bindAddress("bindAddress", defaults::server::bindAddress)
    , maxPlayers("maxPlayers", 1, 1000, defaults::server::maxPlayers)
    , onlineMode("onlineMode", defaults::server::onlineMode)
    , motd("motd", defaults::server::motd)
    , p2pEnabled("p2pEnabled", defaults::server::p2pEnabled)

    // 世界设置
    , worldName("worldName", defaults::server::worldName)
    , levelName("levelName", defaults::server::levelName)
    , levelSeed("levelSeed", defaults::server::levelSeed)
    , levelType("levelType",
          {LevelType::Default, LevelType::Flat, LevelType::LargeBiomes, LevelType::Amplified, LevelType::Debug},
          LevelType::Default,
          {"default", "flat", "largeBiomes", "amplified", "debug_all_block_states"})
    , generateStructures("generateStructures", defaults::server::generateStructures)
    , enableCommandBlock("enableCommandBlock", defaults::server::enableCommandBlock)

    // 游戏设置
    , defaultGameMode("defaultGameMode",
          {GameModeValue::Survival, GameModeValue::Creative, GameModeValue::Adventure, GameModeValue::Spectator},
          GameModeValue::Survival,
          {"survival", "creative", "adventure", "spectator"})
    , difficulty("difficulty",
          {DifficultyValue::Peaceful, DifficultyValue::Easy, DifficultyValue::Normal, DifficultyValue::Hard},
          DifficultyValue::Normal,
          {"peaceful", "easy", "normal", "hard"})
    , hardcore("hardcore", defaults::server::hardcore)
    , pvpEnabled("pvpEnabled", defaults::server::pvpEnabled)
    , allowFlight("allowFlight", defaults::server::allowFlight)
    , playerIdleTimeout("playerIdleTimeout", 0, 1440, defaults::server::playerIdleTimeout)
    , tickRate("tickRate", 1, 20, defaults::server::tickRate)

    // 性能设置
    , viewDistance("viewDistance", 2, 32, defaults::server::viewDistance)
    , simulationDistance("simulationDistance", 2, 32, defaults::server::simulationDistance)
    , maxEntitiesPerChunk("maxEntitiesPerChunk", 1, 1024, defaults::server::maxEntitiesPerChunk)
    , chunkLoadRate("chunkLoadRate", 1, 100, defaults::server::chunkLoadRate)

    // 安全设置
    , whiteList("whiteList", defaults::server::whiteList)
    , blackList("blackList", defaults::server::blackList)
    , maxTickTime("maxTickTime", 1000, 60000, defaults::server::maxTickTime)
    , maxPacketSize("maxPacketSize", 1024, 16777216, defaults::server::maxPacketSize)

    // 日志设置
    , logLevel("logLevel", defaults::server::serverLogLevel)
    , logToFile("logToFile", defaults::server::logToFile)
    , logFile("logFile", defaults::server::logFile)
    , debugLogging("debugLogging", defaults::server::debugLogging)
{
    // 注册网络设置
    registerOption("network", &serverPort);
    registerOption("network", &bindAddress);
    registerOption("network", &maxPlayers);
    registerOption("network", &onlineMode);
    registerOption("network", &motd);
    registerOption("network", &p2pEnabled);

    // 注册世界设置
    registerOption("world", &worldName);
    registerOption("world", &levelName);
    registerOption("world", &levelSeed);
    registerOption("world", &levelType);
    registerOption("world", &generateStructures);
    registerOption("world", &enableCommandBlock);

    // 注册游戏设置
    registerOption("game", &defaultGameMode);
    registerOption("game", &difficulty);
    registerOption("game", &hardcore);
    registerOption("game", &pvpEnabled);
    registerOption("game", &allowFlight);
    registerOption("game", &playerIdleTimeout);
    registerOption("game", &tickRate);

    // 注册性能设置
    registerOption("performance", &viewDistance);
    registerOption("performance", &simulationDistance);
    registerOption("performance", &maxEntitiesPerChunk);
    registerOption("performance", &chunkLoadRate);

    // 注册安全设置
    registerOption("security", &whiteList);
    registerOption("security", &blackList);
    registerOption("security", &maxTickTime);
    registerOption("security", &maxPacketSize);

    // 注册日志设置
    registerOption("log", &logLevel);
    registerOption("log", &logToFile);
    registerOption("log", &logFile);
    registerOption("log", &debugLogging);
}

u64 ServerSettings::parseSeed() const
{
    const auto& seedStr = levelSeed.get();
    if (seedStr.empty()) {
        return 0;
    }

    try {
        return static_cast<u64>(std::stoll(seedStr));
    }
    catch (...) {
        return std::hash<std::string>{}(seedStr);
    }
}

Result<void> ServerSettings::generateDefaultConfig(const std::filesystem::path& path)
{
    resetToDefaults();
    return save(path);
}

Result<void> ServerSettings::loadSettings(const std::filesystem::path& path)
{
    auto result = load(path);
    if (result.failed()) {
        return result;
    }

    spdlog::info("Server settings loaded from: {}", path.string());
    return Result<void>::ok();
}

Result<void> ServerSettings::saveSettings(const std::filesystem::path& path)
{
    auto result = save(path);
    if (result.failed()) {
        return result;
    }

    spdlog::info("Server settings saved to: {}", path.string());
    return Result<void>::ok();
}

} // namespace mc::server
