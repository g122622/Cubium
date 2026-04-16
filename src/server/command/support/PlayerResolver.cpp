#include "PlayerResolver.hpp"

#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

#include <algorithm>

namespace mc::command::support {

namespace {

/**
 * @brief 获取按 ID 排序的在线玩家列表。
 *
 * @param source 命令源。
 * @return 排序后的玩家 ID 列表。
 */
[[nodiscard]] std::vector<PlayerId> getSortedPlayerIds(const ServerCommandSource& source)
{
    if (source.server() == nullptr) {
        return {};
    }

    auto playerIds = source.server()->playerManager().getPlayerIds();
    std::sort(playerIds.begin(), playerIds.end());
    return playerIds;
}

/**
 * @brief 按用户名查找玩家 ID。
 *
 * @param source 命令源。
 * @param username 目标玩家名。
 * @return 找到的玩家 ID；找不到时返回 0。
 */
[[nodiscard]] PlayerId findPlayerIdByUsername(const ServerCommandSource& source, StringView username)
{
    if (source.server() == nullptr) {
        return 0;
    }

    PlayerId resolvedPlayerId = 0;
    source.server()->playerManager().forEachPlayer([&](const server::ServerPlayerData& playerData) {
        if (resolvedPlayerId == 0 && playerData.username == username) {
            resolvedPlayerId = playerData.playerId;
        }
    });
    return resolvedPlayerId;
}

} // namespace

/**
 * @brief 解析单个玩家选择器。
 *
 * @param source 命令源。
 * @param selector 玩家选择器。
 * @return 匹配到的玩家 ID，失败时返回 0。
 */
PlayerId resolveSinglePlayerId(const ServerCommandSource& source, const EntitySelector& selector)
{
    auto playerIds = resolvePlayerIds(source, selector);
    return playerIds.empty() ? 0 : playerIds.front();
}

/**
 * @brief 解析多个玩家选择器。
 *
 * @param source 命令源。
 * @param selector 玩家选择器。
 * @return 匹配到的玩家 ID 列表。
 */
std::vector<PlayerId> resolvePlayerIds(const ServerCommandSource& source, const EntitySelector& selector)
{
    if (source.server() == nullptr) {
        return {};
    }

    if (selector.hasUsername()) {
        const PlayerId playerId = findPlayerIdByUsername(source, selector.username());
        return playerId == 0 ? std::vector<PlayerId>{} : std::vector<PlayerId>{playerId};
    }

    auto playerIds = getSortedPlayerIds(source);
    std::vector<PlayerId> resolved;

    switch (selector.type()) {
        case EntitySelectorType::Self:
            if (source.playerId() != 0) {
                resolved.push_back(source.playerId());
            }
            break;
        case EntitySelectorType::SinglePlayer:
        case EntitySelectorType::RandomPlayer:
            if (!playerIds.empty()) {
                resolved.push_back(playerIds.front());
            }
            break;
        case EntitySelectorType::AllPlayers:
        case EntitySelectorType::AllEntities:
            resolved = std::move(playerIds);
            break;
    }

    if (resolved.empty() && source.playerId() != 0) {
        resolved.push_back(source.playerId());
    }

    if (selector.limit() > 0 && resolved.size() > static_cast<size_t>(selector.limit())) {
        resolved.resize(static_cast<size_t>(selector.limit()));
    }

    return resolved;
}

/**
 * @brief 将游戏模式转换为命令输出名称。
 *
 * @param mode 游戏模式。
 * @return 命令名称。
 */
const char* getGameModeCommandName(GameMode mode) noexcept
{
    switch (mode) {
        case GameMode::Survival:
            return "survival";
        case GameMode::Creative:
            return "creative";
        case GameMode::Adventure:
            return "adventure";
        case GameMode::Spectator:
            return "spectator";
        case GameMode::NotSet:
            return "not_set";
    }

    return "unknown";
}

/**
 * @brief 将难度转换为命令输出名称。
 *
 * @param difficulty 难度。
 * @return 命令名称。
 */
const char* getDifficultyCommandName(Difficulty difficulty) noexcept
{
    switch (difficulty) {
        case Difficulty::Peaceful:
            return "peaceful";
        case Difficulty::Easy:
            return "easy";
        case Difficulty::Normal:
            return "normal";
        case Difficulty::Hard:
            return "hard";
    }

    return "unknown";
}

} // namespace mc::command::support
