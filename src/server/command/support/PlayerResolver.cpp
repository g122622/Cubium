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

#include "PlayerResolver.hpp"

#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/conditions/NBTPredicate.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/loot/LootPredicateManager.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/scoreboard/core/Score.hpp"
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
#include <string>
#include <string_view>
#include <vector>

namespace mc::command::support {

namespace {

/**
 * @brief 玩家统一视图。
 *
 * 把 PlayerManager 的 ServerPlayerData 与 ServerPlayerEntityManager 的 Player 实体统一成同一组
 * 过滤/排序所需字段。真实玩家两者皆有（优先用 ServerPlayerData）；SimulatedPlayer 仅注册于
 * ServerPlayerEntityManager 实体层（不进 PlayerManager，见 ServerPlayerEntityManager 注册注释），
 * 经实体回退取位置/朝向/游戏模式/等级。valid=false 表示两者都查不到（不应参与选择）。
 *
 * 这是 @a/@p/@r 选择器对 SimulatedPlayer 解析的关键修复点：原先 applyFilters/sortPlayerIds 内部
 * 仅经 playerManager().getPlayer(id) 取数，SimulatedPlayer 返 nullptr 即被 remove_if 当作"删除"
 * 误剔除，导致带 distance/排序等条件的选择器对 SimulatedPlayer 全部失效（/tp @a[distance=..N]
 * 选不到 SimulatedPlayer，传送不执行）。
 */
struct PlayerView {
    PlayerId playerId = 0;
    std::string username;
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
    GameMode gameMode = GameMode::Survival;
    bool valid = false;
    // 玩家实体指针（SimulatedPlayer 路径持有，matchesFilter 取经验等级/实时朝向/NBT/谓词用）。
    // 仅供本次解析立即使用，不长期持有（对齐 ServerPlayerEntityManager::getPlayerEntity 注释）。
    Player* entity = nullptr;
};

/**
 * @brief 按 PlayerId 解析玩家统一视图。
 *
 * 优先 PlayerManager 的 ServerPlayerData（真实玩家权威数据），回退 ServerPlayerEntityManager 的
 * Player 实体（SimulatedPlayer 路径）。两者皆无时 valid=false。
 */
[[nodiscard]] PlayerView resolvePlayerView(const ServerCommandSource& source, PlayerId playerId)
{
    PlayerView view;
    view.playerId = playerId;
    if (source.server() == nullptr) {
        return view;
    }

    // 真实玩家路径：PlayerManager 持有 ServerPlayerData。
    const server::ServerPlayerData* data = source.server()->playerManager().getPlayer(playerId);
    if (data != nullptr) {
        view.username = data->username;
        view.x = data->x;
        view.y = data->y;
        view.z = data->z;
        view.yaw = data->yaw;
        view.pitch = data->pitch;
        view.gameMode = data->gameMode;
        view.valid = true;
        // 真实玩家实体也注册于 ServerPlayerEntityManager，取实体用于经验等级/实时朝向/NBT/谓词。
        // 取不到不影响基本过滤（位置/游戏模式已由 ServerPlayerData 提供）。
        if (source.world() != nullptr) {
            view.entity = source.server()->playerEntityManager().getPlayerEntity(playerId, *source.world());
        }
        return view;
    }

    // SimulatedPlayer 回退：经 ServerPlayerEntityManager 取 Player 实体。
    auto* world = source.world();
    if (world == nullptr) {
        return view;
    }
    Player* entity = source.server()->playerEntityManager().getPlayerEntity(playerId, *world);
    if (entity == nullptr) {
        return view;
    }
    const auto pos = entity->position();
    view.username = entity->username();
    view.x = static_cast<f32>(pos.x);
    view.y = static_cast<f32>(pos.y);
    view.z = static_cast<f32>(pos.z);
    view.yaw = entity->yaw();
    view.pitch = entity->pitch();
    view.gameMode = entity->gameMode();
    view.entity = entity;
    view.valid = true;
    return view;
}

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

