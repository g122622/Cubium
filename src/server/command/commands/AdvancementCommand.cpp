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

#include "AdvancementCommand.hpp"

#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

namespace {

/**
 * @brief 解析成就ID参数
 */
std::optional<ResourceLocation> parseAdvancementId(const std::string& input)
{
    if (input.empty()) {
        return std::nullopt;
    }

    // 如果没有命名空间，默认使用 minecraft
    if (input.find(':') == std::string::npos) {
        return ResourceLocation("minecraft", input);
    }
    return ResourceLocation(input);
}

/**
 * @brief 收集成就（根据模式）
 */
std::vector<advancement::AdvancementPtr> collectAdvancements(
    advancement::AdvancementPtr target, GrantMode mode, advancement::AdvancementManager& manager)
{
    std::vector<advancement::AdvancementPtr> result;
    std::set<advancement::AdvancementPtr> visited;

    switch (mode) {
        case GrantMode::Only:
            if (target) {
                result.push_back(target);
            }
            break;

        case GrantMode::Everything:
            manager.forEach([&result](advancement::AdvancementPtr adv) {
                result.push_back(adv);
                return true;
            });
            break;

        case GrantMode::From:
            // 授予指定成就及其所有子成就（递归向下）
            if (target) {
                std::function<void(advancement::AdvancementPtr)> collectChildren;
                collectChildren = [&](advancement::AdvancementPtr adv) {
                    if (visited.count(adv) > 0) return;
                    visited.insert(adv);
                    result.push_back(adv);
                    for (const auto& child : adv->getChildren()) {
                        collectChildren(child);
                    }
                };
                collectChildren(target);
            }
            break;

        case GrantMode::Through:
            // 授予从根到指定成就的路径
            if (target) {
                // 向上遍历到根
                std::vector<advancement::AdvancementPtr> path;
                advancement::AdvancementPtr current = target;
                while (current) {
                    path.push_back(current);
                    if (current->getParent().has_value()) {
                        current = manager.get(current->getParent().value());
                    } else {
                        break;
                    }
                }
                // 反转，从根开始添加
                for (auto it = path.rbegin(); it != path.rend(); ++it) {
                    result.push_back(*it);
                }
            }
            break;

        case GrantMode::Until:
            // 授予指定成就及其所有父成就
            if (target) {
                advancement::AdvancementPtr current = target;
                while (current) {
                    result.push_back(current);
                    if (current->getParent().has_value()) {
                        current = manager.get(current->getParent().value());
                    } else {
                        break;
                    }
                }
            }
            break;
    }

    return result;
}

/**
 * @brief 获取玩家的成就进度
 *
 * 通过 IServer → ServerPlayerEntityManager → ServerPlayer 路径获取
 * 玩家成就管理器，确保使用的是与触发器系统关联的 PlayerAdvancements 实例。
 */
server::PlayerAdvancements* getPlayerAdvancements(server::IServer* server, PlayerId playerId)
{
    if (server == nullptr) {
        return nullptr;
    }
    auto* world = server->getPlayerWorld(playerId);
    if (world == nullptr) {
        return nullptr;
    }
    mc::Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
    if (player == nullptr) {
        return nullptr;
    }
    auto* serverPlayer = player->asServerPlayer();
    if (serverPlayer == nullptr) {
        return nullptr;
    }
    return serverPlayer->getAdvancements();
}

} // namespace

void AdvancementCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto advancementNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("advancement");
    advancementNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(advancementNode,
        support::makeMetadata(
            "Grants, revokes, or tests advancements.", "/advancement <grant|revoke|test> <targets> ...", 2, {}, true));

    // ========== GRANT 子命令 ==========
    auto grantNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("grant");

    // /advancement grant <targets> everything
    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());

    auto everythingNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("everything");
    everythingNode->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _grantAdvancement(ctx, GrantMode::Everything); });

    // /advancement grant <targets> only <advancement>
    auto onlyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("only");
    auto advancementArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "advancement", StringArgumentType::string());
    advancementArg->setCommand(
        [=](CommandContext<ServerCommandSource>& ctx) { return _grantAdvancement(ctx, GrantMode::Only); });
    onlyNode->addChild(advancementArg);

    // /advancement grant <targets> from <advancement>
    auto fromNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("from");
    auto fromArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "advancement", StringArgumentType::string());
    fromArg->setCommand(
        [=](CommandContext<ServerCommandSource>& ctx) { return _grantAdvancement(ctx, GrantMode::From); });
    fromNode->addChild(fromArg);

    // /advancement grant <targets> through <advancement>
    auto throughNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("through");
    auto throughArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "advancement", StringArgumentType::string());
    throughArg->setCommand(
        [=](CommandContext<ServerCommandSource>& ctx) { return _grantAdvancement(ctx, GrantMode::Through); });
    throughNode->addChild(throughArg);

    // /advancement grant <targets> until <advancement>
    auto untilNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("until");
    auto untilArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "advancement", StringArgumentType::string());
    untilArg->setCommand(
        [=](CommandContext<ServerCommandSource>& ctx) { return _grantAdvancement(ctx, GrantMode::Until); });
    untilNode->addChild(untilArg);

    // 组合 grant 节点
    targetsArg->addChild(everythingNode);
    targetsArg->addChild(onlyNode);
    targetsArg->addChild(fromNode);
    targetsArg->addChild(throughNode);
    targetsArg->addChild(untilNode);
    grantNode->addChild(targetsArg);

    // ========== REVOKE 子命令 ==========
    auto revokeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("revoke");

    auto revokeTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());

    auto revokeEverythingNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("everything");
    revokeEverythingNode->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _revokeAdvancement(ctx, GrantMode::Everything); });

    // /advancement revoke <targets> only <advancement>
    auto revokeOnlyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("only");
    auto revokeOnlyArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "advancement", StringArgumentType::string());
    revokeOnlyArg->setCommand(
        [=](CommandContext<ServerCommandSource>& ctx) { return _revokeAdvancement(ctx, GrantMode::Only); });
    revokeOnlyNode->addChild(revokeOnlyArg);

    // /advancement revoke <targets> from <advancement>
    auto revokeFromNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("from");
    auto revokeFromArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "advancement", StringArgumentType::string());
    revokeFromArg->setCommand(
        [=](CommandContext<ServerCommandSource>& ctx) { return _revokeAdvancement(ctx, GrantMode::From); });
    revokeFromNode->addChild(revokeFromArg);

    // /advancement revoke <targets> through <advancement>
    auto revokeThroughNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("through");
    auto revokeThroughArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "advancement", StringArgumentType::string());
    revokeThroughArg->setCommand(
        [=](CommandContext<ServerCommandSource>& ctx) { return _revokeAdvancement(ctx, GrantMode::Through); });
    revokeThroughNode->addChild(revokeThroughArg);

    // /advancement revoke <targets> until <advancement>
    auto revokeUntilNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("until");
    auto revokeUntilArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "advancement", StringArgumentType::string());
    revokeUntilArg->setCommand(
        [=](CommandContext<ServerCommandSource>& ctx) { return _revokeAdvancement(ctx, GrantMode::Until); });
    revokeUntilNode->addChild(revokeUntilArg);

    // 组合 revoke 节点
    revokeTargetsArg->addChild(revokeEverythingNode);
    revokeTargetsArg->addChild(revokeOnlyNode);
    revokeTargetsArg->addChild(revokeFromNode);
    revokeTargetsArg->addChild(revokeThroughNode);
    revokeTargetsArg->addChild(revokeUntilNode);
    revokeNode->addChild(revokeTargetsArg);

    // ========== TEST 子命令 ==========
    auto testNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("test");

    auto testTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());

    auto testAdvArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "advancement", StringArgumentType::string());
    testAdvArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _testAdvancement(ctx); });

    testTargetsArg->addChild(testAdvArg);
    testNode->addChild(testTargetsArg);

    // 注册根节点
    advancementNode->addChild(grantNode);
    advancementNode->addChild(revokeNode);
    advancementNode->addChild(testNode);
    dispatcher.registerCommand(advancementNode);
}

