#include "PlayerResolver.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

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
[[nodiscard]] PlayerId findPlayerIdByUsername(const ServerCommandSource& source, std::string_view username)
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

/**
 * @brief 检查玩家是否符合选择器的过滤条件。
 *
 * @param playerData 玩家数据。
 * @param selector 选择器。
 * @param server 服务器实例（用于获取玩家实体）。
 * @param world 世界实例（用于获取玩家实体）。
 * @return 是否符合条件。
 */
[[nodiscard]] bool matchesFilter(const server::ServerPlayerData& playerData,
    const EntitySelector& selector,
    server::IServer* server,
    server::ServerWorld* world)
{
    // 检查等级范围
    if (!selector.level().isUnbounded()) {
        // 通过 ServerPlayerEntityManager 获取玩家实体来访问经验等级
        if (server != nullptr && world != nullptr) {
            Player* player = server->playerEntityManager().getPlayerEntity(playerData.playerId, *world);
            if (player != nullptr) {
                i32 level = player->experienceLevel();
                if (!selector.level().test(level)) {
                    return false;
                }
            }
        }
    }

    // 检查俯仰角范围（x_rotation，-90 到 90 度）
    // 参考 MC 1.16.5 EntitySelector 过滤逻辑
    if (!selector.xRotation().isUnbounded()) {
        // 优先使用实体实时角度，如果没有实体则使用存储的角度
        f32 pitch = playerData.pitch;
        if (server != nullptr && world != nullptr) {
            Player* player = server->playerEntityManager().getPlayerEntity(playerData.playerId, *world);
            if (player != nullptr) {
                pitch = player->pitch();
            }
        }
        if (!selector.xRotation().testAngle(pitch)) {
            return false;
        }
    }

    // 检查偏航角范围（y_rotation，-180 到 180 度）
    // 参考 MC 1.16.5 EntitySelector 过滤逻辑
    if (!selector.yRotation().isUnbounded()) {
        // 优先使用实体实时角度，如果没有实体则使用存储的角度
        f32 yaw = playerData.yaw;
        if (server != nullptr && world != nullptr) {
            Player* player = server->playerEntityManager().getPlayerEntity(playerData.playerId, *world);
            if (player != nullptr) {
                yaw = player->yaw();
            }
        }
        if (!selector.yRotation().testAngle(yaw)) {
            return false;
        }
    }

    // 检查游戏模式
    if (selector.hasGameMode()) {
        const std::string& mode = selector.gameMode();
        bool matches = false;
        switch (playerData.gameMode) {
            case GameMode::Survival:
                matches = (mode == "survival" || mode == "0");
                break;
            case GameMode::Creative:
                matches = (mode == "creative" || mode == "1");
                break;
            case GameMode::Adventure:
                matches = (mode == "adventure" || mode == "2");
                break;
            case GameMode::Spectator:
                matches = (mode == "spectator" || mode == "3");
                break;
            default:
                break;
        }
        if (selector.gameModeNegated()) {
            matches = !matches;
        }
        if (!matches) {
            return false;
        }
    }

    return true;
}

/**
 * @brief 计算玩家到参考点的距离平方。
 *
 * @param playerData 玩家数据。
 * @param refX 参考点 X。
 * @param refY 参考点 Y。
 * @param refZ 参考点 Z。
 * @return 距离平方。
 */
[[nodiscard]] f32 distanceSquared(const server::ServerPlayerData& playerData, f32 refX, f32 refY, f32 refZ)
{
    const f32 dx = playerData.x - refX;
    const f32 dy = playerData.y - refY;
    const f32 dz = playerData.z - refZ;
    return dx * dx + dy * dy + dz * dz;
}

/**
 * @brief 对玩家列表进行排序。
 *
 * @param playerIds 玩家 ID 列表。
 * @param source 命令源。
 * @param selector 选择器。
 * @param refX 参考点 X。
 * @param refY 参考点 Y。
 * @param refZ 参考点 Z。
 */
void sortPlayerIds(std::vector<PlayerId>& playerIds,
    const ServerCommandSource& source,
    const EntitySelector& selector,
    f32 refX,
    f32 refY,
    f32 refZ)
{
    auto* server = source.server();
    if (server == nullptr) {
        return;
    }

    switch (selector.sort()) {
        case EntitySelectorSort::Nearest: {
            // 按距离近到远排序
            std::sort(playerIds.begin(), playerIds.end(), [&](PlayerId a, PlayerId b) {
                auto* dataA = server->playerManager().getPlayer(a);
                auto* dataB = server->playerManager().getPlayer(b);
                if (dataA == nullptr) return false;
                if (dataB == nullptr) return true;
                return distanceSquared(*dataA, refX, refY, refZ) < distanceSquared(*dataB, refX, refY, refZ);
            });
            break;
        }
        case EntitySelectorSort::Furthest: {
            // 按距离远到近排序
            std::sort(playerIds.begin(), playerIds.end(), [&](PlayerId a, PlayerId b) {
                auto* dataA = server->playerManager().getPlayer(a);
                auto* dataB = server->playerManager().getPlayer(b);
                if (dataA == nullptr) return false;
                if (dataB == nullptr) return true;
                return distanceSquared(*dataA, refX, refY, refZ) > distanceSquared(*dataB, refX, refY, refZ);
            });
            break;
        }
        case EntitySelectorSort::Random: {
            // 随机排序
            math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
            // Fisher-Yates shuffle
            for (size_t i = playerIds.size(); i > 1; --i) {
                size_t j = static_cast<size_t>(rng.nextInt(static_cast<i32>(i)));
                std::swap(playerIds[i - 1], playerIds[j]);
            }
            break;
        }
        case EntitySelectorSort::Arbitrary:
        default:
            // 保持原始顺序
            break;
    }
}

