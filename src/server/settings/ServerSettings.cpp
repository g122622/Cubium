#include "ServerSettings.hpp"

#include <spdlog/spdlog.h>
#include <fstream>

namespace mc::server {

ServerSettings::ServerSettings()
    // 网络设置
    : serverPort("serverPort", 1, 65535, 19132)
    , bindAddress("bindAddress", "0.0.0.0")
    , maxPlayers("maxPlayers", 1, 1000, 20)
    , onlineMode("onlineMode", true)
    , motd("motd", "A Minecraft Reborn Server")
    , p2pEnabled("p2pEnabled", false)

    // 世界设置
    , worldName("worldName", "world")
    , levelName("levelName", "Minecraft Reborn Server")
    , levelSeed("levelSeed", "")
    , levelType("levelType",
                {LevelType::Default, LevelType::Flat, LevelType::LargeBiomes, LevelType::Amplified, LevelType::Debug},
                LevelType::Default,
                {"default", "flat", "largeBiomes", "amplified", "debug_all_block_states"})
    , generateStructures("generateStructures", true)
    , enableCommandBlock("enableCommandBlock", false)

    // 游戏设置
    , defaultGameMode("defaultGameMode",
                      {GameModeValue::Survival, GameModeValue::Creative, GameModeValue::Adventure, GameModeValue::Spectator},
                      GameModeValue::Survival,
                      {"survival", "creative", "adventure", "spectator"})
    , difficulty("difficulty",
                 {DifficultyValue::Peaceful, DifficultyValue::Easy, DifficultyValue::Normal, DifficultyValue::Hard},
                 DifficultyValue::Normal,
                 {"peaceful", "easy", "normal", "hard"})
    , hardcore("hardcore", false)
    , pvpEnabled("pvpEnabled", true)
    , allowFlight("allowFlight", false)
    , playerIdleTimeout("playerIdleTimeout", 0, 1440, 0)
    , tickRate("tickRate", 1, 20, 20)

    // 性能设置
    , viewDistance("viewDistance", 2, 32, 10)
    , simulationDistance("simulationDistance", 2, 32, 10)
    , maxEntitiesPerChunk("maxEntitiesPerChunk", 1, 1024, 128)
    , chunkLoadRate("chunkLoadRate", 1, 100, 16)

    // 安全设置
    , whiteList("whiteList", false)
    , blackList("blackList", true)
    , maxTickTime("maxTickTime", 1000, 60000, 60000)
    , maxPacketSize("maxPacketSize", 1024, 16777216, 2097152)  // 2MB

    // 日志设置
    , logLevel("logLevel", "info")
    , logToFile("logToFile", false)
    , logFile("logFile", "server.log")
    , debugLogging("debugLogging", false)
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

std::filesystem::path ServerSettings::getDefaultPath()
{
    return SettingsBase::getSettingsPath("minecraft-server") / "server.json";
}

} // namespace mc::server