i32 AdvancementCommand::_grantAdvancement(CommandContext<ServerCommandSource>& context, GrantMode mode)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    auto& manager = advancement::AdvancementManager::instance();

    // 获取目标成就（仅对非 Everything 模式需要）
    advancement::AdvancementPtr targetAdvancement = nullptr;
    std::string advancementId;
    if (mode != GrantMode::Everything && context.hasArgument("advancement")) {
        advancementId = context.getArgument<std::string>("advancement");
        auto id = parseAdvancementId(advancementId);
        if (!id.has_value()) {
            source.sendError("Invalid advancement ID: " + advancementId);
            return 0;
        }
        targetAdvancement = manager.get(id.value());
        if (!targetAdvancement) {
            source.sendError("Unknown advancement: " + id->toString());
            return 0;
        }
    }

    // 收集要授予的成就
    auto advancements = collectAdvancements(targetAdvancement, mode, manager);

    // 授予成就给每个玩家
    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        auto* playerAdvancements = getPlayerAdvancements(source.server(), playerId);
        if (!playerAdvancements) {
            spdlog::warn("PlayerAdvancements not available for player {}", playerId);
            continue;
        }

        i32 playerSuccess = 0;
        for (auto adv : advancements) {
            if (playerAdvancements->grantAllCriteria(adv)) {
                playerSuccess++;
            }
        }

        if (playerSuccess > 0) {
            successCount++;
        }
    }

    // 发送反馈
    std::ostringstream ss;
    if (mode == GrantMode::Everything) {
        ss << "Granted all advancements to " << successCount << " player(s)";
    } else {
        ss << "Granted " << advancements.size() << " advancement(s) to " << successCount << " player(s)";
    }
    source.sendMessage(ss.str());

    return successCount;
}

i32 AdvancementCommand::_revokeAdvancement(CommandContext<ServerCommandSource>& context, GrantMode mode)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    auto& manager = advancement::AdvancementManager::instance();

    // 获取目标成就（仅对非 Everything 模式需要）
    advancement::AdvancementPtr targetAdvancement = nullptr;
    std::string advancementId;
    if (mode != GrantMode::Everything && context.hasArgument("advancement")) {
        advancementId = context.getArgument<std::string>("advancement");
        auto id = parseAdvancementId(advancementId);
        if (!id.has_value()) {
            source.sendError("Invalid advancement ID: " + advancementId);
            return 0;
        }
        targetAdvancement = manager.get(id.value());
        if (!targetAdvancement) {
            source.sendError("Unknown advancement: " + id->toString());
            return 0;
        }
    }

    // 收集要撤销的成就
    auto advancements = collectAdvancements(targetAdvancement, mode, manager);

    // 从每个玩家撤销成就
    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        auto* playerAdvancements = getPlayerAdvancements(source.server(), playerId);
        if (!playerAdvancements) {
            spdlog::warn("PlayerAdvancements not available for player {}", playerId);
            continue;
        }

        i32 playerSuccess = 0;
        for (auto adv : advancements) {
            if (playerAdvancements->revokeAllCriteria(adv)) {
                playerSuccess++;
            }
        }

        if (playerSuccess > 0) {
            successCount++;
        }
    }

    // 发送反馈
    std::ostringstream ss;
    if (mode == GrantMode::Everything) {
        ss << "Revoked all advancements from " << successCount << " player(s)";
    } else {
        ss << "Revoked " << advancements.size() << " advancement(s) from " << successCount << " player(s)";
    }
    source.sendMessage(ss.str());

    return successCount;
}

i32 AdvancementCommand::_testAdvancement(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const std::string advancementIdStr = context.getArgument<std::string>("advancement");

    auto id = parseAdvancementId(advancementIdStr);
    if (!id.has_value()) {
        source.sendError("Invalid advancement ID: " + advancementIdStr);
        return 0;
    }

    auto& manager = advancement::AdvancementManager::instance();
    auto advancement = manager.get(id.value());
    if (!advancement) {
        source.sendError("Unknown advancement: " + id->toString());
        return 0;
    }

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    // 测试每个玩家
    i32 completedCount = 0;
    for (PlayerId playerId : playerIds) {
        auto* playerAdvancements = getPlayerAdvancements(source.server(), playerId);
        if (!playerAdvancements) {
            spdlog::warn("PlayerAdvancements not available for player {}", playerId);
            continue;
        }

        if (playerAdvancements->isDone(advancement)) {
            completedCount++;
        }
    }

    // 发送结果
    if (playerIds.size() == 1) {
        std::string status = completedCount > 0 ? "has completed" : "has not completed";
        source.sendMessage("Player " + status + " advancement '" + id->toString() + "'");
    } else {
        std::ostringstream ss;
        ss << completedCount << " of " << playerIds.size() << " players have completed advancement '" << id->toString()
           << "'";
        source.sendMessage(ss.str());
    }

    return completedCount;
}

} // namespace command
} // namespace mc
