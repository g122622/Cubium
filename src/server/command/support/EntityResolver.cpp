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
#include "common/advancement/trigger/conditions/NBTPredicate.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/loot/LootPredicateManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mc {
namespace command::support {

namespace {

/**
 * @brief 根据选择器的 dx/dy/dz 参数和参考坐标，构造绝对坐标下的选择 AABB。
 *
 * 遵循 MC 原版 EntitySelectorParser.createAabb + getAbsoluteAabb 逻辑。
 * 如果选择器没有体积约束，返回 std::nullopt。
 */
[[nodiscard]] std::optional<AxisAlignedBB> createSelectorAabb(
    const EntitySelector& selector, f32 refX, f32 refY, f32 refZ)
{
    auto relativeAabb = selector.createAabb();
    if (!relativeAabb.has_value()) {
        return std::nullopt;
    }
    // 将相对 AABB 平移到绝对坐标
    const auto& rel = relativeAabb.value();
    return AxisAlignedBB(
        rel.minX + refX, rel.minY + refY, rel.minZ + refZ, rel.maxX + refX, rel.maxY + refY, rel.maxZ + refZ);
}

/**
 * @brief 检查实体碰撞箱是否与选择器 AABB 相交。
 *
 * MC 原版使用 AABB.intersects(entity.boundingBox()) 进行体积过滤，
 * 而非简单的位置点包含检查。这样实体体积较大或跨越边界时行为正确。
 */
[[nodiscard]] bool matchesVolume(const Entity& entity, const AxisAlignedBB& selectorAabb)
{
    return selectorAabb.intersects(entity.boundingBox());
}

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

    // NBT 条件过滤
    if (selector.hasNbtCondition()) {
        const auto& nbtCond = selector.nbtCondition();
        // 将实体序列化为 NBT
        nbt::tags::compound_tag entityNbt;
        entity.writeToNBT(entityNbt);
        // 对玩家实体，额外添加 SelectedItem 字段
        auto* player = dynamic_cast<Player*>(&entity);
        if (player != nullptr) {
            const auto& selectedStack = player->inventory().getSelectedStackRef();
            if (!selectedStack.isEmpty()) {
                nbt::tags::compound_tag selectedItemTag;
                selectedStack.toNbt(selectedItemTag);
                entityNbt.value["SelectedItem"] = selectedItemTag.copy();
            }
        }
        // 子集匹配：查询 NBT 中的所有字段必须在实体 NBT 中存在且值相等
        const auto* queryTag = nbtCond.nbt.get();
        bool matches = (queryTag != nullptr) && advancement::NBTPredicate::matchNBT(*queryTag, entityNbt);
        if (nbtCond.negated) {
            matches = !matches;
        }
        if (!matches) {
            return false;
        }
    }

    // 谓词条件过滤
    if (selector.hasPredicateCondition()) {
        const auto& predCond = selector.predicateCondition();
        bool matches = false;
        if (server != nullptr && world != nullptr) {
            // 从谓词管理器查找命名谓词
            const std::string predicateId = predCond.predicate.toString();
            const auto* condition = server->predicateManager().getPredicate(predicateId);
            if (condition != nullptr) {
                // 构建 LootContext（THIS_ENTITY + ORIGIN）
                const auto& pos = entity.position();
                math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
                auto context =
                    loot::LootContextBuilder(*world)
                        .withRandom(rng)
                        .withParameter(loot::LootParams::THIS_ENTITY, &entity)
                        .withOwnedValue(loot::LootParams::BLOCK_POS,
                            BlockPos(static_cast<i32>(pos.x), static_cast<i32>(pos.y), static_cast<i32>(pos.z)))
                        .withPredicateResolver([&predicateManager = server->predicateManager()](
                                                   const std::string& id) -> const loot::LootCondition* {
                            return predicateManager.getPredicate(id);
                        })
                        .build(loot::LootParameterSets::selector());
                // 循环引用检测
                if (!context->pushPredicate(condition)) {
                    matches = false;
                } else {
                    matches = condition->test(*context);
                    context->popPredicate(condition);
                }
            }
            // 谓词不存在时返回 false（不匹配）
        }
        if (predCond.negated) {
            matches = !matches;
        }
        if (!matches) {
            return false;
        }
    }

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
    auto selectorAabb = createSelectorAabb(selector, refX, refY, refZ);

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

        // 体积过滤（MC 原版行为：使用实体碰撞箱与选择 AABB 的相交检查）
        if (selectorAabb.has_value() && !matchesVolume(*entity, selectorAabb.value())) {
            return;
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

    // 构造选择器 AABB（绝对坐标）
    auto selectorAabb = createSelectorAabb(selector, refX, refY, refZ);

    // 使用空间查询收集候选实体
    if (selectorAabb.has_value()) {
        // 使用 AABB 相交查询作为空间预过滤
        result = world->getEntitiesInAABB(selectorAabb.value());
    } else {
        // 没有空间约束，遍历所有实体
        world->entityManager().forEachEntity([&](Entity* entity) {
            result.push_back(entity);
            return true;
        });
    }

    // 应用过滤条件
    result.erase(std::remove_if(result.begin(),
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

                         // 体积过滤（MC 原版行为：使用实体碰撞箱与选择 AABB 的相交检查）
                         if (selectorAabb.has_value() && !matchesVolume(*entity, selectorAabb.value())) {
                             return true;
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
            // 支持非玩家实体执行者（如 /execute as @e[type=zombie] run kill @s）
            Entity* entity = source.entity();
            if (entity == nullptr) {
                // 命令源没有关联实体（如控制台、命令方块执行）
                return {};
            }
            // 对自身也应用过滤条件
            if (!selector.distance().isUnbounded()) {
                f32 distSq = entityDistanceSq(*entity, refX, refY, refZ);
                if (!selector.distance().testSquared(distSq)) {
                    return {};
                }
            }
            // 体积过滤
            auto selectorAabb = createSelectorAabb(selector, refX, refY, refZ);
            if (selectorAabb.has_value() && !matchesVolume(*entity, selectorAabb.value())) {
                return {};
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