/**
 * @brief 应用选择器过滤条件。
 *
 * @param playerIds 玩家 ID 列表。
 * @param source 命令源。
 * @param selector 选择器。
 * @param refX 参考点 X。
 * @param refY 参考点 Y。
 * @param refZ 参考点 Z。
 */
void applyFilters(std::vector<PlayerId>& playerIds,
    const ServerCommandSource& source,
    const EntitySelector& selector,
    f32 refX,
    f32 refY,
    f32 refZ)
{
    auto* server = source.server();
    auto* world = source.world();
    if (server == nullptr) {
        return;
    }

    // 距离过滤
    if (!selector.distance().isUnbounded()) {
        playerIds.erase(std::remove_if(playerIds.begin(),
                            playerIds.end(),
                            [&](PlayerId id) {
                                auto* data = server->playerManager().getPlayer(id);
                                if (data == nullptr) return true;
                                const f32 distSq = distanceSquared(*data, refX, refY, refZ);
                                return !selector.distance().testSquared(distSq);
                            }),
            playerIds.end());
    }

    // 名称过滤
    if (selector.hasUsername()) {
        playerIds.erase(std::remove_if(playerIds.begin(),
                            playerIds.end(),
                            [&](PlayerId id) {
                                auto* data = server->playerManager().getPlayer(id);
                                if (data == nullptr) return true;
                                return data->username != selector.username();
                            }),
            playerIds.end());
    }
    if (selector.hasUsernameNegated()) {
        playerIds.erase(std::remove_if(playerIds.begin(),
                            playerIds.end(),
                            [&](PlayerId id) {
                                auto* data = server->playerManager().getPlayer(id);
                                if (data == nullptr) return true;
                                return data->username == selector.usernameNegated();
                            }),
            playerIds.end());
    }

    // 通用过滤（包含等级检查和游戏模式检查）
    playerIds.erase(std::remove_if(playerIds.begin(),
                        playerIds.end(),
                        [&](PlayerId id) {
                            auto* data = server->playerManager().getPlayer(id);
                            if (data == nullptr) return true;
                            return !matchesFilter(*data, selector, server, world);
                        }),
        playerIds.end());
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

    // 按用户名精确匹配
    if (selector.hasUsername()) {
        const PlayerId playerId = findPlayerIdByUsername(source, selector.username());
        return playerId == 0 ? std::vector<PlayerId>{} : std::vector<PlayerId>{playerId};
    }

    // 计算参考点坐标
    f32 refX = static_cast<f32>(source.position().x);
    f32 refY = static_cast<f32>(source.position().y);
    f32 refZ = static_cast<f32>(source.position().z);

    if (selector.hasX()) refX = selector.getX();
    if (selector.hasY()) refY = selector.getY();
    if (selector.hasZ()) refZ = selector.getZ();

    // 获取基础玩家列表
    auto playerIds = getSortedPlayerIds(source);
    std::vector<PlayerId> resolved;

    switch (selector.type()) {
        case EntitySelectorType::Self:
            if (source.playerId() != 0) {
                resolved.push_back(source.playerId());
            }
            break;
        case EntitySelectorType::SinglePlayer:
            // @p: 需要按距离排序后取最近的
            if (!playerIds.empty()) {
                resolved = playerIds;
                sortPlayerIds(resolved, source, selector, refX, refY, refZ);
                applyFilters(resolved, source, selector, refX, refY, refZ);
                if (!resolved.empty()) {
                    resolved.resize(1);
                }
            }
            break;
        case EntitySelectorType::RandomPlayer:
            // @r: 随机选择
            if (!playerIds.empty()) {
                resolved = playerIds;
                applyFilters(resolved, source, selector, refX, refY, refZ);
                if (!resolved.empty()) {
                    math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
                    size_t idx = static_cast<size_t>(rng.nextInt(static_cast<i32>(resolved.size() - 1)));
                    PlayerId randomId = resolved[idx];
                    resolved.clear();
                    resolved.push_back(randomId);
                }
            }
            break;
        case EntitySelectorType::AllPlayers:
        case EntitySelectorType::AllEntities:
            resolved = playerIds;
            applyFilters(resolved, source, selector, refX, refY, refZ);
            sortPlayerIds(resolved, source, selector, refX, refY, refZ);
            break;
    }

    // 如果没有结果但选择器允许回退到执行者
    if (resolved.empty() && source.playerId() != 0) {
        // 检查是否应该自动回退（仅对某些选择器类型）
        // MC 行为：@p 如果没有其他玩家会返回自己
        if (selector.type() == EntitySelectorType::SinglePlayer) {
            resolved.push_back(source.playerId());
        }
    }

    // 应用数量限制
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
