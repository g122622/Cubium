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
 * The above copyright notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "EntityResolver.hpp"

#include "common/advancement/AdvancementManager.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace mc {
namespace command::support {

namespace {

/**
 * @brief 计算实体到参考点的距离平方。
 */
[[nodiscard]] f32 entityDistanceSq(const Entity& entity, f32 refX, f32 refY, f32 refZ)
{
    const auto& pos = entity.position();
    const f32 dx = pos.x - refX;
    const f32 dy = pos.y - refY;
    const f32 dz = pos.z - refZ;
    return dx * dx + dy * dy + dz * dz;
}

/**
 * @brief 检查实体类型是否匹配选择器的 type 条件。
 */
[[nodiscard]] bool matchesEntityType(const Entity& entity, const EntitySelector& selector)
{
    if (!selector.hasEntityType()) {
        return true;
    }

    const std::string& expectedType = selector.entityType();
    const std::string& actualType = entity.getTypeId();

    // 支持带或不带 minecraft: 前缀的匹配
    bool matches = false;
    if (actualType == expectedType) {
        matches = true;
    } else if (expectedType.find(':') == std::string::npos) {
        // 选择器未指定命名空间，尝试在 actualType 中匹配后缀
        // 例如 "zombie" 匹配 "minecraft:zombie"
        if (actualType.size() > expectedType.size()) {
            std::string_view suffix(actualType.data() + actualType.size() - expectedType.size(), expectedType.size());
            matches = (suffix == expectedType) && (actualType[actualType.size() - expectedType.size() - 1] == ':');
        }
    }

    return selector.entityTypeNegated() ? !matches : matches;
}

/**
 * @brief 检查实体标签是否匹配选择器的 tag 条件。
 */
[[nodiscard]] bool matchesEntityTags(const Entity& entity, const EntitySelector& selector)
{
    // 正向标签：实体必须拥有所有指定标签
    for (const auto& tag : selector.tags()) {
        if (!entity.hasTag(tag)) {
            return false;
        }
    }

    // 反向标签：实体必须不拥有任何指定标签
    for (const auto& tag : selector.tagsNegated()) {
        if (entity.hasTag(tag)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief 检查实体队伍是否匹配选择器的 team 条件。
 */
[[nodiscard]] bool matchesTeam(const Entity& entity, const EntitySelector& selector)
{
    if (!selector.hasTeam()) {
        return true;
    }

    auto* team = entity.getTeam();
    bool onTeam = false;
    if (team != nullptr) {
        onTeam = (team->getName() == selector.team());
    }

    return selector.teamNegated() ? !onTeam : onTeam;
}

/**
 * @brief 检查实体名称是否匹配选择器的 name 条件。
 */
[[nodiscard]] bool matchesName(Entity& entity, const EntitySelector& selector)
{
    // 尝试将实体转换为玩家
    auto* player = dynamic_cast<Player*>(&entity);

    // 正向名称匹配
    if (selector.hasUsername()) {
        bool matches = false;
        // 对于玩家实体，优先匹配用户名
        if (player != nullptr) {
            matches = (player->username() == selector.username());
        }
        // 对于所有实体，也检查自定义名称
        if (!matches && entity.hasCustomName()) {
            matches = (entity.customNameText() == selector.username());
        }
        if (!matches) {
            return false;
        }
    }

    // 反向名称匹配
    if (selector.hasUsernameNegated()) {
        bool matches = false;
        if (player != nullptr) {
            matches = (player->username() == selector.usernameNegated());
        }
        if (!matches && entity.hasCustomName()) {
            matches = (entity.customNameText() == selector.usernameNegated());
        }
        if (matches) {
            return false;
        }
    }

    return true;
}

/**
 * @brief 检查玩家特有的条件（等级、游戏模式、记分板、进度）。
 *
 * 对于非玩家实体，等级和游戏模式条件始终返回 false（不匹配），
 * 因为这些条件只适用于玩家。
 */
[[nodiscard]] bool matchesPlayerConditions(
    Entity& entity, const EntitySelector& selector, server::IServer* server, server::ServerWorld* world)
{
    auto* player = dynamic_cast<Player*>(&entity);
    bool isPlayer = (player != nullptr);

    // 等级过滤（仅玩家）
    if (!selector.level().isUnbounded()) {
        if (!isPlayer) {
            return false;
        }
        i32 level = player->experienceLevel();
        if (!selector.level().test(level)) {
            return false;
        }
    }

    // 俯仰角过滤（x_rotation）
    if (!selector.xRotation().isUnbounded()) {
        if (!selector.xRotation().testAngle(entity.pitch())) {
            return false;
        }
    }

    // 偏航角过滤（y_rotation）
    if (!selector.yRotation().isUnbounded()) {
        if (!selector.yRotation().testAngle(entity.yaw())) {
            return false;
        }
    }

    // 游戏模式过滤（仅玩家）
    if (selector.hasGameMode()) {
        if (!isPlayer) {
            return false;
        }
        const std::string& mode = selector.gameMode();
        bool matches = false;
        switch (player->gameMode()) {
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

    // 记分板分数过滤（仅玩家）
    if (selector.hasScoreConditions()) {
        if (!isPlayer) {
            return false;
        }
        if (server == nullptr) {
            return false;
        }
        auto& scoreboard = server->scoreboard();
        const std::string& playerName = player->username();

        for (const auto& [objectiveName, range] : selector.scoreConditions()) {
            auto* objective = scoreboard.getObjective(objectiveName);
            if (objective == nullptr) {
                return false;
            }
            if (!scoreboard.entityHasObjective(playerName, *objective)) {
                return false;
            }
            auto* score = scoreboard.getScore(playerName, *objective);
            if (score == nullptr) {
                return false;
            }
            if (!range.test(score->getScorePoints())) {
                return false;
            }
        }
    }

    // 进度过滤（仅玩家）
    if (selector.hasAdvancementConditions()) {
        if (!isPlayer) {
            return false;
        }
        server::PlayerAdvancements* playerAdvancements = nullptr;
        if (server != nullptr && world != nullptr) {
            auto* serverPlayer = player->asServerPlayer();
            if (serverPlayer != nullptr) {
                playerAdvancements = serverPlayer->getAdvancements();
            }
        }

        if (playerAdvancements == nullptr) {
            return false;
        }

        auto& manager = advancement::AdvancementManager::instance();
        for (const auto& [advancementId, condition] : selector.advancementConditions()) {
            auto advancement = manager.get(advancementId);
            if (advancement == nullptr) {
                return false;
            }

            auto* progress = playerAdvancements->getProgress(advancement);
            if (progress == nullptr) {
                return false;
            }

            if (condition.isComplete.has_value()) {
                bool isDone = progress->isDone();
                if (isDone != condition.isComplete.value()) {
                    return false;
                }
            }

            for (const auto& [criteriaName, expectedComplete] : condition.criteriaConditions) {
                auto* criterionProgress = progress->getCriterion(criteriaName);
                if (criterionProgress == nullptr) {
                    return false;
                }
                bool isObtained = criterionProgress->isObtained();
                if (isObtained != expectedComplete) {
                    return false;
                }
            }
        }
    }

    // TODO(待完善): NBT 条件过滤逻辑
    // 依赖：Entity 类需要实现 serializeNBT() 方法以获取实体的 NBT 数据
    // 当前行为：跳过 NBT 检查，不排除任何实体

    // TODO(待完善): 谓词条件过滤逻辑
    // 依赖：需要 LootConditionManager 和 LootContext 支持战利品表谓词评估
    // 当前行为：跳过谓词检查，不排除任何实体

    return true;
}

/**
 * @brief 对实体列表进行排序。
 */
void sortEntities(std::vector<Entity*>& entities, const EntitySelector& selector, f32 refX, f32 refY, f32 refZ)
{
    switch (selector.sort()) {
        case EntitySelectorSort::Nearest:
            std::sort(entities.begin(), entities.end(), [&](Entity* a, Entity* b) {
                return entityDistanceSq(*a, refX, refY, refZ) < entityDistanceSq(*b, refX, refY, refZ);
            });
            break;
        case EntitySelectorSort::Furthest:
            std::sort(entities.begin(), entities.end(), [&](Entity* a, Entity* b) {
                return entityDistanceSq(*a, refX, refY, refZ) > entityDistanceSq(*b, refX, refY, refZ);
            });
            break;
        case EntitySelectorSort::Random: {
            math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
            for (size_t i = entities.size(); i > 1; --i) {
                size_t j = static_cast<size_t>(rng.nextInt(static_cast<i32>(i)));
                std::swap(entities[i - 1], entities[j]);
            }
            break;
        }
        case EntitySelectorSort::Arbitrary:
        default:
            break;
    }
}

/**
 * @brief 收集所有符合条件的玩家实体。
 *
 * 仅从 PlayerManager 中收集在线玩家，应用选择器的所有过滤条件。
 */
[[nodiscard]] std::vector<Entity*> collectPlayers(
    const ServerCommandSource& source, const EntitySelector& selector, f32 refX, f32 refY, f32 refZ)
{
    auto* server = source.server();
    auto* world = source.world();
    if (server == nullptr) {
        return {};
    }

    std::vector<Entity*> result;

    // 按用户名精确匹配
    if (selector.hasUsername()) {
        auto* playerManager = &server->playerManager();
        playerManager->forEachPlayer([&](const server::ServerPlayerData& playerData) {
            if (playerData.username == selector.username()) {
                Player* player = server->playerEntityManager().getPlayerEntity(playerData.playerId, *world);
                if (player != nullptr && player->isAlive()) {
                    result.push_back(player);
                }
            }
        });
        return result;
    }

    // 收集所有在线玩家
    server->playerManager().forEachPlayer([&](const server::ServerPlayerData& playerData) {
        Player* player = server->playerEntityManager().getPlayerEntity(playerData.playerId, *world);
        if (player == nullptr) {
            return;
        }
        if (!player->isAlive()) {
            return;
        }

        auto* entity = static_cast<Entity*>(player);

        // 距离过滤
        if (!selector.distance().isUnbounded()) {
            f32 distSq = entityDistanceSq(*entity, refX, refY, refZ);
            if (!selector.distance().testSquared(distSq)) {
                return;
            }
        }

        // 体积过滤（dx/dy/dz）
        if (selector.hasDx() || selector.hasDy() || selector.hasDz()) {
            const auto& pos = entity->position();
            f32 minX = refX;
            f32 minY = refY;
            f32 minZ = refZ;
            f32 maxX = pos.x;
            f32 maxY = pos.y;
            f32 maxZ = pos.z;

            if (selector.hasDx()) {
                f32 dx = selector.getDx();
                if (dx < 0) {
                    minX = refX + dx;
                    maxX = refX;
                } else {
                    maxX = refX + dx;
                }
            }
            if (selector.hasDy()) {
                f32 dy = selector.getDy();
                if (dy < 0) {
                    minY = refY + dy;
                    maxY = refY;
                } else {
                    maxY = refY + dy;
                }
            }
            if (selector.hasDz()) {
                f32 dz = selector.getDz();
                if (dz < 0) {
                    minZ = refZ + dz;
                    maxZ = refZ;
                } else {
                    maxZ = refZ + dz;
                }
            }

            const auto& ePos = entity->position();
            if (ePos.x < minX || ePos.x > maxX || ePos.y < minY || ePos.y > maxY || ePos.z < minZ || ePos.z > maxZ) {
                return;
            }
        }

        // 名称过滤
        if (!matchesName(*entity, selector)) {
            return;
        }

        // 实体类型过滤
        if (!matchesEntityType(*entity, selector)) {
            return;
        }

        // 标签过滤
        if (!matchesEntityTags(*entity, selector)) {
            return;
        }

        // 队伍过滤
        if (!matchesTeam(*entity, selector)) {
            return;
        }

        // 玩家特有条件（等级、游戏模式、角度、记分板、进度）
        if (!matchesPlayerConditions(*entity, selector, server, world)) {
            return;
        }

        result.push_back(entity);
    });

    return result;
}

/**
 * @brief 收集所有符合条件的实体（包括玩家和非玩家）。
 *
 * 从世界中的 EntityManager 收集所有实体，应用选择器的所有过滤条件。
 */
[[nodiscard]] std::vector<Entity*> collectAllEntities(
    const ServerCommandSource& source, const EntitySelector& selector, f32 refX, f32 refY, f32 refZ)
{
    auto* server = source.server();
    auto* world = source.world();
    if (server == nullptr || world == nullptr) {
        return {};
    }

    std::vector<Entity*> result;

    // 计算体积范围（如果有的话）
    // MC 原版行为：如果有 dx/dy/dz 或 distance 有最大值，则使用空间查询优化
    std::optional<f32> maxDistSq;
    if (!selector.distance().isUnbounded() && selector.distance().hasMax()) {
        f32 maxDist = selector.distance().getMax();
        maxDistSq = maxDist * maxDist;
    }

    // 使用空间查询收集候选实体
    if (maxDistSq.has_value()) {
        f32 maxDist = std::sqrt(maxDistSq.value());
        result = world->getEntitiesInRange(Vector3(refX, refY, refZ), maxDist);
    } else if (selector.hasDx() || selector.hasDy() || selector.hasDz()) {
        // 根据体积构建 AABB
        f32 minX = refX, minY = refY, minZ = refZ;
        f32 maxX = refX, maxY = refY, maxZ = refZ;

        if (selector.hasDx()) {
            f32 dx = selector.getDx();
            if (dx < 0) {
                minX = refX + dx;
            } else {
                maxX = refX + dx;
            }
        }
        if (selector.hasDy()) {
            f32 dy = selector.getDy();
            if (dy < 0) {
                minY = refY + dy;
            } else {
                maxY = refY + dy;
            }
        }
        if (selector.hasDz()) {
            f32 dz = selector.getDz();
            if (dz < 0) {
                minZ = refZ + dz;
            } else {
                maxZ = refZ + dz;
            }
        }

        AxisAlignedBB aabb(minX, minY, minZ, maxX, maxY, maxZ);
        result = world->getEntitiesInAABB(aabb);
    } else {
        // 没有空间约束，遍历所有实体
        world->entityManager().forEachEntity([&](Entity* entity) {
            result.push_back(entity);
            return true;
        });
    }

    // 应用过滤条件
    result.erase(
        std::remove_if(result.begin(),
            result.end(),
            [&](Entity* entity) {
                // @e 选择器排除死亡实体（MC 原版行为）
                if (!entity->isAlive()) {
                    return true;
                }

                // 距离过滤
                if (!selector.distance().isUnbounded()) {
                    f32 distSq = entityDistanceSq(*entity, refX, refY, refZ);
                    if (!selector.distance().testSquared(distSq)) {
                        return true;
                    }
                }

                // 体积过滤
                if (selector.hasDx() || selector.hasDy() || selector.hasDz()) {
                    const auto& pos = entity->position();
                    f32 minX = refX, minY = refY, minZ = refZ;
                    f32 maxX = refX, maxY = refY, maxZ = refZ;

                    if (selector.hasDx()) {
                        f32 dx = selector.getDx();
                        if (dx < 0) {
                            minX = refX + dx;
                        } else {
                            maxX = refX + dx;
                        }
                    }
                    if (selector.hasDy()) {
                        f32 dy = selector.getDy();
                        if (dy < 0) {
                            minY = refY + dy;
                        } else {
                            maxY = refY + dy;
                        }
                    }
                    if (selector.hasDz()) {
                        f32 dz = selector.getDz();
                        if (dz < 0) {
                            minZ = refZ + dz;
                        } else {
                            maxZ = refZ + dz;
                        }
                    }

                    if (pos.x < minX || pos.x > maxX || pos.y < minY || pos.y > maxY || pos.z < minZ || pos.z > maxZ) {
                        return true;
                    }
                }

                // 名称过滤
                if (!matchesName(*entity, selector)) {
                    return true;
                }

                // 实体类型过滤
                if (!matchesEntityType(*entity, selector)) {
                    return true;
                }

                // 标签过滤
                if (!matchesEntityTags(*entity, selector)) {
                    return true;
                }

                // 队伍过滤
                if (!matchesTeam(*entity, selector)) {
                    return true;
                }

                // 玩家特有条件（等级、游戏模式、角度、记分板、进度）
                if (!matchesPlayerConditions(*entity, selector, server, world)) {
                    return true;
                }

                return false;
            }),
        result.end());

    return result;
}

} // namespace

std::vector<Entity*> EntityResolver::resolve(const ServerCommandSource& source, const EntitySelector& selector)
{
    if (source.server() == nullptr) {
        return {};
    }

    // 计算参考点坐标
    f32 refX = static_cast<f32>(source.position().x);
    f32 refY = static_cast<f32>(source.position().y);
    f32 refZ = static_cast<f32>(source.position().z);

    if (selector.hasX()) {
        refX = selector.getX();
    }
    if (selector.hasY()) {
        refY = selector.getY();
    }
    if (selector.hasZ()) {
        refZ = selector.getZ();
    }

    std::vector<Entity*> resolved;

    switch (selector.type()) {
        case EntitySelectorType::Self: {
            // @s: 返回命令源自身的实体
            auto* player = source.player();
            if (player == nullptr) {
                return {};
            }
            Entity* entity = static_cast<Entity*>(player);
            // 对自身也应用过滤条件
            if (!selector.distance().isUnbounded()) {
                f32 distSq = entityDistanceSq(*entity, refX, refY, refZ);
                if (!selector.distance().testSquared(distSq)) {
                    return {};
                }
            }
            if (!matchesName(*entity, selector)) {
                return {};
            }
            if (!matchesEntityType(*entity, selector)) {
                return {};
            }
            if (!matchesEntityTags(*entity, selector)) {
                return {};
            }
            if (!matchesTeam(*entity, selector)) {
                return {};
            }
            if (!matchesPlayerConditions(*entity, selector, source.server(), source.world())) {
                return {};
            }
            resolved.push_back(entity);
            break;
        }

        case EntitySelectorType::SinglePlayer: {
            // @p: 最近的玩家
            auto players = collectPlayers(source, selector, refX, refY, refZ);
            sortEntities(players, selector, refX, refY, refZ);
            if (!players.empty()) {
                resolved.push_back(players.front());
            }
            break;
        }

        case EntitySelectorType::RandomPlayer: {
            // @r: 随机玩家
            auto players = collectPlayers(source, selector, refX, refY, refZ);
            if (!players.empty()) {
                math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
                size_t idx = static_cast<size_t>(rng.nextInt(static_cast<i32>(players.size())));
                resolved.push_back(players[idx]);
            }
            break;
        }

        case EntitySelectorType::AllPlayers: {
            // @a: 所有玩家
            resolved = collectPlayers(source, selector, refX, refY, refZ);
            sortEntities(resolved, selector, refX, refY, refZ);
            break;
        }

        case EntitySelectorType::AllEntities: {
            // @e: 所有实体
            resolved = collectAllEntities(source, selector, refX, refY, refZ);
            sortEntities(resolved, selector, refX, refY, refZ);
            break;
        }
    }

    // 应用数量限制
    if (selector.limit() > 0 && resolved.size() > static_cast<size_t>(selector.limit())) {
        resolved.resize(static_cast<size_t>(selector.limit()));
    }

    return resolved;
}

Entity* EntityResolver::resolveSingle(const ServerCommandSource& source, const EntitySelector& selector)
{
    auto entities = resolve(source, selector);
    return entities.empty() ? nullptr : entities.front();
}

} // namespace command::support
} // namespace mc
