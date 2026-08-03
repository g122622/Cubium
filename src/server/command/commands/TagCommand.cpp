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

#include "TagCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

void TagCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto tagNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tag");
    tagNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(
        tagNode, support::makeMetadata("Manages entity tags.", "/tag <targets> <add|remove|list> [tag]", 2, {}, true));

    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());

    // /tag <targets> add <tag>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto tagArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("tag", StringArgumentType::string());
    tagArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addTag(ctx); });
    addNode->addChild(tagArg);
    targetsArg->addChild(addNode);

    // /tag <targets> remove <tag>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTagArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("tag", StringArgumentType::string());
    removeTagArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeTag(ctx); });
    removeNode->addChild(removeTagArg);
    targetsArg->addChild(removeNode);

    // /tag <targets> list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _listTags(ctx); });
    targetsArg->addChild(listNode);

    tagNode->addChild(targetsArg);
    dispatcher.registerCommand(tagNode);
}

/**
 * @brief 获取实体的显示名称。
 *
 * 优先使用玩家用户名，其次使用自定义名称，最后使用实体类型ID。
 */
[[nodiscard]] static std::string getEntityDisplayName(const Entity& entity)
{
    auto* player = dynamic_cast<const Player*>(&entity);
    if (player != nullptr) {
        return player->username();
    }
    if (entity.hasCustomName()) {
        return entity.customNameText();
    }
    return entity.getTypeId();
}

i32 TagCommand::_addTag(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const std::string tag = context.getArgument<std::string>("tag");

    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    i32 successCount = 0;
    for (Entity* entity : entities) {
        if (entity->addTag(tag)) {
            successCount++;
        }
    }

    if (successCount == 0) {
        source.sendError("All entities already have the tag '" + tag + "' or reached tag limit (1024)");
        return 0;
    }

    std::ostringstream ss;
    ss << "Added tag '" << tag << "' to " << successCount << " entit" << (successCount == 1 ? "y" : "ies");
    source.sendMessage(ss.str());

    return successCount;
}

i32 TagCommand::_removeTag(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const std::string tag = context.getArgument<std::string>("tag");

    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    i32 successCount = 0;
    for (Entity* entity : entities) {
        if (entity->removeTag(tag)) {
            successCount++;
        }
    }

    if (successCount == 0) {
        source.sendError("No entities had the tag '" + tag + "'");
        return 0;
    }

    std::ostringstream ss;
    ss << "Removed tag '" << tag << "' from " << successCount << " entit" << (successCount == 1 ? "y" : "ies");
    source.sendMessage(ss.str());

    return successCount;
}

i32 TagCommand::_listTags(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    // 收集所有唯一的标签
    std::set<std::string> allTags;
    std::vector<std::pair<std::string, std::set<std::string>>> entityTags;

    for (Entity* entity : entities) {
        const auto& tags = entity->getTags();
        std::string entityName = getEntityDisplayName(*entity);

        entityTags.emplace_back(std::move(entityName), tags);
        for (const auto& tag : tags) {
            allTags.insert(tag);
        }
    }

    // 输出结果
    if (entities.size() == 1) {
        // 单个实体：显示该实体的所有标签
        if (entityTags.empty() || entityTags[0].second.empty()) {
            source.sendMessage(entityTags.empty() ? "Entity has no tags" : entityTags[0].first + " has no tags");
        } else {
            const auto& [name, tags] = entityTags[0];
            std::ostringstream ss;
            ss << name << " has " << tags.size() << " tag" << (tags.size() == 1 ? "" : "s") << ":";
            for (const auto& tag : tags) {
                ss << " " << tag;
            }
            source.sendMessage(ss.str());
        }
    } else {
        // 多个实体：显示所有实体的标签总数
        if (allTags.empty()) {
            source.sendMessage("No tags found on " + std::to_string(entities.size()) + " entities");
        } else {
            std::ostringstream ss;
            ss << "There are " << allTags.size() << " unique tag" << (allTags.size() == 1 ? "" : "s") << " on "
               << entities.size() << " entities:";
            for (const auto& tag : allTags) {
                ss << " " << tag;
            }
            source.sendMessage(ss.str());
        }
    }

    return static_cast<i32>(allTags.size());
}

} // namespace command
} // namespace mc