    // 合并 PlayerManager（真实玩家）与 ServerPlayerEntityManager（含 SimulatedPlayer）的 PlayerId。
    // 真实玩家同时存在于两者（PlayerManager 有 ServerPlayerData，实体管理器有 PlayerId↔Entity 映射），
    // 须去重。SimulatedPlayer 仅在实体管理器（不进 PlayerManager），合并后方能被 @a/@p/@r 选中。
    auto playerIds = source.server()->playerManager().getPlayerIds();
    auto entityPlayerIds = source.server()->playerEntityManager().getPlayerIds();
    playerIds.insert(playerIds.end(), entityPlayerIds.begin(), entityPlayerIds.end());
    std::sort(playerIds.begin(), playerIds.end());
    playerIds.erase(std::unique(playerIds.begin(), playerIds.end()), playerIds.end());
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
    if (resolvedPlayerId != 0) {
        return resolvedPlayerId;
    }

    // 回退：SimulatedPlayer 不进 PlayerManager，经 ServerPlayerEntityManager 遍历实体按 username 匹配。
    // 命令源须有世界上下文以解析实体（getPlayerEntity 需 ServerWorld）。
    auto* world = source.world();
    if (world == nullptr) {
        return 0;
    }
    for (PlayerId pid : source.server()->playerEntityManager().getPlayerIds()) {
        mc::Player* entity = source.server()->playerEntityManager().getPlayerEntity(pid, *world);
        if (entity != nullptr && entity->username() == username) {
            return pid;
        }
    }
    return 0;
}

/**
 * @brief 检查玩家是否符合选择器的过滤条件。
 *
 * @param playerData 玩家数据。
 * @param view 玩家统一视图（含 ServerPlayerData 或实体回退数据 + 实体指针）。
 * @param selector 选择器。
 * @param server 服务器实例（用于获取玩家实体）。
 * @param world 世界实例（用于获取玩家实体）。
 * @return 是否符合条件。
 */
[[nodiscard]] bool matchesFilter(
    const PlayerView& view, const EntitySelector& selector, server::IServer* server, server::ServerWorld* world)
{
    // 检查等级范围
    if (!selector.level().isUnbounded()) {
        // 通过 view.entity（PlayerManager 数据路径已取实体，SimulatedPlayer 回退路径亦持有）访问经验等级。
        if (view.entity != nullptr) {
            i32 level = view.entity->experienceLevel();
            if (!selector.level().test(level)) {
                return false;
            }
        }
    }

    // 检查俯仰角范围（x_rotation，-90 到 90 度）
    if (!selector.xRotation().isUnbounded()) {
        // 优先使用实体实时角度，回退视图存储角度。
        f32 pitch = view.pitch;
        if (view.entity != nullptr) {
            pitch = view.entity->pitch();
        }
        if (!selector.xRotation().testAngle(pitch)) {
            return false;
        }
    }

    // 检查偏航角范围（y_rotation，-180 到 180 度）
    if (!selector.yRotation().isUnbounded()) {
        // 优先使用实体实时角度，回退视图存储角度。
        f32 yaw = view.yaw;
        if (view.entity != nullptr) {
            yaw = view.entity->yaw();
        }
        if (!selector.yRotation().testAngle(yaw)) {
            return false;
        }
    }

    // 检查游戏模式
    if (selector.hasGameMode()) {
        const std::string& mode = selector.gameMode();
        bool matches = false;
        switch (view.gameMode) {
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

    // 检查记分板分数条件
    if (selector.hasScoreConditions()) {
        if (server == nullptr) {
            return false;
        }
        auto& scoreboard = server->scoreboard();
        const std::string& playerName = view.username;

        for (const auto& [objectiveName, range] : selector.scoreConditions()) {
            auto* objective = scoreboard.getObjective(objectiveName);
            if (objective == nullptr) {
                // 目标不存在，不匹配
                return false;
            }
            if (!scoreboard.entityHasObjective(playerName, *objective)) {
                // 玩家没有该目标的分数，不匹配
                return false;
            }
            auto* score = scoreboard.getScore(playerName, *objective);
            if (score == nullptr) {
                return false;
            }
            if (!range.test(score->getScorePoints())) {
                // 分数不在范围内，不匹配
                return false;
            }
        }
    }

    // 检查进度条件
    if (selector.hasAdvancementConditions()) {
        // 通过 view.entity（ServerPlayerEntityManager 解析的实体）取 ServerPlayer 的成就进度
        // 而不是使用 ServerPlayerData::advancements（始终为 nullptr）
        server::PlayerAdvancements* playerAdvancements = nullptr;
        if (view.entity != nullptr) {
            auto* serverPlayer = view.entity->asServerPlayer();
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
                // 进度不存在，不匹配
                return false;
            }

            auto* progress = playerAdvancements->getProgress(advancement);
            if (progress == nullptr) {
                // 玩家没有该进度的进度记录，不匹配
                return false;
            }

            // 检查整体完成状态
            if (condition.isComplete.has_value()) {
                bool isDone = progress->isDone();
                if (isDone != condition.isComplete.value()) {
                    return false;
                }
            }

            // 检查各准则的完成状态
            for (const auto& [criteriaName, expectedComplete] : condition.criteriaConditions) {
                auto* criterionProgress = progress->getCriterion(criteriaName);
                if (criterionProgress == nullptr) {
                    // 准则不存在，不匹配
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
        bool matches = false;
        if (view.entity != nullptr) {
            // 将玩家实体序列化为 NBT
            nbt::tags::compound_tag entityNbt;
            view.entity->writeToNBT(entityNbt);
            // 对玩家实体，额外添加 SelectedItem 字段
            const auto& selectedStack = view.entity->inventory().getSelectedStackRef();
            if (!selectedStack.isEmpty()) {
                nbt::tags::compound_tag selectedItemTag;
                selectedStack.toNbt(selectedItemTag);
                entityNbt.value["SelectedItem"] = selectedItemTag.copy();
            }
            // 子集匹配：查询 NBT 中的所有字段必须在实体 NBT 中存在且值相等
            const auto* queryTag = nbtCond.nbt.get();
            matches = (queryTag != nullptr) && advancement::NBTPredicate::matchNBT(*queryTag, entityNbt);
        }
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
        if (view.entity != nullptr && world != nullptr) {
            // 从谓词管理器查找命名谓词
            const std::string predicateId = predCond.predicate.toString();
            const auto* condition = server->predicateManager().getPredicate(predicateId);
            if (condition != nullptr) {
                // 构建 LootContext（THIS_ENTITY + ORIGIN）
                Entity* entity = static_cast<Entity*>(view.entity);
                const auto& pos = entity->position();
                math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
                auto context =
                    loot::LootContextBuilder(*world)
                        .withRandom(rng)
                        .withParameter(loot::LootParams::THIS_ENTITY, entity)
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
 * @brief 计算玩家到参考点的距离平方。
 *
 * @param view 玩家统一视图。
 * @param refX 参考点 X。
 * @param refY 参考点 Y。
 * @param refZ 参考点 Z。
 * @return 距离平方。
 */
[[nodiscard]] f32 distanceSquared(const PlayerView& view, f32 refX, f32 refY, f32 refZ)
{
    const f32 dx = view.x - refX;
    const f32 dy = view.y - refY;
    const f32 dz = view.z - refZ;
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
    switch (selector.sort()) {
        case EntitySelectorSort::Nearest: {
            // 按距离近到远排序（无效视图排到末尾）
            std::sort(playerIds.begin(), playerIds.end(), [&](PlayerId a, PlayerId b) {
                const PlayerView viewA = resolvePlayerView(source, a);
                const PlayerView viewB = resolvePlayerView(source, b);
                if (!viewA.valid) return false;
                if (!viewB.valid) return true;
                return distanceSquared(viewA, refX, refY, refZ) < distanceSquared(viewB, refX, refY, refZ);
            });
            break;
        }
        case EntitySelectorSort::Furthest: {
            // 按距离远到近排序（无效视图排到末尾）
            std::sort(playerIds.begin(), playerIds.end(), [&](PlayerId a, PlayerId b) {
                const PlayerView viewA = resolvePlayerView(source, a);
                const PlayerView viewB = resolvePlayerView(source, b);
                if (!viewA.valid) return false;
                if (!viewB.valid) return true;
                return distanceSquared(viewA, refX, refY, refZ) > distanceSquared(viewB, refX, refY, refZ);
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

    // 距离过滤（经 PlayerView 统一取位置，SimulatedPlayer 经实体回退有位置，不再被误删）
    if (!selector.distance().isUnbounded()) {
        playerIds.erase(std::remove_if(playerIds.begin(),
                            playerIds.end(),
                            [&](PlayerId id) {
                                const PlayerView view = resolvePlayerView(source, id);
                                if (!view.valid) return true;
                                const f32 distSq = distanceSquared(view, refX, refY, refZ);
                                return !selector.distance().testSquared(distSq);
                            }),
            playerIds.end());
    }

    // 名称过滤
    if (selector.hasUsername()) {
        playerIds.erase(std::remove_if(playerIds.begin(),
                            playerIds.end(),
                            [&](PlayerId id) {
                                const PlayerView view = resolvePlayerView(source, id);
                                if (!view.valid) return true;
                                return view.username != selector.username();
                            }),
            playerIds.end());
    }
    if (selector.hasUsernameNegated()) {
        playerIds.erase(std::remove_if(playerIds.begin(),
                            playerIds.end(),
                            [&](PlayerId id) {
                                const PlayerView view = resolvePlayerView(source, id);
                                if (!view.valid) return true;
                                return view.username == selector.usernameNegated();
                            }),
            playerIds.end());
    }

    // 通用过滤（包含等级检查和游戏模式检查）
    playerIds.erase(std::remove_if(playerIds.begin(),
                        playerIds.end(),
                        [&](PlayerId id) {
                            const PlayerView view = resolvePlayerView(source, id);
                            if (!view.valid) return true;
                            return !matchesFilter(view, selector, server, world);
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
 * @brief 通过 PlayerId 获取玩家名称。
 *
 * 查找 PlayerManager 中对应 PlayerId 的 ServerPlayerData，返回其 username。PlayerManager 查不到时
 * （SimulatedPlayer 仅注册于 ServerPlayerEntityManager 实体层映射，不进 PlayerManager，见
 * ServerPlayerEntityManager::registerExistingPlayerEntity 注释），回退到经
 * ServerPlayerEntityManager::getPlayerEntity 取 Player::username。两者皆查不到方返 "player_<id>"
 * 兜底（与 getSortedPlayerIds 合并 PlayerManager+实体管理器的对称解析）。
 */
std::string resolvePlayerName(const ServerCommandSource& source, PlayerId playerId)
{
    auto* server = source.server();
    if (server != nullptr) {
        auto* playerData = server->playerManager().getPlayer(playerId);
        if (playerData != nullptr) {
            return playerData->username;
        }
        // SimulatedPlayer 不在 PlayerManager，经实体管理器解析其 Player 实体取 username（对齐 @s/@a
        // 选择器解析路径，否则 team 成员名回退 "player_<id>" 致 hasMember 失配）。
        auto* world = source.world();
        if (world != nullptr) {
            auto* playerEntity = server->playerEntityManager().getPlayerEntity(playerId, *world);
            if (playerEntity != nullptr) {
                return playerEntity->username();
            }
        }
    }
    // 回退：使用 PlayerId 生成临时名称（与 MC 原版行为一致，非玩家实体使用 UUID 字符串）
    return "player_" + std::to_string(playerId);
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

PlayerInventory* resolvePlayerInventory(ServerCommandSource& source, PlayerId playerId)
{
    auto* server = source.server();
    if (server == nullptr) {
        return nullptr;
    }

    // 优先经 InventoryManager（网络层）取真实玩家背包。真实玩家登录时由 LoginFlow 调
    // initializeInventory 注册，InventoryManager 持有与客户端同步的权威背包。
    PlayerInventory* inventory = server->playerInventory(playerId);
    if (inventory != nullptr) {
        return inventory;
    }

    // 回退实体层：SimulatedPlayer 不走登录流程，不在 InventoryManager 注册（getInventory 返 nullptr），
    // 其权威背包是实体层 Player::m_inventory。经 ServerPlayerEntityManager 取实体层 inventory()。
    // 对齐 EffectCommand/GameModeCommand/TeleportCommand 旁路 SimulatedPlayer 的既有模式。
    auto* world = source.world();
    if (world == nullptr) {
        return nullptr;
    }
    mc::Player* entity = server->playerEntityManager().getPlayerEntity(playerId, *world);
    if (entity == nullptr) {
        return nullptr;
    }
    return &entity->inventory();
}

} // namespace mc::command::support
